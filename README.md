# OpenGeoLab

English docs: `README.en.md` and `docs/json_protocols.en.md`.

OpenGeoLab 是一个基于 Qt Quick(QML) + OpenGL 的几何/模型可视化与编辑原型项目：
- 读取 BREP / STEP(STP) 等模型文件
- 基于 OpenCASCADE(OCC) 管理几何拓扑（点/边/面/Part）
- OpenGL 渲染显示，并提供基础视口交互（旋转/平移/缩放、视图预设、Fit）
- 后续将支持鼠标编辑（如 trim/offset）、网格剖分、以及 AI 辅助网格质量诊断与修复

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
- 几何交互：~~选择（点/边/面/Part）、高亮、拾取~~ ✅（已完成 EntitySelector 控件及 QueryPage）、变换与编辑操作（trim/offset 等）
- 网格：面网格剖分、网格质量指标、平滑/修复工具链
- 渲染：更一致的光照（可能引入 PBR/IBL）、~~选择高亮/描边~~ ✅（已完成基于 HighlightState 的高亮渲染）、可视化辅助（法线/网格）
- IO：导入进度与错误回传更完整；导出 STEP/BREP/网格格式
- AI：网格质量诊断与修复建议、交互式问答与操作编排

## 功能特性

### 几何选择与查询
- **Select 控件**（EntitySelector.qml）：通用实体拾取组件
  - 支持选择模式过滤：Vertex（点）、Edge（边）、Face（面）、Solid（体）、Part（零件）
  - 悬停预览：鼠标悬停时临时高亮实体（青色）
  - 点击选择：单击选中实体（橙色高亮）
  - 框选支持：按住鼠标拖拽进行批量框选
  - 右键取消：右键清除所有选择
  - 支持多选及选择上限配置
- **Query 界面**：Geometry 选项卡中的查询功能页
  - 选择模式切换：None / Vertex / Edge / Face / Solid / Part
  - 已选实体列表：显示选中实体的类型和 ID
  - 实体详细信息查询：包括包围盒、中心点、尺寸、父子关系等

## 代码规范与注释
- Doxygen 注释风格参考：`doxygen_comment_style.md`
