## Plan: 重构实体索引与渲染模块（总体计划）

TL;DR — 目标一次性完成三大改造：1) 用按 type 分槽 + 世代计数实现新的 `EntityIndex`（支持 id->(uid,type) 映射并允许破坏性 API 变更）；2) 将拾取改为 32-bit 整数编码（GL_R32UI，编码高24位 uid + 低8位 type），并把渲染器重构为可插拔的 RenderCore/RenderPass/Renderable 架构以便扩展高亮/子元素高亮；3) 在 C++ 注册 `ViewportService` 给 QML 以统一弹窗 focus 行为，同时更新注释与文档。下列步骤包含具体文件/符号引用与验证方法，面向可执行实现。

**Steps**
1. 项目准备与约束确认  
- **编译要求**：在 CMakeLists.txt 中强制或检测 OpenGL ≥ 4.3，并确保 Qt 提供 `QOpenGLExtraFunctions`。  
- **运行时检测**：在渲染初始化处加入 GL feature 检查（整数纹理/着色器整数输出），失败时打印明确错误并退出或（仅用于开发）回退到字节编码临时路径。  

2. 实体索引重构（`EntityIndex`）  
- **新增类型与文件**：基于 `src/geometry/entity/entity_index.hpp` 与 `src/geometry/entity/entity_index.cpp`，实现分 type 的 `vector<Slot>`、slot 世代计数和自由列表复用。  
- **id 映射**：增加 `unordered_map<EntityId, PackedInfo>` 支持快速 id -> (uid,type) 查找。  
- **API 变更**：在 geometry_document.hpp 引入 `EntityKey` 与 `EntityRef` (geometry_types 中已有实现)，替换或新增 `findByKey()` / `findById()` 签名（允许返回轻量值/引用类型，或 `std::optional<std::reference_wrapper<const Entity>>`）。  
- **迁移策略**：因为允许破坏性变更，直接替换调用点；
- 主要修改参照：现有实现 entity_index.cpp 与声明 entity_index.hpp。

3. 渲染模块模块化改造（RenderCore + Passes + Renderable）  
- **架构新增/修改文件**：新增核心头文件 `include/render/renderer_core.hpp`、`include/render/render_pass.hpp`、`include/render/renderable.hpp`；修改并把 scene_renderer.hpp 作为 façade（短期保留外部 API）并在实现中委派到 `RendererCore`。  
- **核心职责划分**：  
  - `RendererCore`：管理 GL context、全局资源（ShaderPool、TexturePool、BufferPool）、FBO 管理与通道调度。  
  - `RenderPass`（可注册）：实现 `GeometryPass` / `PickingPass` / `HighlightPass` / `CompositePass`。  
  - `Renderable` / `RenderBatch`：封装具体可绘制对象（`MeshRenderable`、`EdgeRenderable`、`PointRenderable`、以及支持子实体的 `MeshElement`），并通过批处理/实例化降低 draw call。  
- **PickingPass（整数拾取）**：新增 `src/render/passes/picking_pass.cpp/hpp`，实现：  
  - 创建 GL_R32UI 颜色附件（使用 `QOpenGLExtraFunctions` 调用 `glTexStorage2D`/`glFramebufferTexture2D`），片段着色器输出 `uint` id；  
  - 提供编码函数 `uint32_t encodePickId(EntityType type, EntityUID uid)` 与 `decodePickId(uint32_t)`（编码 = `(uid & 0xFFFFFF) << 8 | (type & 0xFF)`）；  
  - 读取采用异步 PBO（`glReadPixels(..., GL_UNSIGNED_INT, ...)` into PBO，后映射）或在小区域时单像素读取以降低阻塞。替换现有读回逻辑（见 opengl_viewport.cpp 的 `glReadPixels(GL_UNSIGNED_BYTE)` 与 `decodeUid24`）。  
- **HighlightPass（可插拔策略）**：实现接口 `IHighlightStrategy` 并提供至少两种实现：Mask+Post-process Outline（最灵活）与 Instance-level Highlight（最高效）。把策略实现放到 `src/render/highlight/*`。  
- **数据组织**：引入 `RenderBatch`（静态 VBO/IBO + dynamic instance buffer），在实例 buffer 中包含 `instanceId` / `elementId` / `highlightFlags` 字段，支持 `glDrawElementsInstanced` 或 `MultiDrawIndirect`。  

4. 支持子实体高亮（mesh element）与扩展接口  
- **Element ID**：在 mesh 构建阶段为每个 face/edge/vertex 生成 `elementId`（32-bit），并把它映射到顶点/实例数据（可选通过 SSBO/texture buffer）。  
- **API**：在渲染层暴露 `SelectionSet`（包括 `EntityKey` 与 `ElementKey`），HighlightPass 使用该集合渲染高亮；上层（UI/工具）通过 `RenderService` / `SceneController` 提交选择。  
- **插件点**：`RendererCore::registerPass()` 与 `RendererCore::registerHighlightStrategy()`，第三方或未来模块可注入自定义高亮或可视化 pass（例如网格质量热图）。

5. 降低 GPU 使用与性能策略  
- **批处理与实例化**：合并相同材质/VAO 的对象进入 `RenderBatch`，使用 instanced draws。  
- **缓冲策略**：使用 persistent mapped buffers / ring buffer / orphaning 减少上传开销。实现 `BufferPool` 以复用 GPU 资源。  
- **剔除与 LOD**：CPU frustum culling + 可选 GPU occlusion query 批量剔除；为复杂 mesh 提供 LOD 层级。  
- **减少读回阻塞**：Picking 采用 PBO 异步读回，或延迟映射以避免 CPU stall。仅在需要拾取时才执行完整 PickingPass（局部 region 优先）。  
- **着色器管理**：统一 `ShaderProgramManager` 管理 permutations，避免频繁编译/切换。使用紧凑顶点格式与纹理 atlas。

6. QML focus 与服务化接口  
- **新增 `ViewportService`**：实现 `include/app/viewport_service.hpp` 与 `src/app/viewport_service.cpp`，提供 `Q_INVOKABLE void focusViewport()` 并注册到 QML（`qmlEngine->rootContext()->setContextProperty("ViewportService", ...)`）。  
- **QML 修改点**：修改 FunctionPageBase.qml 与 MainPages.qml，在 `open()` / `close()` 时调用 `ViewportService.focusViewport()`（替换或补充 `root.forceActiveFocus()`）。  
- 关联实现参考：opengl_viewport.hpp / opengl_viewport.cpp（拾取读回入口与 focus 行为）。

7. 注释、文档、测试与发布流程  
- **注释**：按 doxygen_comment_style.md 更新关键文件注释（`entity_index_v2.*`、`renderer_core.*`、`picking_pass.*`、`viewport_service.*` 等）。  
- **文档**：同步更新 json_protocols.md 与 README.md（中文为准），说明 picking 编码规则、API 变更点与迁移指引。  
- **测试**：增加单元/集成测试：  
  - `EntityIndexV2` 功能与并发测试；  
  - `encodePickId` / `decodePickId` round-trip 与 PickingPass 读回一致性（含 PBO）；  
  - HighlightPass 渲染效果与子实体选择验证；  
  - QML 集成测试（弹窗打开/关闭后视口响应）。  
- **性能验证**：量化指标（draw calls、GPU 带宽、pick latency、帧率）并记录基线对比。

**Verification（如何测试与验证）**
- 构建：  
  - 运行 CMake 并确保 OpenGL 4.3 检测通过：  
    ```bash
    cmake -S . -B build 
    cmake --build build -j
    ```  
- 功能验证：  
  - 打开含多个实体的大场景，执行随机拾取，核对 pick 返回的 `(uid,type)` 与实体表；  
  - 选择若干 mesh face/edge，验证高亮呈现（outline / instance highlight）；  
  - 打开/关闭 QML 弹窗，验证视口能持续接收鼠标事件。  
- 性能与回归：运行自动化测试并比较变更前后 draw calls、帧率与 pick latency；对大型场景进行压力测试（目标显著降低 draw calls 与 GPU 带宽）。

**Decisions**
- Pick 编码：已选定高24位 uid + 低8位 type（接受 24-bit 限制）。  
- OpenGL 要求：强制最低 OpenGL 4.3，使用 `QOpenGLExtraFunctions`。  
- API 兼容性：允许破坏性变更，直接替换 GeometryDocument 等返回类型（但保留短期兼容适配函数以便测试）。  
- 开发策略：一次性大改（单大 PR），但实现内部分清阶段提交（便于 review 与回退）。