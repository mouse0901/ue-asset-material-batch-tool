# 验证记录

## v0.3.3

- UE 版本：`F:\UE_5.7`
- 编译项目：`E:\CodexOutputs\projects\ue-asset-material-batch-tool\package\HostProject\HostProject.uproject`
- 插件路径：`E:\CodexOutputs\projects\ue-asset-material-batch-tool\package\HostProject\Plugins\AssetMaterialBatchTool`
- 编译命令：`UnrealBuildTool.exe UnrealEditor Win64 Development -Project=... -plugin=... -NoEngineChanges -NoHotReloadFromIDE`
- 结果：`Succeeded`

本版重点验证了预览 UI 重构后的编译可用性：卡片式预览行、无表头压缩列、卡片和右侧详情均保留材质槽编辑。

## v0.3.2

- UE 版本：`F:\UE_5.7`
- 编译项目：`E:\CodexOutputs\projects\ue-asset-material-batch-tool\package\HostProject\HostProject.uproject`
- 插件路径：`E:\CodexOutputs\projects\ue-asset-material-batch-tool\package\HostProject\Plugins\AssetMaterialBatchTool`
- 编译命令：`UnrealBuildTool.exe UnrealEditor Win64 Development -Project=... -plugin=... -NoEngineChanges -NoHotReloadFromIDE`
- 结果：`Succeeded`

本版重点验证了匹配策略和执行保护：优先按当前材质名匹配纹理，预览/执行显示实际参数绑定，没有可绑定纹理参数时跳过而不是假成功。

## v0.3.1

- UE 版本：`F:\UE_5.7`
- 编译项目：`E:\CodexOutputs\projects\ue-asset-material-batch-tool\package\HostProject\HostProject.uproject`
- 插件路径：`E:\CodexOutputs\projects\ue-asset-material-batch-tool\package\HostProject\Plugins\AssetMaterialBatchTool`
- 编译命令：`UnrealBuildTool.exe UnrealEditor Win64 Development -Project=... -plugin=... -NoEngineChanges -NoHotReloadFromIDE`
- 结果：`Succeeded`

本版重点验证了纹理绑定修复后的编译可用性：读取内容浏览器选中文件夹、单张纹理按父材质真实纹理参数绑定、执行阶段再次重映射参数。

## v0.3.0

- UE 版本：`F:\UE_5.7`
- 编译项目：`E:\CodexOutputs\projects\ue-asset-material-batch-tool\package\HostProject\HostProject.uproject`
- 插件路径：`E:\CodexOutputs\projects\ue-asset-material-batch-tool\package\HostProject\Plugins\AssetMaterialBatchTool`
- 编译命令：`UnrealBuildTool.exe UnrealEditor Win64 Development -Project=... -plugin=... -NoEngineChanges -NoHotReloadFromIDE`
- 结果：`Succeeded`

本版重点验证了 Slate 面板重构后的编译可用性：单一主按钮、隐藏空详情栏、移除底部重复状态栏、修正 `SOverlay` include 路径。
