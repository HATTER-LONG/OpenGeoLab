# OpenGeoLab

English docs: `README.en.md` and `docs/json_protocols.en.md`.

OpenGeoLab 是一个基于 Qt Quick(QML) + OpenGL 的几何/模型可视化与编辑原型项目：
- 读取 BREP / STEP(STP) 等模型文件
- 基于 OpenCASCADE(OCC) 管理几何拓扑（点/边/面/Wire/Part）
- OpenGL 渲染显示，使用正交投影，默认不透明渲染（通过预乘 alpha 确保 Qt Quick 合成时完全不透明），支持 X-ray 模式（面片半透明以查看遮挡边线，同时作用于几何和网格；关闭时通过深度偏移正确遮蔽背面线框），并提供基础视口交互（旋转/平移/缩放、视图预设、Fit）
- 支持多层级实体拾取（Vertex/Edge/Face/Wire/Part），Wire 和 Part 模式可自动解析子实体到父级；拾取高亮精准匹配当前拾取类型（如 Edge 模式下仅高亮选中的边，Wire 模式下不会错误高亮 Face）；Wire 高亮包含所有组成边（含多面共享边），呈完整闭环显示
- 网格实体拾取（Node/Line/Element），支持 hover 高亮和选中高亮，基于 GPU 着色器的 pickId 比对实现；即使在线框模式下，被 hover/选中的网格元素也会通过 highlight-only 面片覆盖渲染显示高亮效果；MeshLine 作为正式 MeshElement 对象存储，使用 `generateMeshElementUID(Line)` 生成类型作用域唯一 ID，在网格生成阶段自动从 2D/3D 元素边提取并去重创建
- 网格支持 node↔line↔element 三者互相关联查询，查询结果包含关联实体引用
- 网格剖分后可渲染为线框+节点、面片+点、面片+点+边三种显示模式轮询切换（默认：线框+节点）
- 网格面片颜色继承自所属 Part 的颜色（调暗显示），与几何面颜色有明显区分
- Wire 拾取在多面共享边时，自动优先选择光标所在面对应的 Wire（基于拾取区域中的 Face 上下文消歧义）
- 后续将支持鼠标编辑（如 trim/offset）、网格质量诊断、以及 AI 辅助修复

## 目录结构（约定）
- `include/`：对外接口头文件；不同模块应只通过此处暴露的接口互相调用
- `src/`：模块实现
  - `src/app/`：应用入口、QML 单例后端（BackendService）、OpenGL 视口（GLViewport）
  - `src/geometry/`：几何文档/实体、OCC 形体构建与导出渲染数据
    - 实体标识：内部使用 `EntityKey = (EntityId + EntityUID + EntityType)` 作为可比较/可哈希的实体句柄
  - `src/mesh/`：网格文档/节点/单元管理、FEM 数据存储
  - `src/io/`：模型文件读取服务（STEP/BREP）
  - `src/render/`：渲染数据结构、SceneRenderer、RenderSceneController、GPU 拾取（PickPass）
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
- Geo Query 页面通过 `GeometryService` 的 `query_entity_info` action，基于当前拾取到的实体 (type+uid) 查询实体详细信息并在页面下方列表展示。
- Mesh Query 页面支持 Node/Line/Element 拾取查询，通过 GPU PickPass 实现 GL_POINTS/GL_LINES/GL_TRIANGLES 分别绘制到离屏 FBO 进行像素级拾取。
- X-ray 模式同时作用于 GeometryPass 和 MeshPass，面片使用预乘 alpha 混合半透明渲染（`GL_ONE, GL_ONE_MINUS_SRC_ALPHA`），确保 Qt Quick 合成时颜色正确；关闭时通过 `GL_POLYGON_OFFSET_FILL` 确保面片略偏移，可见面边线通过深度测试、背面边线被正确遮挡。
- 渲染前完整重置 GL 状态（glColorMask/glDepthMask/glBlendFunc 等），防止 Qt 场景图遗留状态导致半透明。
- 正交投影使用对称近/远裁切平面（负近平面），避免模型缩放时被裁切。
- Wire 高亮使用 wire→edges 逆向映射表，hover/选中时高亮组成 wire 的所有 edge（含多面共享边），呈完整闭环。
- 网格线框 MeshLine 重构为正式 MeshElement 对象，使用 `generateMeshElementUID(MeshElementType::Line)` 生成类型作用域唯一 ID；在 `buildEdgeElements()` 阶段自动从 2D/3D 元素边提取并去重创建 Line 元素，同时构建 node↔line↔element 关系映射表。
- MeshLine 拾取现直接使用 Line 元素的 UID 作为 pickId，不再使用复合节点对编码，查询结果包含 `relatedElements`/`relatedLines` 关联引用。
- PickPass::renderToFbo 参数重构为 GeometryPickInput/MeshPickInput 结构体，减少参数数量。
- 网格显示模式改为三档轮循：线框+节点 → 面片+点 → 面片+点+边（默认线框+节点）。
- 网格面片颜色统一使用所属 Part 颜色（调暗 35%+偏移），与几何面颜色明显区分。
- 新增网格 hover/选中高亮，通过着色器中的 pickId uniform 比对实现。
- Wire 拾取增加 Face 上下文消歧义：多面共享边时，优先选择光标所在面对应的 Wire（pick region 中同时读取 Edge 和 Face，按 Face 归属确定 Wire）。
- Wire 拾取模式下 hover/click 过滤：若最高优先级命中为 Face（而非 Edge），则清除 hover 或忽略 click，避免 Wire 模式误高亮整个面。
- 网格元素 hover/选中高亮：即使显示模式为纯线框，也会渲染被高亮的网格元素面片（u_highlightOnly 模式丢弃非高亮片段）。
- X-ray 切换按钮增加视觉状态指示（按下/激活态高亮显示）。
- MeshLine 查询直接通过 MeshDocument 的 `findElementByRef()` 查询 Line 元素，结果包含 relatedElements（共享该边的面/体元素）。

## 后续开发任务（来自 plan.md，做了轻度工程化拆分）
- 几何交互：选择（点/边/面/Part）、高亮、拾取、变换与编辑操作（trim/offset 等）
- 网格：面网格剖分、网格质量指标、平滑/修复工具链
- 渲染：更一致的光照（可能引入 PBR/IBL）、选择高亮/描边、可视化辅助（法线/网格）
- IO：导入进度与错误回传更完整；导出 STEP/BREP/网格格式
- AI：网格质量诊断与修复建议、交互式问答与操作编排

## 代码规范与注释
- Doxygen 注释风格参考：`doxygen_comment_style.md`
