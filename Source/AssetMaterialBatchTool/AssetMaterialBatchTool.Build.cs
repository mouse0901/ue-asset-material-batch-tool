using UnrealBuildTool;

public class AssetMaterialBatchTool : ModuleRules
{
    public AssetMaterialBatchTool(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "ApplicationCore",
            "AssetRegistry",
            "AssetTools",
            "ContentBrowser",
            "EditorFramework",
            "InputCore",
            "Projects",
            "PropertyEditor",
            "ToolMenus",
            "UnrealEd"
        });
    }
}
