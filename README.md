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
    - 实体标识：内部使用 `EntityKey = (EntityId + EntityUID + EntityType)` 作为可比较/可哈希的实体句柄
    - `EntityRef = (EntityUID + EntityType)` 作为轻量级引用，适用于不需要 EntityId 的场景
  - `src/io/`：模型文件读取服务（STEP/BREP）
  - `src/mesh/`：网格模块
    - 网格文档（MeshDocument）存储节点和单元数据
    - 网格动作（GenerateMeshAction）通过 Gmsh 进行网格剖分
    - 支持 MeshNode/MeshElement 实体类型的拾取与高亮
  - `src/render/`：渲染模块
    - 多遍渲染架构：GeometryPass（几何场景）、MeshPass（网格线框/节点）、PickingPass（拾取）、HighlightPass（高亮描边）
    - RenderBatch 管理 GPU 缓冲区（面/边/顶点/网格单元/网格节点）
    - RenderSceneController 桥接几何文档与网格文档的渲染数据，并管理 per-part 显隐状态
    - 渲染架构详细文档见 `docs/render_architecture.md`
- `resources/qml/`：QML UI（主窗口、页面、工具条等）
- `test/`：单元测试（默认关闭，见 CMake 选项）

## 依赖
- CMake >= 3.14
- Qt 6.8（组件：Core/Gui/Qml/Quick/OpenGL）
- OpenCASCADE（必须预装，CMake 通过 `find_package(OpenCASCADE REQUIRED)` 查找）
- GMesh（用于网格剖分与处理，需预装）:
```json
{
    "cmake.configureArgs": [
        "-DENABLE_BUILD_DYNAMIC=ON",
        "-DENABLE_OCC=ON",
        "-DENABLE_FLTK=OFF",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_INSTALL_PREFIX=\"D:/WorkSpace/OpenSource/GMesh/gmsh_4_15_0-debug\"",
        "-DENABLE_TESTS=OFF",
        "-DENABLE_OPENMP=OFF"
    ],
    "search.useIgnoreFiles": false
}
```
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

近期新增：
- EntityIndex 重构为按 type 分槽数组，提供 O(1) uid 查找和 id↔(uid,type) 快速映射
- 网格模块：MeshDocument 存储/查询网格节点与单元，GenerateMeshAction 通过 Gmsh 从几何面生成网格
  - 支持三角形/四边形/自动单元类型选择
  - 支持 2D 表面网格和 3D 体网格生成
  - 新增 `query_mesh_entity_info` 动作，支持查询网格节点/单元的详细信息（邻接关系、坐标等）
  - 新增 MeshQueryPage 页面用于展示网格实体信息
- 渲染管线支持网格数据（线框/节点）的可视化、拾取与高亮
  - MeshPass 独立渲染 FEM 网格单元和节点，与几何渲染解耦
  - 面选中轮廓修复：从实体质心缩放，而非世界原点
  - SceneRenderer 重构为 ISceneRenderer 接口 + 组件工厂模式
  - 渲染信号整合：移除冗余的 `subscribeCameraChanged`，统一使用 `subscribeSceneNeedsUpdate`
- SelectManager 支持 `mesh_node`/`mesh_element` 拾取类型
- 侧边栏 per-part 几何/网格显隐开关（通过 ViewportService 和 RenderSceneController 管理）
- QML 页面关闭后自动恢复 OpenGL 视口焦点

## 后续开发任务（来自 plan.md，做了轻度工程化拆分）
- 几何交互：选择（点/边/面/Part）、高亮、拾取、变换与编辑操作（trim/offset 等）
- 网格：面网格剖分、网格质量指标、平滑/修复工具链
- 渲染：更一致的光照（可能引入 PBR/IBL）、选择高亮/描边、可视化辅助（法线/网格）
- IO：导入进度与错误回传更完整；导出 STEP/BREP/网格格式
- AI：网格质量诊断与修复建议、交互式问答与操作编排

## 代码规范与注释
- Doxygen 注释风格参考：`doxygen_comment_style.md`
