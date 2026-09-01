#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "MaterialBatchTypes.h"

class UMaterialInterface;

class FMaterialBatchProcessor
{
public:
    static void GetContentBrowserSelection(TArray<FAssetData>& OutAssets);
    static void GetContentBrowserSelectedFolders(TArray<FString>& OutFolders);
    static void SplitAssets(const TArray<FAssetData>& Assets, TArray<FAssetData>& OutMeshes, TArray<FAssetData>& OutTextures, TArray<FAssetData>& OutMaterialInterfaces);
    static UMaterialInterface* FindFirstMaterialInterface(const TArray<FAssetData>& Assets);
    static void ScanTexturesInFolder(const FString& FolderPath, TArray<FAssetData>& OutTextures);

    static FString MakeMatchKey(const FString& AssetName);
    static FString DeriveMaterialInstanceName(const FString& TextureName);
    static FName DetectTextureParameter(const FString& TextureName);

    static void BuildPreviewRows(
        const TArray<FAssetData>& TargetMeshes,
        const TArray<FAssetData>& SourceTextures,
        const AssetMaterialBatch::FSettings& Settings,
        TArray<AssetMaterialBatch::FRow>& OutRows);

    static void ExecuteRows(
        const AssetMaterialBatch::FSettings& Settings,
        TArray<AssetMaterialBatch::FRow>& Rows);

private:
    static FString SanitizeAssetName(const FString& InName);
};

