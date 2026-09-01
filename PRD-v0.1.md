# PRD v0.1 - UE Asset Material Batch Tool

## Product goal

Build a UE editor plugin for high-control batch material workflows. The tool should behave like a production asset batch panel, not a one-click script: auto-detect where it can, preview every planned operation, allow manual correction, then execute safely.

## Target users

Artists and technical artists who repeatedly need to rebuild material instances, bind textures, and assign new material instances back to Static Mesh assets.

## MVP scope

### Asset inputs

- Static Mesh assets
- Texture2D assets
- Material assets
- Material Instance Constant assets
- Texture folders through manual /Game path scan

### Operations

1. Create Material Instance Constant assets from selected or scanned textures.
2. Rebuild Static Mesh material instances from a chosen parent material and matched textures.
3. Assign the new material instance back to the Static Mesh asset.

### Safety rules

- No asset changes before Execute.
- Preview table is mandatory.
- Existing assets are handled by conflict policy.
- Old material instances are not deleted.
- Assets are marked dirty but not auto-saved.

## Default naming rules

- T_ prefix becomes M_ prefix.
- T prefix becomes M prefix.
- If a texture has no T prefix, the created material instance gets M_ prepended.

## Default matching rules

- Remove known asset prefixes: SM_, SK_, T_, MI_, M_.
- Remove known texture suffixes for matching only.
- Compare normalized alphanumeric lowercase keys.
- If only one texture group exists, apply it to all selected meshes.

## Acceptance criteria for v0.1

- Plugin loads in UE 5.7 editor.
- Plugin icon appears in the plugin descriptor and menu entry style.
- Tool window opens from Tools menu.
- Context menu entries open the same tool window.
- Selecting meshes and textures can generate preview rows.
- Execute can create material instances.
- Execute can set texture parameters on material instances.
- Execute can assign generated material instances back to Static Mesh slot 0.
- Compile succeeds with UE 5.7 Win64 Development.
