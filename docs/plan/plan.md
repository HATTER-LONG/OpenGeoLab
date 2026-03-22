# OpenGeoLab 开发大纲

## 项目目标

构建一个基于 Modern OpenGL 3.3+ Core Profile 的 3D 建模应用，具备：

- **3D 渲染**：自研轻量场景图 + Shader Pipeline，显示实体面 / 线框 / 顶点
- **交互导航**：四元数轨迹球旋转、平移、缩放，相机状态可序列化
- **图元拾取**：纹理 ID 拾取（离屏 FBO），支持点选 vertex/edge/face 和框选
- **网格剖分**：集成 GMSH，支持几何体的网格生成与参数化控制
- **LLM 分析**：接入大语言模型，进行几何清理建议与网格质量优化任务分析
- **命令架构**：JSON request → response 管道，录制与回放
- **Python 脚本**：pybind11 嵌入，插件发现，脚本导出
- **插件 UI**：PySide6 插件窗口系统，Python 插件可提供独立 GUI 面板
- **现代 UI**：QML Ribbon + Theme + Sidebar + 3D Viewport + Activity Panel

## 技术栈

C++20 · Qt 6.9 / QML · Modern OpenGL 3.3+ · GLM · GMSH · Kangaroo · pybind11 · PySide6 · nlohmann_json · spdlog · doctest · CPM.cmake

## 模块分层

```
app (QML UI + GLViewportItem)
 ├── command (ModuleDispatcher + Actions + RequestRecorder)
 │    ├── render (Camera + Scene Graph + GL Renderer + Picking)
 │    ├── geometry (PointStore + BoundingBox)
 │    └── meshing (GMSH wrapper + mesh quality metrics)
 ├── python (pybind11 wrapper + runtime + PySide6 plugin UI)
 ├── llm (LLM client + geometry cleaning / mesh optimization analysis)
 └── base (CommandResult + IAction + ResponseBuilder)
```

---

## 开发阶段

### Phase 1：QML 界面骨架

搭建完整 UI 骨架，3D 视口暂用占位色块。

| 模块 | 内容 |
|------|------|
| AppTheme | Light/Dark 双色系，颜色/间距/圆角常量集中管理 |
| Ribbon Header | 标签页切换（Geometry / Mesh / AI），每页含 action 按钮组 |
| Sidebar | 场景树、属性面板、状态信息 |
| ViewportPanel | 3D 视口容器（先用纯色 Rectangle 占位） |
| Activity Panel | 右下角操作日志，可展开收起 |
| Main.qml | Window 1500×900，渐变背景 + ColumnLayout 布局 |

**产出：** 可运行的 QML 应用，UI 布局和交互完整，视口为占位。

### Phase 2：渲染核心（纯 C++ / 无 GL 依赖）

纯数学和数据结构层，可独立测试。

| 模块 | 内容 |
|------|------|
| Camera | glm::quat orientation，view/projection matrix，orbit/pan/zoom/view_all |
| CameraState | JSON 序列化快照，兼容脚本录制 |
| Trackball | Arcball 球面 + 双曲面投影 → 四元数旋转 |
| NavigationController | 纯数学 pan/zoom 计算 |
| SceneNode 树 | id, parent/children, world_transform 递归累积 |
| MeshData | 顶点 (position + normal) + face_indices + edge_indices |
| MeshNode | 挂载 MeshData + Material + DisplayMode |
| 图元生成器 | Box / Cylinder / Sphere → MeshData |

**产出：** render 库（scene_graph + camera + trackball + primitives），全部有 doctest 覆盖。

### Phase 3：OpenGL 渲染 + 拾取

依赖 OpenGL Context（Qt），将 Phase 2 的数据送上 GPU。

| 模块 | 内容 |
|------|------|
| ShaderProgram | GLSL 编译/链接/uniform，RAII |
| VertexArray | VAO/VBO/EBO 封装，upload MeshData → GPU |
| Framebuffer | FBO + color/depth attachment，resize |
| GLRenderer | 遍历场景图 → MVP → 多 pass 渲染 (face + edge + vertex)，Polygon Offset |
| PickRenderer | 离屏 FBO 渲染唯一 ID，readPixels → pick_at / pick_rect |
| GLSL Shaders | phong (面 + 光照)、wireframe (线/点)、pick (ID 输出) |

**产出：** 完整 OpenGL 渲染管线 + 拾取，可被 QQuickFramebufferObject 调用。

### Phase 4：GLViewportItem 集成

将 Phase 2-3 接入 QML 视口。

| 模块 | 内容 |
|------|------|
| GLViewportItem | QQuickFramebufferObject 子类，承载 GLRenderer + PickRenderer |
| 鼠标交互 | 左键拖拽 → Trackball 旋转，中键 → 平移，滚轮 → 缩放 |
| 点选 / 框选 | 左键单击 → pick_at，Shift+拖拽 → pick_rect，结果推送到 Sidebar |
| displayMode | FlatLines / Wireframe 切换 |

**产出：** ViewportPanel 中嵌入可交互 3D 视口，替换 Phase 1 占位。

### Phase 5：后端复用（base + geometry + command）

从 v1 分支（`dev/v1-dev-latest-3-20`）复用已有代码。

| 模块 | 内容 |
|------|------|
| base | CommandResult, IAction, IModuleService, ResponseBuilder, RegistrationHelper |
| geometry | PointStore, BoundingBoxCalculator, GeometryModule |
| command | ModuleDispatcher, CommandDispatcher, ModuleRegistry, RequestRecorder |
| Render Actions | camera.get_state / set_state / view_all, scene.add_box / describe, display.set_mode |

**产出：** JSON 命令管道 + 渲染相关 action 可通过 ModuleDispatcher 调用。

### Phase 6：Python 嵌入 + PySide6 插件 UI

从 v1 复用 pybind11 wrapper + EmbeddedPythonRuntime，新增 PySide6 插件窗口系统。

| 模块 | 内容 |
|------|------|
| pybind11 wrapper | C++ 命令管道暴露给 Python，录制 + 回放 |
| EmbeddedPythonRuntime | 嵌入式 Python 初始化、venv 管理、插件发现 |
| PySide6 Plugin UI | 插件可通过 `launch_ui()` 弹出独立 PySide6 窗口/面板 |
| 插件协议 | `describe_plugin()` 元数据 + `launch_ui()` GUI 入口 + headless 降级 |

**产出：** Python 脚本可调用命令管道，插件可提供独立 GUI 面板。

### Phase 7：GMSH 网格剖分

集成 GMSH 库，通过命令管道提供网格生成能力。

| 模块 | 内容 |
|------|------|
| GMSH wrapper | C++ 封装 GMSH API，几何导入 → 网格生成 → MeshData 输出 |
| 网格参数 | 网格尺寸、算法选择、局部加密区域 |
| 网格质量 | 质量指标计算（aspect ratio, skewness），可视化映射 |
| Meshing Actions | mesh.generate / mesh.set_params / mesh.quality_report |

**产出：** 通过命令管道对几何体执行网格剖分，结果在 3D 视口中显示。

### Phase 8：LLM 几何清理与网格优化分析

接入大语言模型，辅助几何清理和网格质量优化。

| 模块 | 内容 |
|------|------|
| LLM Client | HTTP/API 调用封装，支持多种 LLM 后端（OpenAI / 本地模型） |
| 几何清理分析 | 将几何描述 + 拓扑问题发送给 LLM，获取清理建议和修复步骤 |
| 网格质量优化 | 将质量报告发送给 LLM，获取参数调优建议和局部加密策略 |
| LLM Actions | llm.analyze_geometry / llm.optimize_mesh / llm.chat |

**产出：** LLM 辅助用户分析几何问题和优化网格参数，建议以结构化结果展示在 Sidebar。

### Phase 9：联调与验收

端到端验证：Ribbon → Action → Command → Render → Viewport 全链路。

验证清单：
- 渲染 Box 并在视口中显示
- 旋转 / 平移 / 缩放操作流畅
- 点选 vertex / edge / face，框选多图元
- FlatLines / Wireframe 切换
- GMSH 网格剖分 → 3D 显示 + 质量指标
- LLM 分析几何清理 / 网格优化建议
- PySide6 插件窗口弹出 + 交互
- 相机状态录制 → Python 脚本导出 → 回放
- Light / Dark 主题切换
- Activity Panel 操作日志记录

---

## 构建命令

```bash
cmake -S . -B build
cmake --build build --config RelWithDebInfo --parallel 4
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

## 注意事项

1. **不使用 Coin3D** — 全部自研 OpenGL 渲染管线
2. **GLM** 作为数学库（header-only，CPM 管理）
3. render 库的场景图 / 相机 / 轨迹球是**纯 C++ 无 GL 依赖**，可独立测试
4. GL 封装在 `render/gl/` 子目录，QML 集成在 `app/GLViewportItem`
5. 优先 **RelWithDebInfo** 构建（Debug 模式 Windows 下缺部分依赖库）
