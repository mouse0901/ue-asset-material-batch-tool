# Asset Material Batch Tool / 资产材质批处理工具

中文 | [English](#english)

## 中文

`Asset Material Batch Tool` 是一个 Unreal Engine 5.7 编辑器插件，用于批量创建材质实例、绑定纹理，并把新材质实例回填到静态网格体材质槽。

它适合处理汽车、角色、道具等资产包中的批量材质重建工作：先选择静态网格体、父材质和纹理来源，生成可检查的替换预览，再确认执行。插件会尽量自动匹配纹理和父材质参数，同时保留手动微调入口，避免一键脚本式误操作。

### 核心功能

- 批量从纹理创建 `Material Instance Constant`。
- 指定统一父材质，并自动绑定纹理参数。
- 把生成或更新后的材质实例回填到静态网格体。
- 支持选中纹理、选中纹理文件夹、手动填写 `/Game/...` 目录扫描。
- 预览区使用替换卡片，直接展示“当前材质 -> 新材质实例”。
- 支持冲突策略：自动改名、跳过已存在、更新已存在。
- 执行前必须生成预览，不会在预览阶段修改资产。
- 执行后只标记资产为待保存，不会自动保存或删除旧材质。

### 适用场景

- 一批静态网格体原本各自挂着独立材质实例，需要统一换到新的父材质体系。
- 纹理名称和网格体名称、当前材质名称存在对应关系。
- 需要批量生成 `M_` 前缀材质实例，并把纹理绑定到父材质真实存在的纹理参数。

### 安装

1. 下载或解压插件。
2. 将 `AssetMaterialBatchTool` 文件夹放到 UE 项目的 `Plugins` 目录。
3. 如果项目没有 `Plugins` 目录，在 `.uproject` 同级新建一个。
4. 启动 UE 项目。
5. 如果 UE 提示需要重新编译插件，确认编译。

推荐目录结构：

```text
YourProject/
  YourProject.uproject
  Plugins/
    AssetMaterialBatchTool/
      AssetMaterialBatchTool.uplugin
```

### 使用流程

1. 在内容浏览器选择要处理的静态网格体。
2. 打开 `资产材质批处理工具`。
3. 点击 `载入当前选择`。
4. 在左侧设置 `父材质`。
5. 选择纹理来源：选中纹理、选中纹理文件夹，或手动填写 `/Game/...` 目录后扫描。
6. 点击 `生成预览`。
7. 在 `替换预览` 卡片中检查每一条结果。
8. 需要微调时，选中预览行，在右侧 `精细调整` 中修改材质槽、实例名或输出路径。
9. 确认无误后点击 `执行批处理`。
10. 回到 UE 保存被修改的资产。

### 命名规则

```text
T_Glass -> M_Glass
TGlass  -> MGlass
Glass   -> M_Glass
```

如果名称中存在非法字符，插件会自动替换为 `_`。

### 匹配规则

纹理匹配优先级：

1. 当前材质名匹配纹理组。
2. 网格体名匹配纹理组。
3. 如果只有一组纹理，使用单组纹理兜底。

插件会识别常见纹理后缀，例如 `_BaseColor`、`_Albedo`、`_Normal`、`_ORM`、`_Roughness`、`_Metallic`、`_Opacity`、`_Emissive`。

### 安全策略

- 不允许扫描空目录或 `/Game` 根目录。
- 不设置父材质时不能生成预览。
- 没有纹理来源时不能生成网格体重建预览。
- 没有匹配纹理或父材质没有可用纹理参数时，执行会跳过该行。
- 不自动保存资产。
- 不删除旧材质实例。

### 当前限制

- 当前主要面向 Static Mesh。
- 多材质槽默认处理 0 号槽，可在预览或精细调整里手动修改槽位。
- 参数映射还不是用户可编辑预设。
- 暂不支持自动保存、批量撤销报告或发布版安装器。

---

## English

`Asset Material Batch Tool` is an Unreal Engine 5.7 editor plugin for batch-creating material instances, binding textures, and assigning the generated material instances back to Static Mesh material slots.

It is designed for production asset cleanup workflows such as vehicle, character, and prop packs. The tool lets you select Static Mesh assets, choose a parent material, provide texture sources, preview every planned replacement, manually adjust individual rows, and execute only after review.

### Key Features

- Batch-create `Material Instance Constant` assets from selected or scanned textures.
- Assign a chosen parent material to generated material instances.
- Automatically bind textures to real texture parameters found on the parent material.
- Assign generated or updated material instances back to Static Mesh assets.
- Supports selected textures, selected texture folders, and manual `/Game/...` folder scans.
- Uses replacement preview cards instead of compressed table rows.
- Supports conflict policies: auto-rename, skip existing assets, or update existing assets.
- Requires preview before execution.
- Marks assets dirty after execution, but does not auto-save or delete old material instances.

### Use Cases

- Rebuild a batch of per-mesh material instances under a new shared parent material.
- Match texture sets by current material names or Static Mesh names.
- Generate `M_`-prefixed material instances from `T_` texture names.
- Quickly inspect what will be created and assigned before modifying assets.

### Installation

1. Download or unzip the plugin.
2. Put the `AssetMaterialBatchTool` folder into your UE project's `Plugins` directory.
3. If the project does not have a `Plugins` directory, create one next to the `.uproject` file.
4. Open the UE project.
5. Confirm plugin compilation if Unreal Engine asks for it.

Recommended layout:

```text
YourProject/
  YourProject.uproject
  Plugins/
    AssetMaterialBatchTool/
      AssetMaterialBatchTool.uplugin
```

### Workflow

1. Select the Static Mesh assets you want to process in the Content Browser.
2. Open `Asset Material Batch Tool`.
3. Click `Load Current Selection`.
4. Choose the parent material on the left panel.
5. Add texture sources by selecting textures, selecting a texture folder, or manually scanning a `/Game/...` folder.
6. Click `Build Preview`.
7. Review each replacement card.
8. Select a row and use the detail panel to adjust the material slot, instance name, or output path.
9. Click `Execute Batch` when the preview is correct.
10. Save modified assets in Unreal Engine.

### Naming Rules

```text
T_Glass -> M_Glass
TGlass  -> MGlass
Glass   -> M_Glass
```

Invalid asset-name characters are converted to `_`.

### Matching Rules

Texture matching priority:

1. Match by current material name.
2. Match by Static Mesh name.
3. If there is only one texture group, use it as a fallback.

The plugin recognizes common texture suffixes such as `_BaseColor`, `_Albedo`, `_Normal`, `_ORM`, `_Roughness`, `_Metallic`, `_Opacity`, and `_Emissive`.

### Safety Rules

- Empty folders and the `/Game` root cannot be scanned.
- A parent material is required before preview generation.
- A texture source is required before mesh rebuild preview generation.
- Rows without matched textures or valid texture parameters are skipped during execution.
- Assets are not auto-saved.
- Existing material instances are not deleted.

### Current Limitations

- Focused on Static Mesh assets.
- Multi-slot meshes default to slot 0; slot index can be edited manually.
- Texture parameter mapping presets are not user-editable yet.
- No auto-save, batch undo report, or installer packaging yet.
