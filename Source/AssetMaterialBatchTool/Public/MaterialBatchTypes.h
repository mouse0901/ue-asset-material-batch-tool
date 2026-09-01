#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "Materials/MaterialInterface.h"

class UMaterialInstanceConstant;
class UStaticMesh;
class UTexture;

namespace AssetMaterialBatch
{
    enum class EConflictPolicy : uint8
    {
        Skip,
        AutoRename,
        UpdateExisting
    };

    enum class ERowStatus : uint8
    {
        Pending,
        Ready,
        Warning,
        Error,
        Done,
        Skipped
    };

    struct FSettings
    {
        TWeakObjectPtr<UMaterialInterface> ParentMaterial;
        FString TextureFolderPath = TEXT("/Game");
        FString OutputFolderPath = TEXT("/Game/Generated/MaterialInstances");
        EConflictPolicy ConflictPolicy = EConflictPolicy::AutoRename;
        bool bAssignToStaticMesh = true;
    };

    struct FTextureGroup
    {
        FString MatchKey;
        FAssetData PrimaryTexture;
        TMap<FName, FAssetData> TexturesByParameter;
    };

    struct FRow
    {
        bool bEnabled = true;
        FAssetData MeshAsset;
        int32 MaterialSlotIndex = 0;
        FAssetData CurrentMaterialAsset;
        FAssetData PrimaryTextureAsset;
        TMap<FName, FAssetData> TexturesByParameter;
        FString NewMaterialName;
        FString OutputPath;
        ERowStatus Status = ERowStatus::Pending;
        FString Message;
    };
}

