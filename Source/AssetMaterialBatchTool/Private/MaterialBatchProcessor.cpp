#include "MaterialBatchProcessor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "Misc/ScopedSlowTask.h"
#include "ScopedTransaction.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "FMaterialBatchProcessor"

namespace
{
    struct FTextureRule
    {
        FName ParameterName;
        TArray<FString> Suffixes;
    };

    const TArray<FTextureRule>& GetTextureRules()
    {
        static const TArray<FTextureRule> Rules =
        {
            { TEXT("BaseColor"), { TEXT("_BaseColor"), TEXT("_Base_Color"), TEXT("_Albedo"), TEXT("_Diffuse"), TEXT("_D"), TEXT("_BC"), TEXT("_BaseMap") } },
            { TEXT("Normal"),    { TEXT("_Normal"), TEXT("_N"), TEXT("_NRM") } },
            { TEXT("ORM"),       { TEXT("_ORM"), TEXT("_MRA"), TEXT("_Mask"), TEXT("_OcclusionRoughnessMetallic") } },
            { TEXT("Roughness"), { TEXT("_Roughness"), TEXT("_R") } },
            { TEXT("Metallic"),  { TEXT("_Metallic"), TEXT("_Metalness"), TEXT("_M") } },
            { TEXT("Specular"),  { TEXT("_Specular"), TEXT("_S") } },
            { TEXT("Opacity"),   { TEXT("_Opacity"), TEXT("_Alpha"), TEXT("_A") } },
            { TEXT("Emissive"),  { TEXT("_Emissive"), TEXT("_Emission"), TEXT("_E") } }
        };
        return Rules;
    }

    bool IsClassPath(const FAssetData& AssetData, const UClass* Class)
    {
        return AssetData.IsValid() && Class && AssetData.AssetClassPath == Class->GetClassPathName();
    }

    bool IsTextureAsset(const FAssetData& AssetData)
    {
        return IsClassPath(AssetData, UTexture2D::StaticClass()) || IsClassPath(AssetData, UTexture::StaticClass());
    }

    bool IsMaterialInterfaceAsset(const FAssetData& AssetData)
    {
        return IsClassPath(AssetData, UMaterial::StaticClass()) || IsClassPath(AssetData, UMaterialInstanceConstant::StaticClass());
    }

    FString JoinPackageAsset(const FString& PackagePath, const FString& AssetName)
    {
        FString CleanPath = PackagePath;
        CleanPath.RemoveFromEnd(TEXT("/"));
        return CleanPath / AssetName;
    }

    FString GetObjectPath(const FString& PackagePath, const FString& AssetName)
    {
        const FString PackageAssetPath = JoinPackageAsset(PackagePath, AssetName);
        return PackageAssetPath + TEXT(".") + AssetName;
    }

    void RemoveKnownPrefix(FString& Name)
    {
        const FString Prefixes[] = { TEXT("SM_"), TEXT("SK_"), TEXT("T_"), TEXT("MI_"), TEXT("M_") };
        for (const FString& Prefix : Prefixes)
        {
            if (Name.StartsWith(Prefix, ESearchCase::IgnoreCase))
            {
                Name.RightChopInline(Prefix.Len(), EAllowShrinking::No);
                return;
            }
        }
    }

    void RemoveKnownTextureSuffix(FString& Name)
    {
        for (const FTextureRule& Rule : GetTextureRules())
        {
            for (const FString& Suffix : Rule.Suffixes)
            {
                if (Name.EndsWith(Suffix, ESearchCase::IgnoreCase) && Name.Len() > Suffix.Len())
                {
                    Name.LeftChopInline(Suffix.Len(), EAllowShrinking::No);
                    return;
                }
            }
        }
    }

    AssetMaterialBatch::FTextureGroup* FindGroupByKey(TArray<AssetMaterialBatch::FTextureGroup>& Groups, const FString& MatchKey)
    {
        for (AssetMaterialBatch::FTextureGroup& Group : Groups)
        {
            if (Group.MatchKey == MatchKey)
            {
                return &Group;
            }
        }
        return nullptr;
    }

    void AppendRowMessage(AssetMaterialBatch::FRow& Row, const FString& Message)
    {
        if (Message.IsEmpty())
        {
            return;
        }

        if (!Row.Message.IsEmpty())
        {
            Row.Message += TEXT(" ");
        }
        Row.Message += Message;
    }

    FString DescribeTextureAssignments(const TMap<FName, FAssetData>& TexturesByParameter)
    {
        if (TexturesByParameter.Num() == 0)
        {
            return TEXT("未绑定父材质纹理参数。");
        }

        TArray<FString> Parts;
        for (const TPair<FName, FAssetData>& Pair : TexturesByParameter)
        {
            Parts.Add(FString::Printf(TEXT("%s=%s"), *Pair.Key.ToString(), *Pair.Value.AssetName.ToString()));
        }
        Parts.Sort();
        return FString::Printf(TEXT("绑定参数：%s。"), *FString::Join(Parts, TEXT(", ")));
    }

    FAssetData ChoosePrimaryTexture(const AssetMaterialBatch::FTextureGroup& Group)
    {
        if (const FAssetData* BaseColorTexture = Group.TexturesByParameter.Find(TEXT("BaseColor")))
        {
            return *BaseColorTexture;
        }

        if (Group.PrimaryTexture.IsValid())
        {
            return Group.PrimaryTexture;
        }

        for (const TPair<FName, FAssetData>& Pair : Group.TexturesByParameter)
        {
            return Pair.Value;
        }

        return FAssetData();
    }

    int32 ScoreTextureGroupMatch(const FString& CandidateKey, const FString& TextureGroupKey)
    {
        if (CandidateKey.IsEmpty() || TextureGroupKey.IsEmpty())
        {
            return 0;
        }

        if (CandidateKey == TextureGroupKey)
        {
            return 1000;
        }

        if (CandidateKey.Len() < 3 || TextureGroupKey.Len() < 3)
        {
            return 0;
        }

        if (CandidateKey.Contains(TextureGroupKey) || TextureGroupKey.Contains(CandidateKey))
        {
            return 500 + FMath::Min(CandidateKey.Len(), TextureGroupKey.Len());
        }

        return 0;
    }

    const AssetMaterialBatch::FTextureGroup* FindBestTextureGroupForRow(
        const FAssetData& MeshAsset,
        const FAssetData& CurrentMaterialAsset,
        const TArray<AssetMaterialBatch::FTextureGroup>& Groups,
        FString& OutMatchSource)
    {
        if (Groups.Num() == 0)
        {
            return nullptr;
        }

        if (Groups.Num() == 1)
        {
            OutMatchSource = TEXT("单组纹理兜底匹配");
            return &Groups[0];
        }

        struct FCandidateKey
        {
            FString Label;
            FString Key;
            int32 Priority = 0;
        };

        TArray<FCandidateKey> CandidateKeys;
        if (CurrentMaterialAsset.IsValid())
        {
            CandidateKeys.Add({ TEXT("当前材质名"), FMaterialBatchProcessor::MakeMatchKey(CurrentMaterialAsset.AssetName.ToString()), 100 });
        }
        if (MeshAsset.IsValid())
        {
            CandidateKeys.Add({ TEXT("网格体名"), FMaterialBatchProcessor::MakeMatchKey(MeshAsset.AssetName.ToString()), 0 });
        }

        const AssetMaterialBatch::FTextureGroup* BestGroup = nullptr;
        FString BestSource;
        int32 BestScore = 0;

        for (const FCandidateKey& Candidate : CandidateKeys)
        {
            for (const AssetMaterialBatch::FTextureGroup& Group : Groups)
            {
                const int32 Score = ScoreTextureGroupMatch(Candidate.Key, Group.MatchKey);
                if (Score > 0 && Score + Candidate.Priority > BestScore)
                {
                    BestScore = Score + Candidate.Priority;
                    BestGroup = &Group;
                    BestSource = FString::Printf(TEXT("%s匹配"), *Candidate.Label);
                }
            }
        }

        OutMatchSource = BestSource;
        return BestGroup;
    }

    TArray<AssetMaterialBatch::FTextureGroup> MakeTextureGroups(const TArray<FAssetData>& SourceTextures)
    {
        TArray<AssetMaterialBatch::FTextureGroup> Groups;

        for (const FAssetData& TextureAsset : SourceTextures)
        {
            if (!TextureAsset.IsValid())
            {
                continue;
            }

            const FString MatchKey = FMaterialBatchProcessor::MakeMatchKey(TextureAsset.AssetName.ToString());
            const FName ParameterName = FMaterialBatchProcessor::DetectTextureParameter(TextureAsset.AssetName.ToString());

            AssetMaterialBatch::FTextureGroup* Group = FindGroupByKey(Groups, MatchKey);
            if (!Group)
            {
                AssetMaterialBatch::FTextureGroup NewGroup;
                NewGroup.MatchKey = MatchKey;
                Groups.Add(NewGroup);
                Group = &Groups.Last();
            }

            if (!Group->PrimaryTexture.IsValid() || ParameterName == TEXT("BaseColor"))
            {
                Group->PrimaryTexture = TextureAsset;
            }

            Group->TexturesByParameter.Add(ParameterName, TextureAsset);
        }

        return Groups;
    }

    UMaterialInstanceConstant* FindExistingMaterialInstance(const FString& PackagePath, const FString& AssetName)
    {
        return Cast<UMaterialInstanceConstant>(StaticLoadObject(UMaterialInstanceConstant::StaticClass(), nullptr, *GetObjectPath(PackagePath, AssetName)));
    }

    FString NormalizeContentBrowserFolderPath(const FString& InPath)
    {
        FString Path = InPath;
        Path.TrimStartAndEndInline();
        Path.RemoveFromEnd(TEXT("/"));

        if (Path.StartsWith(TEXT("/All/Game"), ESearchCase::IgnoreCase))
        {
            Path = TEXT("/Game") + Path.RightChop(9);
        }

        return Path;
    }

    FString ParameterKey(const FString& Name)
    {
        FString Key;
        Key.Reserve(Name.Len());
        for (const TCHAR Ch : Name)
        {
            if (FChar::IsAlnum(Ch))
            {
                Key.AppendChar(FChar::ToLower(Ch));
            }
        }
        return Key;
    }

    TArray<FName> GetParentTextureParameters(const UMaterialInterface* ParentMaterial)
    {
        TArray<FName> ParameterNames;
        if (!ParentMaterial)
        {
            return ParameterNames;
        }

        TArray<FMaterialParameterInfo> ParameterInfos;
        TArray<FGuid> ParameterIds;
        ParentMaterial->GetAllTextureParameterInfo(ParameterInfos, ParameterIds);

        for (const FMaterialParameterInfo& Info : ParameterInfos)
        {
            if (!Info.Name.IsNone())
            {
                ParameterNames.AddUnique(Info.Name);
            }
        }

        return ParameterNames;
    }

    TArray<FString> GetParameterAliases(const FName& DetectedParameter)
    {
        const FString Key = ParameterKey(DetectedParameter.ToString());

        if (Key == TEXT("basecolor"))
        {
            return { TEXT("basecolor"), TEXT("basecolour"), TEXT("albedo"), TEXT("diffuse"), TEXT("color"), TEXT("colour"), TEXT("maintex"), TEXT("maintexture"), TEXT("texture"), TEXT("tex") };
        }
        if (Key == TEXT("normal"))
        {
            return { TEXT("normal"), TEXT("normalmap"), TEXT("nrm") };
        }
        if (Key == TEXT("orm"))
        {
            return { TEXT("orm"), TEXT("mra"), TEXT("mask"), TEXT("packed"), TEXT("occlusionroughnessmetallic") };
        }
        if (Key == TEXT("roughness"))
        {
            return { TEXT("roughness"), TEXT("rough") };
        }
        if (Key == TEXT("metallic"))
        {
            return { TEXT("metallic"), TEXT("metalness"), TEXT("metal") };
        }
        if (Key == TEXT("specular"))
        {
            return { TEXT("specular"), TEXT("spec") };
        }
        if (Key == TEXT("opacity"))
        {
            return { TEXT("opacity"), TEXT("alpha"), TEXT("transparency") };
        }
        if (Key == TEXT("emissive"))
        {
            return { TEXT("emissive"), TEXT("emission"), TEXT("emissivecolor") };
        }

        return { Key };
    }

    int32 ScoreParentParameter(const FName& ParentParameter, const FName& DetectedParameter)
    {
        const FString ParentKey = ParameterKey(ParentParameter.ToString());
        const FString DetectedKey = ParameterKey(DetectedParameter.ToString());

        if (ParentKey.IsEmpty() || DetectedKey.IsEmpty())
        {
            return 0;
        }

        if (ParentKey == DetectedKey)
        {
            return 1000;
        }

        int32 BestScore = 0;
        for (const FString& Alias : GetParameterAliases(DetectedParameter))
        {
            if (Alias.IsEmpty())
            {
                continue;
            }

            if (ParentKey == Alias)
            {
                BestScore = FMath::Max(BestScore, 950);
            }
            else if (ParentKey.Contains(Alias))
            {
                BestScore = FMath::Max(BestScore, 800 - FMath::Min(ParentKey.Len() - Alias.Len(), 200));
            }
            else if (Alias.Contains(ParentKey) && ParentKey.Len() >= 3)
            {
                BestScore = FMath::Max(BestScore, 650);
            }
        }

        return BestScore;
    }

    FName ChooseFallbackTextureParameter(const TArray<FName>& ParentTextureParameters)
    {
        if (ParentTextureParameters.Num() == 0)
        {
            return NAME_None;
        }

        FName BestParameter = ParentTextureParameters[0];
        int32 BestScore = 0;

        for (const FName& ParentParameter : ParentTextureParameters)
        {
            const FString Key = ParameterKey(ParentParameter.ToString());
            int32 Score = 0;
            if (Key.Contains(TEXT("base")) || Key.Contains(TEXT("albedo")) || Key.Contains(TEXT("diffuse")))
            {
                Score = 100;
            }
            else if (Key.Contains(TEXT("color")) || Key.Contains(TEXT("colour")))
            {
                Score = 90;
            }
            else if (Key.Contains(TEXT("main")) || Key.Contains(TEXT("texture")) || Key == TEXT("tex"))
            {
                Score = 80;
            }

            if (Score > BestScore)
            {
                BestScore = Score;
                BestParameter = ParentParameter;
            }
        }

        return BestParameter;
    }

    FName FindParentTextureParameter(const FName& DetectedParameter, const TArray<FName>& ParentTextureParameters)
    {
        FName BestParameter = NAME_None;
        int32 BestScore = 0;

        for (const FName& ParentParameter : ParentTextureParameters)
        {
            const int32 Score = ScoreParentParameter(ParentParameter, DetectedParameter);
            if (Score > BestScore)
            {
                BestScore = Score;
                BestParameter = ParentParameter;
            }
        }

        return BestScore > 0 ? BestParameter : NAME_None;
    }

    TMap<FName, FAssetData> BuildTextureAssignmentsForParent(
        const TMap<FName, FAssetData>& TexturesByParameter,
        const FAssetData& PrimaryTexture,
        const UMaterialInterface* ParentMaterial)
    {
        const TArray<FName> ParentTextureParameters = GetParentTextureParameters(ParentMaterial);
        if (ParentTextureParameters.Num() == 0)
        {
            return TMap<FName, FAssetData>();
        }

        TMap<FName, FAssetData> Assignments;

        for (const TPair<FName, FAssetData>& Pair : TexturesByParameter)
        {
            if (!Pair.Value.IsValid())
            {
                continue;
            }

            const FName ParentParameter = FindParentTextureParameter(Pair.Key, ParentTextureParameters);
            if (!ParentParameter.IsNone())
            {
                Assignments.Add(ParentParameter, Pair.Value);
            }
        }

        if (Assignments.Num() == 0 && PrimaryTexture.IsValid())
        {
            const FName FallbackParameter = ChooseFallbackTextureParameter(ParentTextureParameters);
            if (!FallbackParameter.IsNone())
            {
                Assignments.Add(FallbackParameter, PrimaryTexture);
            }
        }

        return Assignments.Num() > 0 ? Assignments : TexturesByParameter;
    }
}

void FMaterialBatchProcessor::GetContentBrowserSelection(TArray<FAssetData>& OutAssets)
{
    OutAssets.Reset();

    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
    ContentBrowserModule.Get().GetSelectedAssets(OutAssets);
}

void FMaterialBatchProcessor::GetContentBrowserSelectedFolders(TArray<FString>& OutFolders)
{
    OutFolders.Reset();

    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

    TArray<FString> SelectedFolders;
    TArray<FString> PathViewFolders;
    TArray<FAssetData> SelectedAssets;
    ContentBrowserModule.Get().GetSelectedFolders(SelectedFolders);
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    if (SelectedAssets.Num() == 0)
    {
        ContentBrowserModule.Get().GetSelectedPathViewFolders(PathViewFolders);
        SelectedFolders.Append(PathViewFolders);
    }

    for (const FString& Folder : SelectedFolders)
    {
        const FString NormalizedFolder = NormalizeContentBrowserFolderPath(Folder);
        if (!NormalizedFolder.IsEmpty())
        {
            OutFolders.AddUnique(NormalizedFolder);
        }
    }
}

void FMaterialBatchProcessor::SplitAssets(const TArray<FAssetData>& Assets, TArray<FAssetData>& OutMeshes, TArray<FAssetData>& OutTextures, TArray<FAssetData>& OutMaterialInterfaces)
{
    OutMeshes.Reset();
    OutTextures.Reset();
    OutMaterialInterfaces.Reset();

    for (const FAssetData& AssetData : Assets)
    {
        if (IsClassPath(AssetData, UStaticMesh::StaticClass()))
        {
            OutMeshes.Add(AssetData);
        }
        else if (IsTextureAsset(AssetData))
        {
            OutTextures.Add(AssetData);
        }
        else if (IsMaterialInterfaceAsset(AssetData))
        {
            OutMaterialInterfaces.Add(AssetData);
        }
    }
}

UMaterialInterface* FMaterialBatchProcessor::FindFirstMaterialInterface(const TArray<FAssetData>& Assets)
{
    for (const FAssetData& AssetData : Assets)
    {
        if (IsMaterialInterfaceAsset(AssetData))
        {
            return Cast<UMaterialInterface>(AssetData.GetAsset());
        }
    }

    return nullptr;
}

void FMaterialBatchProcessor::ScanTexturesInFolder(const FString& FolderPath, TArray<FAssetData>& OutTextures)
{
    OutTextures.Reset();

    FString CleanFolderPath = FolderPath;
    CleanFolderPath.TrimStartAndEndInline();
    CleanFolderPath.RemoveFromEnd(TEXT("/"));

    if (CleanFolderPath.IsEmpty() || CleanFolderPath == TEXT("/Game") || !CleanFolderPath.StartsWith(TEXT("/Game/")))
    {
        return;
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

    FARFilter Filter;
    Filter.PackagePaths.Add(*CleanFolderPath);
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(UTexture2D::StaticClass()->GetClassPathName());
    AssetRegistryModule.Get().GetAssets(Filter, OutTextures);
}

FString FMaterialBatchProcessor::MakeMatchKey(const FString& AssetName)
{
    FString Key = AssetName;
    RemoveKnownPrefix(Key);
    RemoveKnownTextureSuffix(Key);

    FString Clean;
    Clean.Reserve(Key.Len());
    for (const TCHAR Ch : Key)
    {
        if (FChar::IsAlnum(Ch))
        {
            Clean.AppendChar(FChar::ToLower(Ch));
        }
    }

    return Clean;
}

FString FMaterialBatchProcessor::DeriveMaterialInstanceName(const FString& TextureName)
{
    FString Name = TextureName;

    if (Name.StartsWith(TEXT("T_"), ESearchCase::IgnoreCase))
    {
        Name = TEXT("M_") + Name.RightChop(2);
    }
    else if (Name.StartsWith(TEXT("T"), ESearchCase::IgnoreCase) && Name.Len() > 1)
    {
        Name = TEXT("M") + Name.RightChop(1);
    }
    else if (!Name.StartsWith(TEXT("M_"), ESearchCase::IgnoreCase) && !Name.StartsWith(TEXT("MI_"), ESearchCase::IgnoreCase))
    {
        Name = TEXT("M_") + Name;
    }

    return SanitizeAssetName(Name);
}

FName FMaterialBatchProcessor::DetectTextureParameter(const FString& TextureName)
{
    for (const FTextureRule& Rule : GetTextureRules())
    {
        for (const FString& Suffix : Rule.Suffixes)
        {
            if (TextureName.EndsWith(Suffix, ESearchCase::IgnoreCase))
            {
                return Rule.ParameterName;
            }
        }
    }

    return TEXT("BaseColor");
}

void FMaterialBatchProcessor::BuildPreviewRows(
    const TArray<FAssetData>& TargetMeshes,
    const TArray<FAssetData>& SourceTextures,
    const AssetMaterialBatch::FSettings& Settings,
    TArray<AssetMaterialBatch::FRow>& OutRows)
{
    OutRows.Reset();

    const TArray<AssetMaterialBatch::FTextureGroup> TextureGroups = MakeTextureGroups(SourceTextures);

    if (TargetMeshes.Num() == 0)
    {
        for (const AssetMaterialBatch::FTextureGroup& Group : TextureGroups)
        {
            AssetMaterialBatch::FRow Row;
            Row.OutputPath = Settings.OutputFolderPath;
            Row.PrimaryTextureAsset = ChoosePrimaryTexture(Group);
            Row.TexturesByParameter = Group.TexturesByParameter;
            Row.TexturesByParameter = BuildTextureAssignmentsForParent(Row.TexturesByParameter, Row.PrimaryTextureAsset, Settings.ParentMaterial.Get());
            Row.NewMaterialName = Row.PrimaryTextureAsset.IsValid()
                ? DeriveMaterialInstanceName(Row.PrimaryTextureAsset.AssetName.ToString())
                : TEXT("M_NewMaterialInstance");

            if (!Settings.ParentMaterial.IsValid())
            {
                Row.Status = AssetMaterialBatch::ERowStatus::Warning;
                AppendRowMessage(Row, TEXT("未设置父材质。"));
            }
            else if (!Row.PrimaryTextureAsset.IsValid())
            {
                Row.Status = AssetMaterialBatch::ERowStatus::Warning;
                AppendRowMessage(Row, TEXT("没有可用纹理。"));
            }
            else if (Row.TexturesByParameter.Num() == 0)
            {
                Row.Status = AssetMaterialBatch::ERowStatus::Warning;
                AppendRowMessage(Row, TEXT("父材质没有可用纹理参数，无法自动绑定纹理。"));
            }
            else
            {
                Row.Status = AssetMaterialBatch::ERowStatus::Ready;
                AppendRowMessage(Row, TEXT("就绪：将根据纹理创建材质实例。"));
                AppendRowMessage(Row, DescribeTextureAssignments(Row.TexturesByParameter));
            }

            OutRows.Add(Row);
        }
        return;
    }

    for (const FAssetData& MeshAsset : TargetMeshes)
    {
        AssetMaterialBatch::FRow Row;
        Row.MeshAsset = MeshAsset;
        Row.OutputPath = Settings.OutputFolderPath;

        UStaticMesh* Mesh = Cast<UStaticMesh>(MeshAsset.GetAsset());
        if (Mesh)
        {
            const int32 SlotCount = Mesh->GetStaticMaterials().Num();
            Row.MaterialSlotIndex = 0;
            if (SlotCount > 0 && Mesh->GetMaterial(0))
            {
                Row.CurrentMaterialAsset = FAssetData(Mesh->GetMaterial(0));
            }
            if (SlotCount > 1)
            {
                Row.Status = AssetMaterialBatch::ERowStatus::Warning;
                AppendRowMessage(Row, FString::Printf(TEXT("网格体有 %d 个材质槽；此行默认使用 0 号槽。"), SlotCount));
            }
        }

        FString MatchSource;
        const AssetMaterialBatch::FTextureGroup* Group = FindBestTextureGroupForRow(MeshAsset, Row.CurrentMaterialAsset, TextureGroups, MatchSource);
        if (Group)
        {
            Row.PrimaryTextureAsset = ChoosePrimaryTexture(*Group);
            Row.TexturesByParameter = Group->TexturesByParameter;
            Row.TexturesByParameter = BuildTextureAssignmentsForParent(Row.TexturesByParameter, Row.PrimaryTextureAsset, Settings.ParentMaterial.Get());
            Row.NewMaterialName = Row.PrimaryTextureAsset.IsValid()
                ? DeriveMaterialInstanceName(Row.PrimaryTextureAsset.AssetName.ToString())
                : FString::Printf(TEXT("M_%s"), *MeshAsset.AssetName.ToString());
            AppendRowMessage(Row, MatchSource.IsEmpty() ? TEXT("已匹配纹理。") : MatchSource + TEXT("。"));
        }
        else
        {
            Row.NewMaterialName = SanitizeAssetName(FString::Printf(TEXT("M_%s"), *MeshAsset.AssetName.ToString()));
        }

        if (!Settings.ParentMaterial.IsValid())
        {
            Row.Status = AssetMaterialBatch::ERowStatus::Warning;
            AppendRowMessage(Row, TEXT("未设置父材质。"));
        }
        else if (!Row.PrimaryTextureAsset.IsValid())
        {
            Row.Status = AssetMaterialBatch::ERowStatus::Warning;
            AppendRowMessage(Row, TEXT("未找到匹配纹理。"));
        }
        else if (Row.TexturesByParameter.Num() == 0)
        {
            Row.Status = AssetMaterialBatch::ERowStatus::Warning;
            AppendRowMessage(Row, TEXT("父材质没有可用纹理参数，无法自动绑定纹理。"));
        }
        else
        {
            if (Row.Status != AssetMaterialBatch::ERowStatus::Warning)
            {
                Row.Status = AssetMaterialBatch::ERowStatus::Ready;
                AppendRowMessage(Row, TEXT("就绪。"));
            }
            AppendRowMessage(Row, DescribeTextureAssignments(Row.TexturesByParameter));
        }

        OutRows.Add(Row);
    }
}

void FMaterialBatchProcessor::ExecuteRows(const AssetMaterialBatch::FSettings& Settings, TArray<AssetMaterialBatch::FRow>& Rows)
{
    if (!Settings.ParentMaterial.IsValid())
    {
        for (AssetMaterialBatch::FRow& Row : Rows)
        {
            Row.Status = AssetMaterialBatch::ERowStatus::Error;
            Row.Message = TEXT("必须指定父材质。");
        }
        return;
    }

    FScopedTransaction Transaction(LOCTEXT("ExecuteTransaction", "执行资产材质批处理"));
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
    IAssetTools& AssetTools = AssetToolsModule.Get();

    FScopedSlowTask SlowTask(static_cast<float>(Rows.Num()), LOCTEXT("ExecutingRows", "正在处理材质批处理任务..."));
    SlowTask.MakeDialog(false);

    for (AssetMaterialBatch::FRow& Row : Rows)
    {
        SlowTask.EnterProgressFrame(1.0f);

        if (!Row.bEnabled)
        {
            Row.Status = AssetMaterialBatch::ERowStatus::Skipped;
            Row.Message = TEXT("此行已禁用。");
            continue;
        }

        if (!Row.OutputPath.StartsWith(TEXT("/Game")))
        {
            Row.Status = AssetMaterialBatch::ERowStatus::Error;
            Row.Message = TEXT("输出路径必须以 /Game 开头。");
            continue;
        }

        Row.TexturesByParameter = BuildTextureAssignmentsForParent(Row.TexturesByParameter, Row.PrimaryTextureAsset, Settings.ParentMaterial.Get());

        if (!Row.PrimaryTextureAsset.IsValid() && Row.TexturesByParameter.Num() == 0)
        {
            Row.Status = AssetMaterialBatch::ERowStatus::Warning;
            Row.Message = TEXT("未匹配纹理，已跳过。");
            continue;
        }

        if (Row.PrimaryTextureAsset.IsValid() && Row.TexturesByParameter.Num() == 0)
        {
            Row.Status = AssetMaterialBatch::ERowStatus::Warning;
            Row.Message = TEXT("父材质没有可用纹理参数，未创建或回填材质实例。");
            continue;
        }

        FString AssetName = SanitizeAssetName(Row.NewMaterialName.IsEmpty() ? TEXT("M_NewMaterialInstance") : Row.NewMaterialName);
        FString PackagePath = Row.OutputPath;
        PackagePath.RemoveFromEnd(TEXT("/"));

        UMaterialInstanceConstant* MaterialInstance = FindExistingMaterialInstance(PackagePath, AssetName);
        bool bCreatedNewAsset = false;

        if (MaterialInstance)
        {
            if (Settings.ConflictPolicy == AssetMaterialBatch::EConflictPolicy::Skip)
            {
                Row.Status = AssetMaterialBatch::ERowStatus::Skipped;
                Row.Message = FString::Printf(TEXT("资产已存在：%s"), *AssetName);
                continue;
            }

            if (Settings.ConflictPolicy == AssetMaterialBatch::EConflictPolicy::AutoRename)
            {
                FString UniquePackageName;
                FString UniqueAssetName;
                AssetTools.CreateUniqueAssetName(JoinPackageAsset(PackagePath, AssetName), TEXT(""), UniquePackageName, UniqueAssetName);
                PackagePath = FPackageName::GetLongPackagePath(UniquePackageName);
                AssetName = UniqueAssetName;
                Row.NewMaterialName = AssetName;
                MaterialInstance = nullptr;
            }
        }

        if (!MaterialInstance)
        {
            UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
            Factory->InitialParent = Settings.ParentMaterial.Get();

            UObject* NewAsset = AssetTools.CreateAsset(AssetName, PackagePath, UMaterialInstanceConstant::StaticClass(), Factory);
            MaterialInstance = Cast<UMaterialInstanceConstant>(NewAsset);
            bCreatedNewAsset = MaterialInstance != nullptr;
        }

        if (!MaterialInstance)
        {
            Row.Status = AssetMaterialBatch::ERowStatus::Error;
            Row.Message = TEXT("创建材质实例失败。");
            continue;
        }

        MaterialInstance->Modify();
        MaterialInstance->SetParentEditorOnly(Settings.ParentMaterial.Get());

        int32 TextureCount = 0;
        for (const TPair<FName, FAssetData>& Pair : Row.TexturesByParameter)
        {
            UTexture* Texture = Cast<UTexture>(Pair.Value.GetAsset());
            if (!Texture)
            {
                continue;
            }

            MaterialInstance->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(Pair.Key), Texture);
            ++TextureCount;
        }

        MaterialInstance->PostEditChange();
        MaterialInstance->MarkPackageDirty();

        if (Settings.bAssignToStaticMesh && Row.MeshAsset.IsValid())
        {
            UStaticMesh* Mesh = Cast<UStaticMesh>(Row.MeshAsset.GetAsset());
            if (!Mesh)
            {
                Row.Status = AssetMaterialBatch::ERowStatus::Warning;
                Row.Message = TEXT("材质实例已创建，但静态网格体加载失败。");
                continue;
            }

            if (!Mesh->GetStaticMaterials().IsValidIndex(Row.MaterialSlotIndex))
            {
                Row.Status = AssetMaterialBatch::ERowStatus::Warning;
                Row.Message = TEXT("材质实例已创建，但材质槽索引无效。");
                continue;
            }

            Mesh->Modify();
            Mesh->SetMaterial(Row.MaterialSlotIndex, MaterialInstance);
            Mesh->PostEditChange();
            Mesh->MarkPackageDirty();
        }

        Row.Status = AssetMaterialBatch::ERowStatus::Done;
        Row.Message = FString::Printf(TEXT("%s %s。已设置 %d 个纹理参数。%s"), bCreatedNewAsset ? TEXT("已创建") : TEXT("已更新"), *AssetName, TextureCount, *DescribeTextureAssignments(Row.TexturesByParameter));
    }
}

FString FMaterialBatchProcessor::SanitizeAssetName(const FString& InName)
{
    FString Clean;
    Clean.Reserve(InName.Len());

    for (const TCHAR Ch : InName)
    {
        if (FChar::IsAlnum(Ch) || Ch == TEXT('_'))
        {
            Clean.AppendChar(Ch);
        }
        else
        {
            Clean.AppendChar(TEXT('_'));
        }
    }

    Clean.TrimStartAndEndInline();
    while (Clean.Contains(TEXT("__")))
    {
        Clean.ReplaceInline(TEXT("__"), TEXT("_"));
    }

    if (Clean.IsEmpty())
    {
        Clean = TEXT("M_NewMaterialInstance");
    }

    if (FChar::IsDigit(Clean[0]))
    {
        Clean = TEXT("M_") + Clean;
    }

    return Clean;
}

#undef LOCTEXT_NAMESPACE



