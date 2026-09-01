# Risk Roadmap

## Risk 1 - Matching false positives

Problem: automatic name matching can bind the wrong texture to a mesh.

Next step: add a confidence score column and make row-level texture assignment editable with asset pickers.

## Risk 2 - Multi-slot mesh handling

Problem: slot 0 is safe for MVP, but production meshes may have several material slots.

Next step: add slot name display, per-slot rows, and a mode selector: slot 0 only, all slots, selected slot names, or match by old material name.

## Risk 3 - Parent material parameter differences

Problem: studios use different parameter names.

Next step: expose parameter mapping as editable presets, including suffix -> parameter name rules.

## Risk 4 - Folder context path

Problem: folder right-click currently opens the panel but does not auto-fill the folder path.

Next step: read UContentBrowserFolderContext from the ToolMenus context and push the clicked path into the texture folder field.

## Risk 5 - Existing asset overwrite safety

Problem: Update Existing can modify production material instances.

Next step: add a dry-run diff view and an optional duplicate-before-update mode.

## Risk 6 - Operation auditability

Problem: users need a clear record of what changed.

Next step: export execution report as CSV or JSON under the project Saved folder, with mesh path, old material, new material, textures, status, and message.

## Risk 7 - Higher asset coverage

Problem: the MVP only supports Static Mesh asset assignment.

Next step: add Skeletal Mesh support, then optional level Actor component override support as a separate mode.
