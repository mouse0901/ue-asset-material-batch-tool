#include "SAssetMaterialBatchPanel.h"

#include "MaterialBatchProcessor.h"
#include "Materials/MaterialInterface.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SAssetMaterialBatchPanel"

namespace
{
    FString StatusToString(AssetMaterialBatch::ERowStatus Status)
    {
        switch (Status)
        {
        case AssetMaterialBatch::ERowStatus::Ready: return TEXT("就绪");
        case AssetMaterialBatch::ERowStatus::Warning: return TEXT("警告");
        case AssetMaterialBatch::ERowStatus::Error: return TEXT("错误");
        case AssetMaterialBatch::ERowStatus::Done: return TEXT("完成");
        case AssetMaterialBatch::ERowStatus::Skipped: return TEXT("已跳过");
        case AssetMaterialBatch::ERowStatus::Pending:
        default: return TEXT("待处理");
        }
    }

    FSlateColor StatusColor(AssetMaterialBatch::ERowStatus Status)
    {
        switch (Status)
        {
        case AssetMaterialBatch::ERowStatus::Ready: return FLinearColor(0.48f, 0.86f, 0.56f);
        case AssetMaterialBatch::ERowStatus::Warning: return FLinearColor(1.0f, 0.72f, 0.28f);
        case AssetMaterialBatch::ERowStatus::Error: return FLinearColor(1.0f, 0.36f, 0.32f);
        case AssetMaterialBatch::ERowStatus::Done: return FLinearColor(0.35f, 0.75f, 1.0f);
        case AssetMaterialBatch::ERowStatus::Skipped: return FLinearColor(0.70f, 0.70f, 0.70f);
        case AssetMaterialBatch::ERowStatus::Pending:
        default: return FLinearColor::White;
        }
    }

    FString AssetLabel(const FAssetData& AssetData)
    {
        return AssetData.IsValid() ? AssetData.AssetName.ToString() : TEXT("未设置");
    }

    FString TextureParameterSummary(const TMap<FName, FAssetData>& TexturesByParameter)
    {
        if (TexturesByParameter.Num() == 0)
        {
            return TEXT("未绑定纹理参数");
        }

        TArray<FString> Parts;
        for (const TPair<FName, FAssetData>& Pair : TexturesByParameter)
        {
            Parts.Add(FString::Printf(TEXT("%s=%s"), *Pair.Key.ToString(), *AssetLabel(Pair.Value)));
        }
        Parts.Sort();
        return FString::Join(Parts, TEXT(", "));
    }

    TSharedRef<STextBlock> SmallMutedText(const FText& Text)
    {
        return SNew(STextBlock)
            .Text(Text)
            .ColorAndOpacity(FSlateColor(FLinearColor(0.66f, 0.66f, 0.66f)))
            .AutoWrapText(true);
    }

    class SMaterialBatchTableRow : public STableRow<TSharedPtr<AssetMaterialBatch::FRow>>
    {
    public:
        SLATE_BEGIN_ARGS(SMaterialBatchTableRow) {}
            SLATE_ARGUMENT(TSharedPtr<AssetMaterialBatch::FRow>, RowItem)
        SLATE_END_ARGS()

        void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
        {
            RowItem = InArgs._RowItem;
            STableRow<TSharedPtr<AssetMaterialBatch::FRow>>::Construct(
                STableRow<TSharedPtr<AssetMaterialBatch::FRow>>::FArguments()
                .Padding(FMargin(3.0f, 4.0f))
                [
                    BuildRowContent()
                ],
                OwnerTableView);
        }

    private:
        TSharedRef<SWidget> BuildLabelValue(const FText& Label, TFunction<FText()> ValueGetter) const
        {
            return SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SmallMutedText(Label)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text_Lambda([ValueGetter]() { return ValueGetter(); })
                    .AutoWrapText(true)
                ];
        }

        TSharedRef<SWidget> BuildRowContent() const
        {
            if (!RowItem.IsValid())
            {
                return SNew(STextBlock).Text(LOCTEXT("InvalidRow", "无效预览行"));
            }

            return SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("Brushes.Recessed"))
                .Padding(FMargin(9.0f, 7.0f))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 2.0f, 8.0f, 0.0f)
                    [
                        SNew(SCheckBox)
                        .IsChecked_Lambda([Item = RowItem]()
                        {
                            return Item->bEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                        })
                        .OnCheckStateChanged_Lambda([Item = RowItem](ECheckBoxState NewState)
                        {
                            Item->bEnabled = NewState == ECheckBoxState::Checked;
                        })
                    ]
                    + SHorizontalBox::Slot().FillWidth(1.0f)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                            [
                                SNew(STextBlock)
                                .Text_Lambda([Item = RowItem]() { return FText::FromString(StatusToString(Item->Status)); })
                                .ColorAndOpacity_Lambda([Item = RowItem]() { return StatusColor(Item->Status); })
                            ]
                            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Text_Lambda([Item = RowItem]() { return FText::FromString(AssetLabel(Item->MeshAsset)); })
                                .ToolTipText_Lambda([Item = RowItem]() { return FText::FromName(Item->MeshAsset.PackageName); })
                                .AutoWrapText(true)
                            ]
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                                [
                                    SmallMutedText(LOCTEXT("CardSlotLabel", "槽"))
                                ]
                                + SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f, 0.0f, 0.0f)
                                [
                                    SNew(SBox)
                                    .WidthOverride(48.0f)
                                    [
                                        SNew(SEditableTextBox)
                                        .Text_Lambda([Item = RowItem]() { return FText::AsNumber(Item->MaterialSlotIndex); })
                                        .OnTextCommitted_Lambda([Item = RowItem](const FText& Text, ETextCommit::Type)
                                        {
                                            Item->MaterialSlotIndex = FMath::Max(0, FCString::Atoi(*Text.ToString()));
                                        })
                                    ]
                                ]
                            ]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1.0f)
                            [
                                BuildLabelValue(LOCTEXT("CardCurrentMaterial", "当前材质"), [Item = RowItem]()
                                {
                                    return FText::FromString(AssetLabel(Item->CurrentMaterialAsset));
                                })
                            ]
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 12.0f, 8.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("CardTo", "替换为"))
                                .ColorAndOpacity(FSlateColor(FLinearColor(0.66f, 0.66f, 0.66f)))
                            ]
                            + SHorizontalBox::Slot().FillWidth(1.0f)
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot().AutoHeight()
                                [
                                    SmallMutedText(LOCTEXT("CardNewMaterial", "新材质实例"))
                                ]
                                + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
                                [
                                    SNew(SEditableTextBox)
                                    .MinDesiredWidth(180.0f)
                                    .Text_Lambda([Item = RowItem]() { return FText::FromString(Item->NewMaterialName); })
                                    .OnTextCommitted_Lambda([Item = RowItem](const FText& Text, ETextCommit::Type)
                                    {
                                        Item->NewMaterialName = Text.ToString();
                                    })
                                ]
                            ]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
                        [
                            BuildLabelValue(LOCTEXT("CardTextureBinding", "纹理绑定"), [Item = RowItem]()
                            {
                                return FText::FromString(TextureParameterSummary(Item->TexturesByParameter));
                            })
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Text_Lambda([Item = RowItem]() { return FText::FromString(Item->Message.IsEmpty() ? TEXT("-") : Item->Message); })
                            .ColorAndOpacity_Lambda([Item = RowItem]() { return StatusColor(Item->Status); })
                            .AutoWrapText(true)
                        ]
                    ]
                ];
        }

        TSharedPtr<AssetMaterialBatch::FRow> RowItem;
    };
}

void SAssetMaterialBatchPanel::Construct(const FArguments& InArgs)
{
    Settings.TextureFolderPath = TEXT("");
    Settings.OutputFolderPath = TEXT("/Game/Generated/MaterialInstances");
    Settings.ConflictPolicy = AssetMaterialBatch::EConflictPolicy::AutoRename;
    Settings.bAssignToStaticMesh = true;

    ConflictOptions.Add(MakeShared<FString>(TEXT("自动改名")));
    ConflictOptions.Add(MakeShared<FString>(TEXT("跳过已存在")));
    ConflictOptions.Add(MakeShared<FString>(TEXT("更新已存在")));
    SelectedConflictOption = ConflictOptions[0];

    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [
            BuildHeader()
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(8.0f, 4.0f, 8.0f, 0.0f)
        [
            BuildNextStepPanel()
        ]
        + SVerticalBox::Slot().FillHeight(1.0f).Padding(8.0f, 4.0f)
        [
            SNew(SSplitter)
            .Orientation(Orient_Horizontal)
            + SSplitter::Slot().Value(0.25f)
            [
                BuildAssetPoolPanel()
            ]
            + SSplitter::Slot().Value(0.49f)
            [
                BuildPreviewPanel()
            ]
            + SSplitter::Slot().Value(0.26f)
            [
                BuildDetailPanel()
            ]
        ]
    ];

    RefreshSelectionFromContentBrowser(false);
    UpdateSummary();
}

TSharedRef<SWidget> SAssetMaterialBatchPanel::BuildHeader()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
        .Padding(FMargin(10.0f, 7.0f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("ToolTitle", "资产材质批处理工具"))
                    .TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text(this, &SAssetMaterialBatchPanel::GetCompactSummaryText)
                    .ColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.72f, 0.72f)))
                ]
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .OnClicked(this, &SAssetMaterialBatchPanel::OnLoadSelectionClicked)
                [ SNew(STextBlock).Text(LOCTEXT("LoadSelection", "载入当前选择")) ]
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f, 0.0f, 0.0f)
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .OnClicked(this, &SAssetMaterialBatchPanel::OnClearClicked)
                [ SNew(STextBlock).Text(LOCTEXT("Clear", "清空")) ]
            ]
        ];
}

TSharedRef<SWidget> SAssetMaterialBatchPanel::BuildNextStepPanel()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("Brushes.Recessed"))
        .Padding(FMargin(10.0f, 8.0f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(this, &SAssetMaterialBatchPanel::GetNextStepTitle)
                    .ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.38f)))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text(this, &SAssetMaterialBatchPanel::GetNextStepText)
                    .ColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.78f, 0.78f)))
                    .AutoWrapText(true)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Visibility(this, &SAssetMaterialBatchPanel::GetIssueVisibility)
                    .Text(this, &SAssetMaterialBatchPanel::GetIssueText)
                    .ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.72f, 0.28f)))
                    .AutoWrapText(true)
                ]
            ]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "PrimaryButton")
                .IsEnabled_Lambda([this]() { return CanBuildPreview() || CanExecuteRows() || !HasAnyInputAsset(); })
                .OnClicked(this, &SAssetMaterialBatchPanel::OnPrimaryActionClicked)
                [ SNew(STextBlock).Text(this, &SAssetMaterialBatchPanel::GetPrimaryActionText) ]
            ]
        ];
}

TSharedRef<SWidget> SAssetMaterialBatchPanel::BuildAssetPoolPanel()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
        .Padding(10.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [ MakeSectionTitle(LOCTEXT("InputTitle", "输入"), LOCTEXT("InputSub", "先把模型、纹理和父材质放齐。")) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
            [ MakeChecklistLine(LOCTEXT("MeshCount", "网格体"), [this]() { return FText::AsNumber(TargetMeshes.Num()); }, [this]() { return TargetMeshes.Num() > 0; }) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
            [ MakeChecklistLine(LOCTEXT("ParentMaterial", "父材质"), [this]() { return FText::FromString(Settings.ParentMaterial.IsValid() ? Settings.ParentMaterial->GetName() : TEXT("未设置")); }, [this]() { return Settings.ParentMaterial.IsValid(); }) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
            [ MakeChecklistLine(LOCTEXT("TextureSource", "纹理来源"), [this]() { return FText::FromString(FString::Printf(TEXT("已选 %d / 扫描 %d"), SelectedTextures.Num(), ScannedTextures.Num())); }, [this]() { return SelectedTextures.Num() + ScannedTextures.Num() > 0; }) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 8.0f)
            [ SNew(SSeparator) ]
            + SVerticalBox::Slot().AutoHeight()
            [ SNew(STextBlock).Text(LOCTEXT("ParentMaterialPicker", "父材质")) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 8.0f)
            [
                SNew(SObjectPropertyEntryBox)
                .AllowedClass(UMaterialInterface::StaticClass())
                .ObjectPath(this, &SAssetMaterialBatchPanel::GetParentMaterialPath)
                .OnObjectChanged(this, &SAssetMaterialBatchPanel::OnParentMaterialChanged)
            ]
            + SVerticalBox::Slot().AutoHeight()
            [ SNew(STextBlock).Text(LOCTEXT("TextureFolder", "扫描纹理目录")) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 2.0f)
            [
                SAssignNew(TextureFolderTextBox, SEditableTextBox)
                .HintText(LOCTEXT("TextureFolderHint", "/Game/具体纹理目录，不能直接填 /Game"))
                .Text(FText::FromString(Settings.TextureFolderPath))
                .OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type) { Settings.TextureFolderPath = Text.ToString(); })
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 10.0f)
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .OnClicked(this, &SAssetMaterialBatchPanel::OnScanTextureFolderClicked)
                [ SNew(STextBlock).Text(LOCTEXT("ScanTextures", "扫描这个目录")) ]
            ]
            + SVerticalBox::Slot().AutoHeight()
            [ SNew(STextBlock).Text(LOCTEXT("OutputFolder", "输出目录")) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 8.0f)
            [
                SAssignNew(OutputFolderTextBox, SEditableTextBox)
                .Text(FText::FromString(Settings.OutputFolderPath))
                .OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type) { Settings.OutputFolderPath = Text.ToString(); })
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 8.0f)
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&ConflictOptions)
                .InitiallySelectedItem(SelectedConflictOption)
                .OnSelectionChanged(this, &SAssetMaterialBatchPanel::OnConflictPolicyChanged)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                {
                    return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()));
                })
                [ SNew(STextBlock).Text(this, &SAssetMaterialBatchPanel::GetConflictPolicyLabel) ]
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [
                    SNew(SCheckBox)
                    .IsChecked_Lambda([this]() { return Settings.bAssignToStaticMesh ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                    .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { Settings.bAssignToStaticMesh = NewState == ECheckBoxState::Checked; })
                ]
                + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
                [ SNew(STextBlock).Text(LOCTEXT("AssignBack", "执行后回填网格体")) ]
            ]
        ];
}

TSharedRef<SWidget> SAssetMaterialBatchPanel::BuildPreviewPanel()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
        .Padding(10.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [ MakeSectionTitle(LOCTEXT("PreviewTitle", "替换预览"), LOCTEXT("PreviewSub", "逐行确认：当前材质会替换成哪个新实例，以及纹理会绑定到哪个参数。")) ]
            + SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 8.0f, 0.0f, 0.0f)
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                [
                    SAssignNew(RowListView, SListView<TSharedPtr<AssetMaterialBatch::FRow>>)
                    .Visibility(this, &SAssetMaterialBatchPanel::GetPreviewTableVisibility)
                    .ListItemsSource(&Rows)
                    .OnGenerateRow(this, &SAssetMaterialBatchPanel::OnGenerateRow)
                    .OnSelectionChanged(this, &SAssetMaterialBatchPanel::OnRowSelectionChanged)
                    .SelectionMode(ESelectionMode::Single)
                ]
                + SOverlay::Slot()
                [
                    BuildEmptyPreviewPanel()
                ]
            ]
        ];
}

TSharedRef<SWidget> SAssetMaterialBatchPanel::BuildEmptyPreviewPanel()
{
    return SNew(SBorder)
        .Visibility(this, &SAssetMaterialBatchPanel::GetEmptyPreviewVisibility)
        .BorderImage(FAppStyle::GetBrush("Brushes.Recessed"))
        .Padding(18.0f)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(STextBlock)
                .Text(this, &SAssetMaterialBatchPanel::GetNextStepTitle)
                .ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.38f)))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f).HAlign(HAlign_Center)
            [
                SNew(STextBlock)
                .Text(this, &SAssetMaterialBatchPanel::GetNextStepText)
                .AutoWrapText(true)
                .Justification(ETextJustify::Center)
            ]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 12.0f, 0.0f, 0.0f)
            [
                SmallMutedText(LOCTEXT("EmptyPreviewHint", "使用上方主按钮继续。生成预览后，这里会列出每个网格体要创建和回填的材质实例。"))
            ]
        ];
}

TSharedRef<SWidget> SAssetMaterialBatchPanel::BuildDetailPanel()
{
    return SNew(SBorder)
        .Visibility_Lambda([this]() { return Rows.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
        .BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
        .Padding(10.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [ MakeSectionTitle(LOCTEXT("DetailTitle", "精细调整"), LOCTEXT("DetailSub", "选中一条预览后，只在这里改名称、槽位和输出路径。")) ]
            + SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 10.0f, 0.0f, 0.0f)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [ MakeDetailLine(LOCTEXT("DetailMesh", "网格体"), [this]() { return FText::FromString(SelectedRow.IsValid() ? AssetLabel(SelectedRow->MeshAsset) : TEXT("暂无预览行")); }) ]
                + SScrollBox::Slot().Padding(0.0f, 6.0f, 0.0f, 0.0f)
                [ SNew(STextBlock).Text(LOCTEXT("DetailSlot", "材质槽")) ]
                + SScrollBox::Slot().Padding(0.0f, 4.0f, 0.0f, 6.0f)
                [
                    SNew(SEditableTextBox)
                    .IsEnabled_Lambda([this]() { return SelectedRow.IsValid(); })
                    .Text_Lambda([this]() { return SelectedRow.IsValid() ? FText::AsNumber(SelectedRow->MaterialSlotIndex) : FText::GetEmpty(); })
                    .OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
                    {
                        if (SelectedRow.IsValid())
                        {
                            SelectedRow->MaterialSlotIndex = FMath::Max(0, FCString::Atoi(*Text.ToString()));
                            if (RowListView.IsValid())
                            {
                                RowListView->RequestListRefresh();
                            }
                        }
                    })
                ]
                + SScrollBox::Slot().Padding(0.0f, 6.0f, 0.0f, 0.0f)
                [ MakeDetailLine(LOCTEXT("DetailCurrentMat", "当前材质"), [this]() { return FText::FromString(SelectedRow.IsValid() ? AssetLabel(SelectedRow->CurrentMaterialAsset) : TEXT("-")); }) ]
                + SScrollBox::Slot().Padding(0.0f, 6.0f, 0.0f, 0.0f)
                [ MakeDetailLine(LOCTEXT("DetailParentMat", "父材质"), [this]() { return FText::FromString(Settings.ParentMaterial.IsValid() ? Settings.ParentMaterial->GetName() : TEXT("未设置")); }) ]
                + SScrollBox::Slot().Padding(0.0f, 12.0f, 0.0f, 0.0f)
                [ MakeDetailLine(LOCTEXT("DetailPrimaryTexture", "主纹理"), [this]() { return FText::FromString(SelectedRow.IsValid() ? AssetLabel(SelectedRow->PrimaryTextureAsset) : TEXT("-")); }) ]
                + SScrollBox::Slot().Padding(0.0f, 6.0f, 0.0f, 0.0f)
                [ MakeDetailLine(LOCTEXT("DetailParams", "纹理参数"), [this]() { return FText::FromString(SelectedRow.IsValid() ? TextureParameterSummary(SelectedRow->TexturesByParameter) : TEXT("-")); }) ]
                + SScrollBox::Slot().Padding(0.0f, 12.0f, 0.0f, 0.0f)
                [ SNew(STextBlock).Text(LOCTEXT("DetailNewMat", "新材质实例名")) ]
                + SScrollBox::Slot().Padding(0.0f, 4.0f, 0.0f, 0.0f)
                [
                    SNew(SEditableTextBox)
                    .IsEnabled_Lambda([this]() { return SelectedRow.IsValid(); })
                    .Text_Lambda([this]() { return FText::FromString(SelectedRow.IsValid() ? SelectedRow->NewMaterialName : FString()); })
                    .OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
                    {
                        if (SelectedRow.IsValid())
                        {
                            SelectedRow->NewMaterialName = Text.ToString();
                            if (RowListView.IsValid())
                            {
                                RowListView->RequestListRefresh();
                            }
                        }
                    })
                ]
                + SScrollBox::Slot().Padding(0.0f, 10.0f, 0.0f, 0.0f)
                [ SNew(STextBlock).Text(LOCTEXT("DetailOutput", "输出路径")) ]
                + SScrollBox::Slot().Padding(0.0f, 4.0f, 0.0f, 0.0f)
                [
                    SNew(SEditableTextBox)
                    .IsEnabled_Lambda([this]() { return SelectedRow.IsValid(); })
                    .Text_Lambda([this]() { return FText::FromString(SelectedRow.IsValid() ? SelectedRow->OutputPath : FString()); })
                    .OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
                    {
                        if (SelectedRow.IsValid())
                        {
                            SelectedRow->OutputPath = Text.ToString();
                            if (RowListView.IsValid())
                            {
                                RowListView->RequestListRefresh();
                            }
                        }
                    })
                ]
                + SScrollBox::Slot().Padding(0.0f, 12.0f, 0.0f, 0.0f)
                [ MakeDetailLine(LOCTEXT("DetailMessage", "当前问题"), [this]() { return FText::FromString(SelectedRow.IsValid() ? SelectedRow->Message : TEXT("先生成预览。")); }) ]
            ]
        ];
}

TSharedRef<SWidget> SAssetMaterialBatchPanel::MakeSectionTitle(const FText& Title, const FText& SubTitle) const
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(STextBlock)
            .Text(Title)
            .TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
        [
            SmallMutedText(SubTitle)
        ];
}

TSharedRef<SWidget> SAssetMaterialBatchPanel::MakeMetricLine(const FText& Label, TFunction<FText()> ValueGetter) const
{
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1.0f)
        [ SmallMutedText(Label) ]
        + SHorizontalBox::Slot().AutoWidth()
        [
            SNew(STextBlock)
            .Text_Lambda([ValueGetter]() { return ValueGetter(); })
        ];
}

TSharedRef<SWidget> SAssetMaterialBatchPanel::MakeChecklistLine(const FText& Label, TFunction<FText()> ValueGetter, TFunction<bool()> IsGoodGetter) const
{
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text_Lambda([IsGoodGetter]() { return IsGoodGetter() ? FText::FromString(TEXT("✓")) : FText::FromString(TEXT("!")); })
            .ColorAndOpacity_Lambda([IsGoodGetter]() { return IsGoodGetter() ? FSlateColor(FLinearColor(0.48f, 0.86f, 0.56f)) : FSlateColor(FLinearColor(1.0f, 0.72f, 0.28f)); })
        ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
        [ SmallMutedText(Label) ]
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text_Lambda([ValueGetter]() { return ValueGetter(); })
        ];
}

TSharedRef<SWidget> SAssetMaterialBatchPanel::MakeDetailLine(const FText& Label, TFunction<FText()> ValueGetter) const
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [ SmallMutedText(Label) ]
        + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text_Lambda([ValueGetter]() { return ValueGetter(); })
            .AutoWrapText(true)
        ];
}

FReply SAssetMaterialBatchPanel::OnLoadSelectionClicked()
{
    RefreshSelectionFromContentBrowser(true);
    return FReply::Handled();
}

FReply SAssetMaterialBatchPanel::OnScanTextureFolderClicked()
{
    if (TextureFolderTextBox.IsValid())
    {
        Settings.TextureFolderPath = TextureFolderTextBox->GetText().ToString();
    }

    if (IsUnsafeTextureFolder(Settings.TextureFolderPath))
    {
        ScannedTextures.Reset();
        Rows.Reset();
        SelectedRow.Reset();
        if (RowListView.IsValid())
        {
            RowListView->RequestListRefresh();
        }
        IssueText = TEXT("请填写明确的纹理子目录，例如 /Game/Characters/Hand/Textures。为了安全，不允许扫描空目录或 /Game 根目录。");
        UpdateSummary();
        return FReply::Handled();
    }

    FMaterialBatchProcessor::ScanTexturesInFolder(Settings.TextureFolderPath, ScannedTextures);
    Rows.Reset();
    SelectedRow.Reset();
    if (RowListView.IsValid())
    {
        RowListView->RequestListRefresh();
    }
    IssueText = FString::Printf(TEXT("已扫描 %d 个纹理。下一步：点击“生成预览”。"), ScannedTextures.Num());
    UpdateSummary();
    return FReply::Handled();
}

FReply SAssetMaterialBatchPanel::OnBuildPreviewClicked()
{
    if (OutputFolderTextBox.IsValid())
    {
        Settings.OutputFolderPath = OutputFolderTextBox->GetText().ToString();
    }
    if (TextureFolderTextBox.IsValid())
    {
        Settings.TextureFolderPath = TextureFolderTextBox->GetText().ToString();
    }

    FString ValidationMessage;
    if (!ValidateBeforePreview(ValidationMessage))
    {
        IssueText = ValidationMessage;
        UpdateSummary();
        return FReply::Handled();
    }

    TArray<FAssetData> AllTextures = SelectedTextures;
    for (const FAssetData& TextureAsset : ScannedTextures)
    {
        AllTextures.AddUnique(TextureAsset);
    }

    TArray<AssetMaterialBatch::FRow> NewRows;
    FMaterialBatchProcessor::BuildPreviewRows(TargetMeshes, AllTextures, Settings, NewRows);
    RebuildRows(NewRows);
    IssueText = NewRows.Num() > 0 ? TEXT("预览已生成。下一步：检查表格，确认后点击“执行批处理”。") : TEXT("没有生成可处理的预览行。");
    UpdateSummary();
    return FReply::Handled();
}

FReply SAssetMaterialBatchPanel::OnExecuteClicked()
{
    if (!CanExecuteRows())
    {
        IssueText = TEXT("当前没有可执行任务。请先生成预览并处理关键问题。");
        UpdateSummary();
        return FReply::Handled();
    }

    TArray<AssetMaterialBatch::FRow> RawRows;
    RawRows.Reserve(Rows.Num());
    for (const TSharedPtr<AssetMaterialBatch::FRow>& Row : Rows)
    {
        if (Row.IsValid())
        {
            RawRows.Add(*Row);
        }
    }

    FMaterialBatchProcessor::ExecuteRows(Settings, RawRows);
    RebuildRows(RawRows);
    IssueText = TEXT("执行完成。请检查完成、警告和错误行。资产已标记为待保存。");
    UpdateSummary();
    return FReply::Handled();
}

FReply SAssetMaterialBatchPanel::OnPrimaryActionClicked()
{
    if (!HasAnyInputAsset())
    {
        return OnLoadSelectionClicked();
    }
    if (CanExecuteRows())
    {
        return OnExecuteClicked();
    }
    if (CanBuildPreview())
    {
        return OnBuildPreviewClicked();
    }

    FString ValidationMessage;
    ValidateBeforePreview(ValidationMessage);
    IssueText = ValidationMessage.IsEmpty() ? GetNextStepText().ToString() : ValidationMessage;
    UpdateSummary();
    return FReply::Handled();
}

FReply SAssetMaterialBatchPanel::OnClearClicked()
{
    LastSelection.Reset();
    TargetMeshes.Reset();
    SelectedTextures.Reset();
    ScannedTextures.Reset();
    SelectedMaterials.Reset();
    SelectedFolders.Reset();
    Rows.Reset();
    SelectedRow.Reset();
    IssueText.Empty();

    if (RowListView.IsValid())
    {
        RowListView->RequestListRefresh();
    }

    UpdateSummary();
    return FReply::Handled();
}

void SAssetMaterialBatchPanel::RefreshSelectionFromContentBrowser(bool bUpdateStatusText)
{
    FMaterialBatchProcessor::GetContentBrowserSelection(LastSelection);
    FMaterialBatchProcessor::GetContentBrowserSelectedFolders(SelectedFolders);

    TArray<FAssetData> NewMeshes;
    TArray<FAssetData> NewTextures;
    TArray<FAssetData> NewMaterials;
    FMaterialBatchProcessor::SplitAssets(LastSelection, NewMeshes, NewTextures, NewMaterials);

    if (NewMeshes.Num() > 0 || !bUpdateStatusText)
    {
        TargetMeshes = NewMeshes;
    }

    if (NewTextures.Num() > 0 || !bUpdateStatusText)
    {
        SelectedTextures = NewTextures;
    }

    if (NewMaterials.Num() > 0 || !bUpdateStatusText)
    {
        SelectedMaterials = NewMaterials;
    }

    if (NewMaterials.Num() > 0 || !Settings.ParentMaterial.IsValid())
    {
        Settings.ParentMaterial = FMaterialBatchProcessor::FindFirstMaterialInterface(LastSelection);
    }

    bool bScannedSelectedFolder = false;
    bool bSelectedUnsafeFolder = false;
    if (SelectedFolders.Num() > 0)
    {
        Settings.TextureFolderPath = SelectedFolders[0];
        if (TextureFolderTextBox.IsValid())
        {
            TextureFolderTextBox->SetText(FText::FromString(Settings.TextureFolderPath));
        }

        if (IsUnsafeTextureFolder(Settings.TextureFolderPath))
        {
            ScannedTextures.Reset();
            bSelectedUnsafeFolder = true;
        }
        else
        {
            FMaterialBatchProcessor::ScanTexturesInFolder(Settings.TextureFolderPath, ScannedTextures);
            bScannedSelectedFolder = true;
        }
    }

    if (bUpdateStatusText)
    {
        Rows.Reset();
        SelectedRow.Reset();
        if (RowListView.IsValid())
        {
            RowListView->RequestListRefresh();
        }

        IssueText = FString::Printf(
            TEXT("已载入：%d 个网格体，%d 个已选纹理，%d 个扫描纹理，%d 个父材质候选。"),
            TargetMeshes.Num(),
            SelectedTextures.Num(),
            ScannedTextures.Num(),
            SelectedMaterials.Num());

        if (bScannedSelectedFolder)
        {
            IssueText += FString::Printf(TEXT(" 已使用选中文件夹：%s。"), *Settings.TextureFolderPath);
        }
        else if (bSelectedUnsafeFolder)
        {
            IssueText += TEXT(" 选中的文件夹不能扫描，请选择 /Game 下的具体子目录。");
        }

        UpdateSummary();
    }
}

void SAssetMaterialBatchPanel::RebuildRows(const TArray<AssetMaterialBatch::FRow>& SourceRows)
{
    Rows.Reset();
    Rows.Reserve(SourceRows.Num());

    for (const AssetMaterialBatch::FRow& SourceRow : SourceRows)
    {
        Rows.Add(MakeShared<AssetMaterialBatch::FRow>(SourceRow));
    }

    SelectedRow = Rows.Num() > 0 ? Rows[0] : nullptr;

    if (RowListView.IsValid())
    {
        RowListView->RequestListRefresh();
        if (SelectedRow.IsValid())
        {
            RowListView->SetSelection(SelectedRow);
        }
    }
}

void SAssetMaterialBatchPanel::UpdateSummary()
{
    int32 ReadyCount = 0;
    int32 DoneCount = 0;
    int32 WarningCount = 0;
    int32 ErrorCount = 0;

    for (const TSharedPtr<AssetMaterialBatch::FRow>& Row : Rows)
    {
        if (!Row.IsValid())
        {
            continue;
        }

        switch (Row->Status)
        {
        case AssetMaterialBatch::ERowStatus::Ready: ++ReadyCount; break;
        case AssetMaterialBatch::ERowStatus::Done: ++DoneCount; break;
        case AssetMaterialBatch::ERowStatus::Warning: ++WarningCount; break;
        case AssetMaterialBatch::ERowStatus::Error: ++ErrorCount; break;
        default: break;
        }
    }

    SummaryText = FString::Printf(
        TEXT("%d 网格体 | %d 纹理 | %s | %d 预览行"),
        TargetMeshes.Num(),
        SelectedTextures.Num() + ScannedTextures.Num(),
        Settings.ParentMaterial.IsValid() ? TEXT("父材质已设置") : TEXT("父材质未设置"),
        Rows.Num());

    if (Rows.Num() > 0)
    {
        SummaryText += FString::Printf(TEXT(" | %d 就绪 / %d 完成 / %d 警告 / %d 错误"), ReadyCount, DoneCount, WarningCount, ErrorCount);
    }
}

bool SAssetMaterialBatchPanel::ValidateBeforePreview(FString& OutMessage) const
{
    if (!Settings.ParentMaterial.IsValid())
    {
        OutMessage = TEXT("下一步：先在左侧选择父材质。");
        return false;
    }

    if (!HasAnyInputAsset())
    {
        OutMessage = TEXT("下一步：在内容浏览器选择网格体和纹理，然后点击“载入当前选择”。");
        return false;
    }

    if (TargetMeshes.Num() > 0 && SelectedTextures.Num() == 0 && ScannedTextures.Num() == 0)
    {
        OutMessage = TEXT("下一步：添加纹理来源。可以选择纹理后重新载入，或填写纹理目录并扫描。");
        return false;
    }

    if (!Settings.OutputFolderPath.StartsWith(TEXT("/Game")))
    {
        OutMessage = TEXT("下一步：把输出目录改成 /Game 开头的路径。");
        return false;
    }

    OutMessage.Empty();
    return true;
}

bool SAssetMaterialBatchPanel::CanBuildPreview() const
{
    FString Ignored;
    return Rows.Num() == 0 && ValidateBeforePreview(Ignored);
}

bool SAssetMaterialBatchPanel::CanExecuteRows() const
{
    if (!Settings.ParentMaterial.IsValid() || Rows.Num() == 0)
    {
        return false;
    }

    for (const TSharedPtr<AssetMaterialBatch::FRow>& Row : Rows)
    {
        if (Row.IsValid()
            && Row->bEnabled
            && Row->Status != AssetMaterialBatch::ERowStatus::Error
            && Row->PrimaryTextureAsset.IsValid()
            && Row->TexturesByParameter.Num() > 0)
        {
            return true;
        }
    }

    return false;
}

bool SAssetMaterialBatchPanel::IsUnsafeTextureFolder(const FString& FolderPath) const
{
    FString CleanPath = FolderPath;
    CleanPath.TrimStartAndEndInline();
    CleanPath.RemoveFromEnd(TEXT("/"));
    return CleanPath.IsEmpty() || CleanPath == TEXT("/Game") || !CleanPath.StartsWith(TEXT("/Game/"));
}

bool SAssetMaterialBatchPanel::HasAnyInputAsset() const
{
    return TargetMeshes.Num() + SelectedTextures.Num() + ScannedTextures.Num() > 0;
}

FString SAssetMaterialBatchPanel::GetParentMaterialPath() const
{
    return Settings.ParentMaterial.IsValid() ? Settings.ParentMaterial->GetPathName() : FString();
}

void SAssetMaterialBatchPanel::OnParentMaterialChanged(const FAssetData& AssetData)
{
    Settings.ParentMaterial = Cast<UMaterialInterface>(AssetData.GetAsset());
    IssueText = Settings.ParentMaterial.IsValid() ? TEXT("父材质已设置。下一步：补齐纹理来源后生成预览。") : TEXT("父材质已清空。");
    Rows.Reset();
    SelectedRow.Reset();
    if (RowListView.IsValid())
    {
        RowListView->RequestListRefresh();
    }
    UpdateSummary();
}

void SAssetMaterialBatchPanel::OnConflictPolicyChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
    SelectedConflictOption = NewSelection;

    if (!NewSelection.IsValid())
    {
        return;
    }

    if (*NewSelection == TEXT("跳过已存在"))
    {
        Settings.ConflictPolicy = AssetMaterialBatch::EConflictPolicy::Skip;
    }
    else if (*NewSelection == TEXT("更新已存在"))
    {
        Settings.ConflictPolicy = AssetMaterialBatch::EConflictPolicy::UpdateExisting;
    }
    else
    {
        Settings.ConflictPolicy = AssetMaterialBatch::EConflictPolicy::AutoRename;
    }
}

FText SAssetMaterialBatchPanel::GetConflictPolicyLabel() const
{
    return FText::FromString(SelectedConflictOption.IsValid() ? *SelectedConflictOption : FString(TEXT("自动改名")));
}

void SAssetMaterialBatchPanel::OnRowSelectionChanged(TSharedPtr<AssetMaterialBatch::FRow> RowItem, ESelectInfo::Type SelectInfo)
{
    SelectedRow = RowItem;
}

FText SAssetMaterialBatchPanel::GetCompactSummaryText() const
{
    return FText::FromString(SummaryText);
}

FText SAssetMaterialBatchPanel::GetNextStepTitle() const
{
    if (!HasAnyInputAsset())
    {
        return LOCTEXT("NextLoad", "下一步：载入资产");
    }
    if (!Settings.ParentMaterial.IsValid())
    {
        return LOCTEXT("NextParent", "下一步：选择父材质");
    }
    if (TargetMeshes.Num() > 0 && SelectedTextures.Num() == 0 && ScannedTextures.Num() == 0)
    {
        return LOCTEXT("NextTexture", "下一步：添加纹理来源");
    }
    if (!Settings.OutputFolderPath.StartsWith(TEXT("/Game")))
    {
        return LOCTEXT("NextOutput", "下一步：修正输出目录");
    }
    if (Rows.Num() == 0)
    {
        return LOCTEXT("NextPreview", "下一步：生成预览");
    }
    if (CanExecuteRows())
    {
        return LOCTEXT("NextExecute", "下一步：执行批处理");
    }
    return LOCTEXT("NextCheck", "下一步：检查问题");
}

FText SAssetMaterialBatchPanel::GetNextStepText() const
{
    if (!HasAnyInputAsset())
    {
        return LOCTEXT("NextLoadText", "在内容浏览器选择静态网格体、纹理或父材质，然后点击“载入当前选择”。");
    }
    if (!Settings.ParentMaterial.IsValid())
    {
        return LOCTEXT("NextParentText", "左侧“父材质”必须先指定，否则不会生成预览。");
    }
    if (TargetMeshes.Num() > 0 && SelectedTextures.Num() == 0 && ScannedTextures.Num() == 0)
    {
        return LOCTEXT("NextTextureText", "你已经选择了网格体。现在选择一个或多个纹理后重新载入，或填写具体纹理目录并扫描。");
    }
    if (!Settings.OutputFolderPath.StartsWith(TEXT("/Game")))
    {
        return LOCTEXT("NextOutputText", "输出目录必须是 /Game 开头的内容目录，例如 /Game/Generated/MaterialInstances。");
    }
    if (Rows.Num() == 0)
    {
        return LOCTEXT("NextPreviewText", "输入条件已满足。点击“生成预览”，插件会列出将要创建和回填的材质实例。");
    }
    if (CanExecuteRows())
    {
        return LOCTEXT("NextExecuteText", "检查表格和右侧当前行详情，确认无误后执行。执行不会自动保存资产。");
    }
    return LOCTEXT("NextCheckText", "当前预览行存在问题，请检查“问题”列和右侧详情。");
}

FText SAssetMaterialBatchPanel::GetPrimaryActionText() const
{
    if (!HasAnyInputAsset())
    {
        return LOCTEXT("PrimaryLoad", "载入当前选择");
    }
    if (!Settings.ParentMaterial.IsValid())
    {
        return LOCTEXT("PrimaryNeedParent", "先选择父材质");
    }
    if (TargetMeshes.Num() > 0 && SelectedTextures.Num() == 0 && ScannedTextures.Num() == 0)
    {
        return LOCTEXT("PrimaryNeedTexture", "先添加纹理来源");
    }
    if (!Settings.OutputFolderPath.StartsWith(TEXT("/Game")))
    {
        return LOCTEXT("PrimaryNeedOutput", "修正输出目录");
    }
    if (CanExecuteRows())
    {
        return LOCTEXT("PrimaryExecute", "执行批处理");
    }
    if (CanBuildPreview())
    {
        return LOCTEXT("PrimaryPreview", "生成预览");
    }
    return LOCTEXT("PrimaryBlocked", "等待补齐条件");
}

FText SAssetMaterialBatchPanel::GetIssueText() const
{
    return FText::FromString(IssueText);
}

EVisibility SAssetMaterialBatchPanel::GetIssueVisibility() const
{
    return IssueText.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SAssetMaterialBatchPanel::GetPreviewTableVisibility() const
{
    return Rows.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SAssetMaterialBatchPanel::GetEmptyPreviewVisibility() const
{
    return Rows.Num() == 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

TSharedRef<ITableRow> SAssetMaterialBatchPanel::OnGenerateRow(TSharedPtr<AssetMaterialBatch::FRow> RowItem, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(SMaterialBatchTableRow, OwnerTable).RowItem(RowItem);
}

#undef LOCTEXT_NAMESPACE
