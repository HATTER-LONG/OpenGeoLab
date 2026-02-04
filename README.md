# OpenGeoLab

English docs: `README.en.md` and `docs/json_protocols.en.md`.

OpenGeoLab 是一个基于 Qt Quick(QML) + OpenGL 的几何/模型可视化与编辑原型项目：
- 读取 BREP / STEP(STP) 等模型文件
- 基于 OpenCASCADE(OCC) 管理几何拓扑（点/边/面/Part）
- OpenGL 渲染显示，并提供基础视口交互（旋转/平移/缩放、视图预设、Fit）
- 支持鼠标拾取几何实体（点/边/面/实体/Part），实现类似 CAD 软件的选择交互
- 后续将支持网格剖分、以及 AI 辅助网格质量诊断与修复

## 主要功能

### 几何显示与交互
- 支持 STEP/BREP 模型导入与显示
- 基础几何体创建（长方体、圆柱、球体、圆环等）
- 视口控制：旋转（Ctrl+左键）、平移（Shift+左键/中键）、缩放（右键/Ctrl+滚轮）
- 视图预设：前/后/左/右/上/下、自适应

### 实体拾取
- 支持点、边、面、实体、Part 类型的拾取
- 吸附拾取：在点击位置周围自动搜索最近的实体
- 类型过滤：可限制只拾取特定类型的实体
- 选择管理：支持多选、移除、清空操作

### 实体查询
- 查询实体详细信息（类型、名称、所属 Part、包围盒等）
- 支持单个或批量实体查询

## 目录结构（约定）
- `include/`：对外接口头文件；不同模块应只通过此处暴露的接口互相调用
- `src/`：模块实现
  - `src/app/`：应用入口、QML 单例后端（BackendService）、OpenGL 视口（GLViewport）
  - `src/geometry/`：几何文档/实体、OCC 形体构建与导出渲染数据
  - `src/io/`：模型文件读取服务（STEP/BREP）
  - `src/render/`：渲染数据结构、SceneRenderer、RenderSceneController 等
- `resources/qml/`：QML UI（主窗口、页面、工具条等）
- `test/`：单元测试（默认关闭，见 CMake 选项）

## 依赖
- CMake >= 3.14
- Qt 6.8（组件：Core/Gui/Qml/Quick/OpenGL）
- OpenCASCADE（必须预装，CMake 通过 `find_package(OpenCASCADE REQUIRED)` 查找）
- HDF5（HighFive 需要系统预装 HDF5；若未用到可后续做成可选依赖）
- Ninja（推荐）+ MSVC（Windows）

项目使用 CPM 拉取部分依赖：Kangaroo、nlohmann/json、HighFive 等。

## 构建（Windows 示例）

> 以下以 Ninja + MSVC Developer Prompt 为例。

1) 配置

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

如果 OpenCASCADE 未能被自动找到，请按你的安装方式设置 `OpenCASCADE_DIR` 或相关 CMake 变量（取决于你的 OCC 安装包导出的配置）。

2) 编译

```powershell
cmake --build build
```

3) 运行

可执行文件输出到：`build/bin/OpenGeoLabApp.exe`

## JSON 协议（QML ↔ C++）
- 统一入口：QML 调用 `BackendService.request(module, JSON.stringify(params))`
- 服务端分发：各模块 `*Service::processRequest()` 按 `action` 路由到对应 Action

信号约定：
- `operationFinished(moduleName, actionName, result)`
- `operationFailed(moduleName, actionName, error)`

完整协议清单见：`docs/json_protocols.md`

## 后续开发任务（来自 plan.md，做了轻度工程化拆分）
- 几何交互：选择（点/边/面/Part）、高亮、拾取、变换与编辑操作（trim/offset 等）
- 网格：面网格剖分、网格质量指标、平滑/修复工具链
- 渲染：更一致的光照（可能引入 PBR/IBL）、选择高亮/描边、可视化辅助（法线/网格）
- IO：导入进度与错误回传更完整；导出 STEP/BREP/网格格式
- AI：网格质量诊断与修复建议、交互式问答与操作编排

## 代码规范与注释
- Doxygen 注释风格参考：`doxygen_comment_style.md`
