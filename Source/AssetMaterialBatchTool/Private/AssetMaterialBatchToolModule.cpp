#include "AssetMaterialBatchToolModule.h"

#include "Interfaces/IPluginManager.h"
#include "SAssetMaterialBatchPanel.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FAssetMaterialBatchToolModule"

namespace
{
    static const FName AssetMaterialBatchToolTabName(TEXT("AssetMaterialBatchTool"));
    static const FName AssetMaterialBatchToolStyleName(TEXT("AssetMaterialBatchToolStyle"));
}

void FAssetMaterialBatchToolModule::StartupModule()
{
    RegisterStyle();

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        AssetMaterialBatchToolTabName,
        FOnSpawnTab::CreateRaw(this, &FAssetMaterialBatchToolModule::SpawnMainTab))
        .SetDisplayName(LOCTEXT("TabTitle", "资产材质批处理工具"))
        .SetMenuType(ETabSpawnerMenuType::Hidden)
        .SetIcon(FSlateIcon(AssetMaterialBatchToolStyleName, "AssetMaterialBatchTool.Icon"));

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAssetMaterialBatchToolModule::RegisterMenus));
}

void FAssetMaterialBatchToolModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AssetMaterialBatchToolTabName);
    UnregisterStyle();
}

TSharedRef<SDockTab> FAssetMaterialBatchToolModule::SpawnMainTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SAssetMaterialBatchPanel)
        ];
}

void FAssetMaterialBatchToolModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
    FToolMenuSection& ToolsSection = ToolsMenu->FindOrAddSection("AssetMaterialBatchTool");
    ToolsSection.AddMenuEntry(
        "OpenAssetMaterialBatchTool",
        LOCTEXT("OpenToolLabel", "资产材质批处理工具"),
        LOCTEXT("OpenToolTooltip", "打开资产材质批处理面板。"),
        FSlateIcon(AssetMaterialBatchToolStyleName, "AssetMaterialBatchTool.Icon"),
        FUIAction(FExecuteAction::CreateRaw(this, &FAssetMaterialBatchToolModule::OpenToolWindow)));

    const FName ContextMenus[] =
    {
        TEXT("ContentBrowser.AssetContextMenu.StaticMesh"),
        TEXT("ContentBrowser.AssetContextMenu.Texture2D"),
        TEXT("ContentBrowser.AssetContextMenu.Material"),
        TEXT("ContentBrowser.AssetContextMenu.MaterialInstanceConstant"),
        TEXT("ContentBrowser.FolderContextMenu")
    };

    for (const FName MenuName : ContextMenus)
    {
        UToolMenu* ContextMenu = UToolMenus::Get()->ExtendMenu(MenuName);
        FToolMenuSection& Section = ContextMenu->FindOrAddSection("AssetMaterialBatchTool");
        Section.AddMenuEntry(
            "OpenAssetMaterialBatchToolFromContext",
            LOCTEXT("OpenFromContextLabel", "材质批处理工具"),
            LOCTEXT("OpenFromContextTooltip", "使用当前内容浏览器选择打开材质批处理面板。"),
            FSlateIcon(AssetMaterialBatchToolStyleName, "AssetMaterialBatchTool.Icon"),
            FUIAction(FExecuteAction::CreateRaw(this, &FAssetMaterialBatchToolModule::OpenToolWindow)));
    }
}

void FAssetMaterialBatchToolModule::OpenToolWindow()
{
    FGlobalTabmanager::Get()->TryInvokeTab(AssetMaterialBatchToolTabName);
}

void FAssetMaterialBatchToolModule::RegisterStyle()
{
    if (StyleSet.IsValid())
    {
        return;
    }

    StyleSet = MakeShared<FSlateStyleSet>(AssetMaterialBatchToolStyleName);

    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("AssetMaterialBatchTool"));
    if (Plugin.IsValid())
    {
        StyleSet->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources"));
        StyleSet->Set("AssetMaterialBatchTool.Icon", new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Icon128"), TEXT(".png")), FVector2D(20.0f, 20.0f)));
    }

    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FAssetMaterialBatchToolModule::UnregisterStyle()
{
    if (StyleSet.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
        ensure(StyleSet.IsUnique());
        StyleSet.Reset();
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAssetMaterialBatchToolModule, AssetMaterialBatchTool)

