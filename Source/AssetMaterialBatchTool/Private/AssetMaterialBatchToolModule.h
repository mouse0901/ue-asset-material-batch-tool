#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSlateStyleSet;
class SDockTab;

class FAssetMaterialBatchToolModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TSharedRef<SDockTab> SpawnMainTab(const class FSpawnTabArgs& SpawnTabArgs);
    void RegisterMenus();
    void OpenToolWindow();
    void RegisterStyle();
    void UnregisterStyle();

private:
    TSharedPtr<FSlateStyleSet> StyleSet;
};
