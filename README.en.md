# Asset Material Batch Tool

An Unreal Engine 5.7 editor plugin for batch-creating material instances, binding textures, and assigning generated material instances back to Static Mesh material slots.

Current version: `v0.3.3`

## What It Solves

This plugin is built for workflows where a batch of Static Mesh assets each has its own material instance, and you need to rebuild those instances under a chosen parent material with matched textures.

Typical use cases:

- Rebuilding material instance sets for vehicle, character, or prop asset packs.
- Matching texture sets by current material names or Static Mesh names.
- Generating `M_` material instances from `T_` texture assets.
- Previewing every planned replacement before modifying assets.

## Installation

1. Download or unzip the plugin.
2. Place the `AssetMaterialBatchTool` folder under your UE project's `Plugins` directory.
3. If the project does not have a `Plugins` directory, create one next to the `.uproject` file.
4. Open the Unreal Engine project.
5. Confirm plugin compilation if Unreal Engine asks for it.

Recommended layout:

```text
YourProject/
  YourProject.uproject
  Plugins/
    AssetMaterialBatchTool/
      AssetMaterialBatchTool.uplugin
```

## Opening the Tool

The plugin adds a Chinese editor entry named:

```text
资产材质批处理工具
```

It can be opened from the editor tools menu or from Content Browser context menus for Static Mesh, Texture2D, Material, Material Instance Constant, and folders.

## Recommended Workflow

1. Select the Static Mesh assets in the Content Browser.
2. Open the tool panel and click `载入当前选择`.
3. Set the parent material on the left panel.
4. Add texture sources by selecting textures, selecting a texture folder, or manually scanning a `/Game/...` folder.
5. Confirm the output folder, for example `/Game/Generated/MaterialInstances`.
6. Click `生成预览`.
7. Review every card in the `替换预览` area.
8. Select a row and adjust material slot, instance name, or output path in the detail panel if needed.
9. Click `执行批处理` when the preview is correct.
10. Save modified assets in Unreal Engine.

## Preview Cards

Each preview card represents one planned material creation and assignment operation.

A card shows:

- `Status`: ready, done, warning, error, or skipped.
- `Static Mesh`: the target mesh asset.
- `Slot`: the target material slot index.
- `Current Material`: the material currently assigned to that slot.
- `New Material Instance`: the instance that will be created or updated.
- `Texture Binding`: the parent material parameter each texture will bind to.
- `Message`: match source, warning, or failure reason.

## Naming Rules

The generated material instance name is derived from the primary texture name.

```text
T_Glass -> M_Glass
TGlass  -> MGlass
Glass   -> M_Glass
```

Invalid asset-name characters are converted to `_`.

## Texture Matching Rules

Matching priority:

1. Current material name.
2. Static Mesh name.
3. Single texture group fallback.

Recognized texture suffixes include:

- BaseColor: `_BaseColor`, `_Albedo`, `_Diffuse`, `_D`, `_BC`
- Normal: `_Normal`, `_N`, `_NRM`
- ORM: `_ORM`, `_MRA`, `_Mask`
- Roughness: `_Roughness`, `_R`
- Metallic: `_Metallic`, `_Metalness`, `_M`
- Specular: `_Specular`, `_S`
- Opacity: `_Opacity`, `_Alpha`, `_A`
- Emissive: `_Emissive`, `_Emission`, `_E`

When only one texture is provided, the plugin attempts to bind it to the most reasonable texture parameter found on the parent material.

## Safety Rules

- Empty folders and the `/Game` root cannot be scanned.
- A parent material is required before preview generation.
- Texture sources are required before mesh rebuild preview generation.
- Rows without matched textures or valid texture parameters are skipped during execution.
- Assets are marked dirty but are not auto-saved.
- Existing material instances are not deleted.

## Current Limitations

- Focused on Static Mesh assets.
- Multi-slot meshes default to slot 0; the slot index can be edited manually.
- Texture parameter mapping presets are not user-editable yet.
- No automatic asset saving.
