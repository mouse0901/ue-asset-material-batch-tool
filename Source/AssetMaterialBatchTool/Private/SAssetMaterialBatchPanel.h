#pragma once

#include "CoreMinimal.h"
#include "MaterialBatchTypes.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class STableViewBase;
class ITableRow;
template<typename ItemType> class SListView;

class SAssetMaterialBatchPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAssetMaterialBatchPanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FReply OnLoadSelectionClicked();
    FReply OnScanTextureFolderClicked();
    FReply OnBuildPreviewClicked();
    FReply OnExecuteClicked();
    FReply OnClearClicked();

    void RefreshSelectionFromContentBrowser(bool bUpdateStatusText);
    void RebuildRows(const TArray<AssetMaterialBatch::FRow>& SourceRows);
    void UpdateSummary();
    bool ValidateBeforePreview(FString& OutMessage) const;
    bool CanBuildPreview() const;
    bool CanExecuteRows() const;
    bool IsUnsafeTextureFolder(const FString& FolderPath) const;
    bool HasAnyInputAsset() const;

    TSharedRef<SWidget> BuildHeader();
    TSharedRef<SWidget> BuildAssetPoolPanel();
    TSharedRef<SWidget> BuildPreviewPanel();
    TSharedRef<SWidget> BuildDetailPanel();
    TSharedRef<SWidget> BuildNextStepPanel();
    TSharedRef<SWidget> BuildEmptyPreviewPanel();
    TSharedRef<SWidget> MakeSectionTitle(const FText& Title, const FText& SubTitle = FText::GetEmpty()) const;
    TSharedRef<SWidget> MakeMetricLine(const FText& Label, TFunction<FText()> ValueGetter) const;
    TSharedRef<SWidget> MakeDetailLine(const FText& Label, TFunction<FText()> ValueGetter) const;
    TSharedRef<SWidget> MakeChecklistLine(const FText& Label, TFunction<FText()> ValueGetter, TFunction<bool()> IsGoodGetter) const;

    FString GetParentMaterialPath() const;
    void OnParentMaterialChanged(const FAssetData& AssetData);
    void OnConflictPolicyChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
    FText GetConflictPolicyLabel() const;
    void OnRowSelectionChanged(TSharedPtr<AssetMaterialBatch::FRow> RowItem, ESelectInfo::Type SelectInfo);
    FReply OnPrimaryActionClicked();

    FText GetCompactSummaryText() const;
    FText GetNextStepTitle() const;
    FText GetNextStepText() const;
    FText GetPrimaryActionText() const;
    FText GetIssueText() const;
    EVisibility GetIssueVisibility() const;
    EVisibility GetPreviewTableVisibility() const;
    EVisibility GetEmptyPreviewVisibility() const;
    TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<AssetMaterialBatch::FRow> RowItem, const TSharedRef<STableViewBase>& OwnerTable);

private:
    AssetMaterialBatch::FSettings Settings;

    TArray<FAssetData> LastSelection;
    TArray<FAssetData> TargetMeshes;
    TArray<FAssetData> SelectedTextures;
    TArray<FAssetData> ScannedTextures;
    TArray<FAssetData> SelectedMaterials;
    TArray<FString> SelectedFolders;

    TArray<TSharedPtr<AssetMaterialBatch::FRow>> Rows;
    TSharedPtr<AssetMaterialBatch::FRow> SelectedRow;
    TSharedPtr<SListView<TSharedPtr<AssetMaterialBatch::FRow>>> RowListView;
    TSharedPtr<SEditableTextBox> TextureFolderTextBox;
    TSharedPtr<SEditableTextBox> OutputFolderTextBox;

    TArray<TSharedPtr<FString>> ConflictOptions;
    TSharedPtr<FString> SelectedConflictOption;
    FString SummaryText;
    FString IssueText;
};
