# Phase 2: OCC 几何内核集成 — 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 用 OpenCASCADE B-Rep 内核替换现有 `libs/geometry` 占位代码，实现四个基本体创建 → 三角化 → SceneGraph → Viewport 渲染的完整链路。

**架构：** libs/geometry（OCC PRIVATE）→ 写入 libs/scene SceneGraph → libs/render GeometryPass/WireframePass 读取并渲染。App 层负责实例化与 QML 桥接。所有模块纯 C++，Qt 仅在 app 层。

**技术栈：** C++20 · OpenCASCADE 7.9.2 · OpenGL 4.5 Core · Qt 6.9 / QML · nlohmann_json · doctest · CMake + Ninja

**规格文档：** `docs/superpowers/specs/2026-03-25-phase2-occ-geometry-design.md`

---

## 前置条件

用户需要完成以下步骤后才能开始本计划：

1. 构建 OCCT 7.9.2 的 RelWithDebInfo 版本（当前仅有 Debug 版本）
2. 将 `CMAKE_PREFIX_PATH` 或 `OpenCASCADE_DIR` 指向 RelWithDebInfo 安装目录
3. 重新运行 `cmake -S . -B build` 确认 OCC 能被 find_package 找到

---

## 文件变更清单

### 新增文件

| 文件 | 职责 |
|------|------|
| `src/libs/geometry/src/shape_store.hpp` | PRIVATE 头文件：ShapeInfo + ShapeStore（含 OCC 类型） |
| `src/libs/geometry/src/shape_store.cpp` | ShapeStore 实现 |
| `src/libs/geometry/src/tessellator.hpp` | PRIVATE 头文件：TessellationOptions + Tessellator |
| `src/libs/geometry/src/tessellator.cpp` | OCC → RenderMeshData 三角化实现 |
| `src/libs/geometry/src/occ_primitives.hpp` | PRIVATE 头文件：makeBox/makeCylinder/makeSphere/makeTorus |
| `src/libs/geometry/src/occ_primitives.cpp` | OCC 图元工厂函数实现 |
| `src/libs/render/include/opengeolab/render/vertex_array_object.hpp` | RAII VAO/VBO/EBO 封装 |
| `src/libs/render/src/vertex_array_object.cpp` | VAO 实现（GL 4.5 DSA） |
| `src/libs/render/include/opengeolab/render/geometry_pass.hpp` | Phong 实体面渲染 Pass |
| `src/libs/render/src/geometry_pass.cpp` | GeometryPass 实现 + Phong 着色器 |
| `src/libs/render/include/opengeolab/render/wireframe_pass.hpp` | 边线叠加渲染 Pass |
| `src/libs/render/src/wireframe_pass.cpp` | WireframePass 实现 + 线框着色器 |

### 修改文件

| 文件 | 变更内容 |
|------|---------|
| `CMakeLists.txt` | 添加 `OPENGEOLAB_USE_OCC` 选项 + `find_package(OpenCASCADE)` |
| `src/libs/geometry/CMakeLists.txt` | 重写：删旧源文件，添加新源文件 + OCC PRIVATE_LINKS |
| `src/libs/geometry/include/opengeolab/geometry/geometry_module.hpp` | 重写：新 GeometryModule 接口（带 SceneGraph 参数） |
| `src/libs/geometry/src/geometry_module.cpp` | 重写：OCC 创建 + 三角化 + SceneGraph 写入 |
| `src/libs/geometry/tests/geometry_module_test.cpp` | 重写：全新测试覆盖 ShapeStore/Tessellator/GeometryModule |
| `src/libs/render/CMakeLists.txt` | 添加 VAO、GeometryPass、WireframePass 源文件 |
| `src/libs/render/src/render_engine.cpp` | 注册 GeometryPass + WireframePass；添加 SceneGraph 脏检测 |
| `src/libs/render/include/opengeolab/render/render_engine.hpp` | 添加 `setSceneGraph()` 方法 |
| `src/app/src/main.cpp` | 创建 SceneGraph + ShapeStore + GeometryModule 实例；注册 QML 属性 |
| `src/app/src/gl_viewport_item.cpp` | 传递 SceneGraph 给 RenderEngine |
| `src/app/src/gl_viewport_item.hpp` (public) | 添加 SceneGraph 指针成员 |
| `src/app/CMakeLists.txt` | 添加 `OpenGeoLab::Geometry` 链接 |
| `src/libs/python/python_wrapper/src/python_wrapper_module.cpp` | 适配新 GeometryModule API |
| `src/libs/python/python_wrapper/CMakeLists.txt` | 可能需要添加 OpenGeoLab::Scene 链接 |

### 删除文件

| 文件 | 原因 |
|------|------|
| `src/libs/geometry/include/opengeolab/geometry/box_data.hpp` | 旧占位代码 |
| `src/libs/geometry/include/opengeolab/geometry/create_box_action.hpp` | 旧占位代码 |
| `src/libs/geometry/include/opengeolab/geometry/scene_store.hpp` | 旧占位代码 |
| `src/libs/geometry/src/create_box_action.cpp` | 旧占位代码 |
| `src/libs/geometry/src/scene_store.cpp` | 旧占位代码 |

### 稳定接口（不修改）

- `IRenderPass` / `RenderContext` / `PassManager` — 现有公共接口不变
- `SceneGraph` / `SceneNode` / `RenderMeshData` / `BoundingBox` — 仅增不减
- `INotificationSink` / `NotificationRegistry` — 不变
- `Camera` / `ShaderProgram` / `GridPass` — 不变

---

## 任务列表

### 任务 1：顶层 CMake OCC 集成

**文件：**
- 修改：`CMakeLists.txt`

- [ ] 步骤 1：在 `# Third-party` 段之后、`# Python` 段之前添加 OCC 查找逻辑：
  ```cmake
  option(OPENGEOLAB_USE_OCC "Enable OpenCASCADE geometry kernel" ON)
  if(OPENGEOLAB_USE_OCC)
      find_package(OpenCASCADE 7.8 REQUIRED)
      message(STATUS "OpenCASCADE found: ${OpenCASCADE_INSTALL_PREFIX}")
  endif()
  ```
- [ ] 步骤 2：运行 `cmake -S . -B build`（需要用户已设置 `CMAKE_PREFIX_PATH` 或 `OpenCASCADE_DIR`）确认找到 OCC
- [ ] 步骤 3：提交

**验证命令：** `cmake -S . -B build 2>&1 | findstr "OpenCASCADE"`
**预期结果：** 输出 `OpenCASCADE found: <path>`

---

### 任务 2：删除旧 geometry 占位代码

**文件：**
- 删除：`src/libs/geometry/include/opengeolab/geometry/box_data.hpp`
- 删除：`src/libs/geometry/include/opengeolab/geometry/create_box_action.hpp`
- 删除：`src/libs/geometry/include/opengeolab/geometry/scene_store.hpp`
- 删除：`src/libs/geometry/src/create_box_action.cpp`
- 删除：`src/libs/geometry/src/scene_store.cpp`
- 修改：`src/libs/geometry/CMakeLists.txt`（清空旧引用，暂保留骨架）
- 修改：`src/libs/geometry/include/opengeolab/geometry/geometry_module.hpp`（清空内容，保留文件）
- 修改：`src/libs/geometry/src/geometry_module.cpp`（清空内容，保留文件）
- 修改：`src/libs/geometry/tests/geometry_module_test.cpp`（清空测试，保留文件）

- [ ] 步骤 1：删除 5 个旧文件（box_data.hpp, create_box_action.hpp, scene_store.hpp, create_box_action.cpp, scene_store.cpp）
- [ ] 步骤 2：更新 CMakeLists.txt 移除旧文件引用，添加 OCC 依赖。新 CMakeLists.txt 内容：
  ```cmake
  set(geometry_public_headers
      include/opengeolab/geometry/geometry_module.hpp)

  set(geometry_sources
      src/geometry_module.cpp)

  opengeolab_add_module(
      opengeolab_geometry
      ALIAS_NAME Geometry
      SOURCES ${geometry_sources}
      PUBLIC_HEADERS ${geometry_public_headers}
      PUBLIC_LINKS OpenGeoLab::Base OpenGeoLab::Scene
      PRIVATE_LINKS nlohmann_json::nlohmann_json
                    TKernel TKMath TKBRep TKTopAlgo TKPrim TKMesh TKG3d)

  if(OPENGEOLAB_BUILD_TESTS)
      opengeolab_add_doctest_test(
          opengeolab_geometry_module_test
          SOURCES tests/geometry_module_test.cpp
          LINKS OpenGeoLab::Geometry nlohmann_json::nlohmann_json
                TKernel TKBRep TKPrim TKMath TKTopAlgo TKMesh TKG3d)
      target_include_directories(
          opengeolab_geometry_module_test
          PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  endif()
  ```
  **关键说明：** 测试 target 需要 `target_include_directories(... PRIVATE .../src)` 以 include PRIVATE 头文件（shape_store.hpp 等），并且需要直接链接 OCC 库以便测试创建 `TopoDS_Shape`。
- [ ] 步骤 3：写 geometry_module.hpp 最小骨架（空 class + process 声明），geometry_module.cpp 返回 `{"ok":false}` 的最小实现
- [ ] 步骤 4：清空 geometry_module_test.cpp 为单个占位测试
- [ ] 步骤 5：`cmake --build build --config RelWithDebInfo --parallel 4` 确认编译通过（注意：pywrapper 会编译失败，此处只验证 geometry 模块自身）
- [ ] 步骤 6：提交 `refactor(geometry): remove placeholder code and add OCC dependencies`

**验证命令：** `cmake --build build --target opengeolab_geometry --config RelWithDebInfo --parallel 4`
**预期结果：** geometry 模块编译成功

---

### 任务 3：实现 ShapeStore

**文件：**
- 新增：`src/libs/geometry/src/shape_store.hpp`
- 新增：`src/libs/geometry/src/shape_store.cpp`
- 修改：`src/libs/geometry/CMakeLists.txt`（添加新源文件）
- 修改：`src/libs/geometry/tests/geometry_module_test.cpp`（添加 ShapeStore 测试）

- [ ] 步骤 1：写 ShapeStore 失败测试（3 个测试用例）：
  - `ShapeStore addShape and retrieve`：addShape × 2 → getInfo 正确; allInfos 返回 2; shapeCount == 2
  - `ShapeStore removeShape`：addShape → removeShape → shapeCount == 0; removeShape 不存在 ID 返回 false
  - `ShapeStore clear`：addShape × 3 → clear → shapeCount == 0
- [ ] 步骤 2：运行测试，确认失败（编译失败 — 头文件不存在）
- [ ] 步骤 3：实现 shape_store.hpp — `ShapeInfo` 结构体 + `ShapeStore` 类声明（参考规格 §4.2）
  - `ShapeInfo`：id, sceneNodeId, label, faceMesh, edgeMesh, bounds
  - `ShapeStore`：addShape, setSceneNodeId, removeShape, getInfo, getShape, allInfos, shapeCount, clear
  - 内部：`mutex_`, `entries_`, `nextId_`
- [ ] 步骤 4：实现 shape_store.cpp
- [ ] 步骤 5：更新 CMakeLists.txt 添加 `src/shape_store.cpp`（PRIVATE 头文件不需要列在 PUBLIC_HEADERS 中）
- [ ] 步骤 6：运行测试，确认 3 个测试通过
- [ ] 步骤 7：提交 `feat(geometry): add ShapeStore with thread-safe shape CRUD`

**验证命令：** `ctest --test-dir build -C RelWithDebInfo -R geometry --output-on-failure`
**预期结果：** 3 个 ShapeStore 测试通过

**注意：** 测试中创建 `TopoDS_Shape` 需要使用 OCC 的 `BRepPrimAPI_MakeBox`。如果 OCC 头文件路径不在 test target 的 include 中，测试需要通过 geometry module 的 PRIVATE 头路径或直接 include OCC 头。推荐做法：测试文件 include 相对路径 `#include "shape_store.hpp"` 并在 CMake 中添加 `target_include_directories(test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)`。

---

### 任务 4：实现 Tessellator

**文件：**
- 新增：`src/libs/geometry/src/tessellator.hpp`
- 新增：`src/libs/geometry/src/tessellator.cpp`
- 修改：`src/libs/geometry/CMakeLists.txt`（添加 tessellator.cpp）
- 修改：`src/libs/geometry/tests/geometry_module_test.cpp`（添加 Tessellator 测试）

- [ ] 步骤 1：写 Tessellator 失败测试（3 个测试用例）：
  - `Tessellator tessellate box`：BRepPrimAPI_MakeBox → tessellate → positions 非空; indices 非空; topology == Triangles; 每 3 个 index 组成三角形; indices 在 vertex count 范围内
  - `Tessellator extractEdges box`：同上 → extractEdges → positions 非空; topology == Lines
  - `Tessellator computeBounds`：makeBox 2×2×2 at origin → bounds ≈ (0,0,0)~(2,2,2)（注意 OCC MakeBox 默认从 origin 出发，不是居中）
- [ ] 步骤 2：运行测试，确认失败
- [ ] 步骤 3：实现 tessellator.hpp — 静态方法：tessellate, extractEdges, computeBounds（参考规格 §4.3）
- [ ] 步骤 4：实现 tessellator.cpp：
  - `tessellate()`：`BRepMesh_IncrementalMesh` → `TopExp_Explorer(TopAbs_FACE)` → `BRep_Tool::Triangulation()` → 提取 positions/normals/indices
  - `extractEdges()`：`TopExp_Explorer(TopAbs_EDGE)` → `BRepAdaptor_Curve` → `GCPnts_UniformAbscissa` 采样为线段
  - `computeBounds()`：`Bnd_Box` + `BRepBndLib::Add()`
- [ ] 步骤 5：更新 CMakeLists.txt 添加 `src/tessellator.cpp`
- [ ] 步骤 6：运行测试，确认 3 个 Tessellator 测试通过
- [ ] 步骤 7：提交 `feat(geometry): add Tessellator for OCC shape triangulation`

**验证命令：** `ctest --test-dir build -C RelWithDebInfo -R geometry --output-on-failure`
**预期结果：** 之前 3 个 + 新 3 个 = 6 个测试通过

---

### 任务 5：实现 OCC 图元工厂函数

**文件：**
- 新增：`src/libs/geometry/src/occ_primitives.hpp`
- 新增：`src/libs/geometry/src/occ_primitives.cpp`
- 修改：`src/libs/geometry/CMakeLists.txt`（添加 occ_primitives.cpp）
- 修改：`src/libs/geometry/tests/geometry_module_test.cpp`（添加拓扑测试）

- [ ] 步骤 1：写图元拓扑失败测试（1 个测试用例，多个断言）：
  - `makePrimitive topology counts`：makeBox → TopExp_Explorer 验证 6 faces, 12 edges, 8 vertices; makeSphere → faces > 0; makeCylinder → faces > 0; makeTorus → faces > 0
- [ ] 步骤 2：运行测试，确认失败
- [ ] 步骤 3：实现 occ_primitives.hpp — 4 个工厂函数声明（参考规格 §4.4）
- [ ] 步骤 4：实现 occ_primitives.cpp：
  - `makeBox(center, size)`：计算 corner = center - size/2，`BRepPrimAPI_MakeBox(gp_Pnt, w, h, d)`
  - `makeCylinder(center, radius, height)`：`BRepPrimAPI_MakeCylinder(gp_Ax2, radius, height)`
  - `makeSphere(center, radius)`：`BRepPrimAPI_MakeSphere(gp_Pnt, radius)`
  - `makeTorus(center, majorRadius, minorRadius)`：`BRepPrimAPI_MakeTorus(gp_Ax2, major, minor)`
- [ ] 步骤 5：更新 CMakeLists.txt
- [ ] 步骤 6：运行测试，确认拓扑测试通过（总共 7 个测试）
- [ ] 步骤 7：提交 `feat(geometry): add OCC primitive factory functions`

**验证命令：** `ctest --test-dir build -C RelWithDebInfo -R geometry --output-on-failure`
**预期结果：** 7 个测试全通过

---

### 任务 6：实现新 GeometryModule（Phase 2a 完成）

**文件：**
- 修改：`src/libs/geometry/include/opengeolab/geometry/geometry_module.hpp`
- 修改：`src/libs/geometry/src/geometry_module.cpp`
- 修改：`src/libs/geometry/tests/geometry_module_test.cpp`（添加 GeometryModule 测试）

- [ ] 步骤 1：写 GeometryModule 失败测试（4 个测试用例）：
  - `GeometryModule create_box JSON`：构造 `SceneGraph graph; GeometryModule module(graph);` → JSON request → response ok; result.id > 0; `graph.root().children.size() == 1`
  - `GeometryModule create_cylinder JSON`：同上
  - `GeometryModule list_shapes`：create × 2 → list_shapes → response.result.count == 2
  - `GeometryModule unknown action`：返回 error JSON
  - **注意**：测试通过 `SceneGraph.root().children` 和 `list_shapes` JSON 响应验证数量，不直接访问 ShapeStore（ShapeStore 是内部实现细节）
- [ ] 步骤 2：运行测试，确认失败
- [ ] 步骤 3：实现 geometry_module.hpp（基于规格 §4.5，简化构造函数）：
  - `explicit GeometryModule(Scene::SceneGraph& graph)` — ShapeStore 作为内部成员持有
  - `process(string_view request_json, ModuleProgressCallback)` → string
  - `ModuleProgressCallback = std::function<void(double, string_view)>`
  - 注意：public header 不含 OCC 类型；ShapeStore 通过 pimpl 或 unique_ptr 隐藏（因为 ShapeStore 头文件含 OCC 类型，不能直接作为 public header 的成员类型）
  - **与规格偏离说明**：规格 §4.5 声明 `GeometryModule(ShapeStore& store)`，§6.2 扩展为 `GeometryModule(ShapeStore& store, SceneGraph& graph)`。此处简化为 `GeometryModule(SceneGraph& graph)` 并内部持有 ShapeStore。原因：ShapeStore 头文件包含 OCC 类型，若作为构造函数参数会泄漏 OCC 依赖到 app 层，违反 PRIVATE 隔离策略。
- [ ] 步骤 4：实现 geometry_module.cpp：
  - 分发 action：create_box / create_cylinder / create_sphere / create_torus / list_shapes
  - create_X 流程：解析 JSON → makeX() → tessellate/extractEdges/computeBounds → store_->addShape → graph_.addNode → store_->setSceneNodeId → notify → 返回 JSON
  - list_shapes 流程：store_->allInfos() → 序列化为 JSON
  - 使用 `notifyIfAvailable` 发送 geometry.status / geometry.data_changed
- [ ] 步骤 5：运行测试，确认 4 个新测试 + 之前 7 个 = 11 个全通过
- [ ] 步骤 6：运行完整构建 `cmake --build build --config RelWithDebInfo --parallel 4`（pywrapper 仍会失败 — 预期行为）
- [ ] 步骤 7：提交 `feat(geometry): implement OCC-backed GeometryModule with JSON dispatch`

**验证命令：** `ctest --test-dir build -C RelWithDebInfo -R geometry --output-on-failure`
**预期结果：** 11 个 geometry 测试全通过

---

### 任务 7：实现 VertexArrayObject

**文件：**
- 新增：`src/libs/render/include/opengeolab/render/vertex_array_object.hpp`
- 新增：`src/libs/render/src/vertex_array_object.cpp`
- 修改：`src/libs/render/CMakeLists.txt`（添加新源文件）

- [ ] 步骤 1：实现 vertex_array_object.hpp（参考规格 §5.1）：
  - RAII VAO + position VBO + normal VBO + EBO
  - `upload(const Scene::RenderMeshData& mesh)`
  - `draw() const`
  - `release()`
  - `isValid() const`
  - Move-only（delete copy ctor/assign）
- [ ] 步骤 2：实现 vertex_array_object.cpp：
  - `upload()`：使用 GL 4.5 DSA（`glCreateVertexArrays`, `glCreateBuffers`, `glNamedBufferStorage`, `glVertexArrayVertexBuffer`, `glVertexArrayAttribFormat`, `glVertexArrayElementBuffer`）
  - Attribute layout：location 0 = positions (vec3), location 1 = normals (vec3, optional)
  - `draw()`：`glBindVertexArray(vao_)` → `glDrawElements(drawMode_, indexCount_, GL_UNSIGNED_INT, nullptr)`
  - `release()`：`glDeleteVertexArrays` + `glDeleteBuffers`
  - Destructor 调用 release()；move 语义转移所有权
- [ ] 步骤 3：更新 CMakeLists.txt 添加源文件
- [ ] 步骤 4：编译验证 `cmake --build build --target opengeolab_render --config RelWithDebInfo --parallel 4`
- [ ] 步骤 5：提交 `feat(render): add VertexArrayObject RAII wrapper for GL 4.5 DSA`

**验证命令：** `cmake --build build --target opengeolab_render --config RelWithDebInfo --parallel 4`
**预期结果：** render 模块编译成功

**TDD 例外说明：** VAO 需要 OpenGL context 才能正确测试，无法在单元测试中验证。实际渲染验证推迟到任务 12 端到端测试。

---

### 任务 8：实现 GeometryPass

**文件：**
- 新增：`src/libs/render/include/opengeolab/render/geometry_pass.hpp`
- 新增：`src/libs/render/src/geometry_pass.cpp`
- 修改：`src/libs/render/CMakeLists.txt`

- [ ] 步骤 1：实现 geometry_pass.hpp（参考规格 §5.2）：
  - `struct Entry { Scene::RenderMeshData faceMesh; glm::mat4 transform{1.0F}; }`
  - `setup/execute/teardown` override
  - `setGeometry(std::vector<Entry> entries)`
  - Private：`ShaderProgram shader_`, `std::vector<VertexArrayObject> vaos_`, `bool dirty_`
- [ ] 步骤 2：实现 geometry_pass.cpp：
  - Phong vertex shader：MVP + normal transform → vNormal, vFragPos
  - Phong fragment shader：ambient(0.15) + diffuse + specular(32)，灯光方向 = camera 方向，材质颜色 `vec3(0.6, 0.7, 0.8)`
  - `setup()`：编译 shader（仅首次）
  - `execute()`：如果 dirty_ → 重新 upload VAOs；然后 draw
  - `setGeometry()`：存储 entries，标记 dirty_
  - 启用 `GL_DEPTH_TEST`，`glDepthFunc(GL_LESS)`
- [ ] 步骤 3：更新 CMakeLists.txt
- [ ] 步骤 4：编译验证
- [ ] 步骤 5：提交 `feat(render): add GeometryPass with Phong shading`

**验证命令：** `cmake --build build --target opengeolab_render --config RelWithDebInfo --parallel 4`
**预期结果：** 编译成功

---

### 任务 9：实现 WireframePass

**文件：**
- 新增：`src/libs/render/include/opengeolab/render/wireframe_pass.hpp`
- 新增：`src/libs/render/src/wireframe_pass.cpp`
- 修改：`src/libs/render/CMakeLists.txt`

- [ ] 步骤 1：实现 wireframe_pass.hpp（参考规格 §5.3）：
  - 结构与 GeometryPass 类似，Entry 使用 edgeMesh
  - Priority 300（GeometryPass 200 之后）
- [ ] 步骤 2：实现 wireframe_pass.cpp：
  - 简单 MVP vertex shader + 固定颜色 `vec3(0.1, 0.1, 0.1)` fragment shader
  - `execute()`：`glLineWidth(1.5F)` + `glEnable(GL_POLYGON_OFFSET_LINE)` + `glPolygonOffset(-1.0F, -1.0F)` depth offset
  - 如果 dirty_ → 重新 upload VAOs
- [ ] 步骤 3：更新 CMakeLists.txt
- [ ] 步骤 4：编译验证
- [ ] 步骤 5：提交 `feat(render): add WireframePass with edge overlay`

**验证命令：** `cmake --build build --target opengeolab_render --config RelWithDebInfo --parallel 4`
**预期结果：** 编译成功

---

### 任务 10：RenderEngine SceneGraph 集成（Phase 2b+2c）

**文件：**
- 修改：`src/libs/render/include/opengeolab/render/render_engine.hpp`
- 修改：`src/libs/render/src/render_engine.cpp`

- [ ] 步骤 1：在 render_engine.hpp 中添加：
  ```cpp
  #include <opengeolab/scene/scene_graph.hpp>
  // Forward declare GeometryPass/WireframePass
  void setSceneGraph(Scene::SceneGraph* graph);
  ```
  Private 成员：`Scene::SceneGraph* sceneGraph_ = nullptr; bool sceneDirty_ = true;`
- [ ] 步骤 2：在 render_engine.cpp `initialize()` 中注册 GeometryPass（priority 200）和 WireframePass（priority 300）
- [ ] 步骤 3：实现 `setSceneGraph()`：存储指针 + 注册 `onChanged` 回调（设置 sceneDirty_ = true）
- [ ] 步骤 4：在 `render()` 中添加脏检测逻辑：
  - 如果 `sceneDirty_ && sceneGraph_ != nullptr`：
    - 遍历 `sceneGraph_->root().children`
    - 按 `mesh.topology` 分类：Triangles → GeometryPass entries, Lines → WireframePass entries
    - 调用 `geometryPass->setGeometry()` 和 `wireframePass->setGeometry()`
    - `sceneDirty_ = false`
- [ ] 步骤 5：编译验证
- [ ] 步骤 6：提交 `feat(render): integrate SceneGraph with GeometryPass and WireframePass`

**验证命令：** `cmake --build build --target opengeolab_render --config RelWithDebInfo --parallel 4`
**预期结果：** 编译成功

---

### 任务 11：App 层布线 + pywrapper 修复（Phase 2d 核心）

**文件：**
- 修改：`src/app/src/main.cpp`
- 修改：`src/app/include/opengeolab/app/gl_viewport_item.hpp`
- 修改：`src/app/src/gl_viewport_item.cpp`
- 修改：`src/app/CMakeLists.txt`
- 修改：`src/libs/python/python_wrapper/src/python_wrapper_module.cpp`
- 修改：`src/libs/python/python_wrapper/CMakeLists.txt`

- [ ] 步骤 1：在 app CMakeLists.txt 添加 `OpenGeoLab::Geometry` 到 `target_link_libraries`
- [ ] 步骤 2：在 gl_viewport_item.hpp 添加 `void setSceneGraph(Scene::SceneGraph* graph)` 方法
- [ ] 步骤 3：在 gl_viewport_item.cpp 实现 `setSceneGraph` → 转发给 `renderEngine_.setSceneGraph(graph)`
- [ ] 步骤 4：在 main.cpp 中：
  - `#include <opengeolab/geometry/geometry_module.hpp>`
  - `#include <opengeolab/scene/scene_graph.hpp>`
  - 创建实例（GeometryModule 已在任务 6 中设计为内部持有 ShapeStore）：
    ```cpp
    OpenGeoLab::Scene::SceneGraph scene_graph;
    OpenGeoLab::Geometry::GeometryModule geometry_module(scene_graph);
    ```
  - 将 `geometry_module` 接入 `RequestService`（添加 geometry 模块路由）
  - 在 GLViewportItem 创建后调用 `setSceneGraph(&scene_graph)`
- [ ] 步骤 5：更新 pywrapper：
  - 创建静态 `SceneGraph` 实例
  - `static OpenGeoLab::Geometry::GeometryModule geometry_module{scene_graph};`
  - 移除旧 `SceneStore` 引用
- [ ] 步骤 6：更新 pywrapper CMakeLists.txt 添加 `OpenGeoLab::Scene` 链接
- [ ] 步骤 7：运行完整构建 `cmake --build build --config RelWithDebInfo --parallel 4`
- [ ] 步骤 8：运行完整测试 `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- [ ] 步骤 9：提交 `feat(app): wire SceneGraph through geometry, render, and pywrapper`

**验证命令：** `cmake --build build --config RelWithDebInfo --parallel 4 && ctest --test-dir build -C RelWithDebInfo --output-on-failure`
**预期结果：** 全量编译成功，所有测试通过

**线程安全简化说明：** 规格 §6.4 要求 App 层持有 `std::shared_mutex` 协调 geometry 写入与 render 读取。当前计划以功能正确性为优先，暂不加锁。端到端测试（任务 12）在单用户交互下执行，竞争窗口极窄。如果出现渲染抖动或崩溃，在任务 12 中追加 mutex 逻辑。

---

### 任务 12：端到端手动验证（Phase 2e）

**文件：** 无新文件变更

- [ ] 步骤 1：启动 app `build\bin\opengeolab_app.exe`
- [ ] 步骤 2：验证 Grid 渲染正常
- [ ] 步骤 3：通过 QML 或 Python 控制台发送 `create_box` JSON 请求
- [ ] 步骤 4：验证 Viewport 中出现实体面（蓝灰色 Phong） + 线框（深灰边线）
- [ ] 步骤 5：鼠标 orbit/pan/zoom 验证交互正常
- [ ] 步骤 6：发送 `create_sphere`、`create_cylinder`，验证多图元渲染
- [ ] 步骤 7：如果以上全部通过 → 提交 `docs(plans): mark Phase 2 core tasks as complete`

**验证方法：** 视觉检查
**预期结果：** 几何体在 Viewport 中可见，Grid 共存，交互正常

---

### 任务 13：代码质量检查

**文件：** 所有本计划新增/修改的文件

- [ ] 步骤 1：`clang-format -i` 所有新增/修改的 .hpp/.cpp 文件
- [ ] 步骤 2：`cmake-format -i` 所有修改的 CMakeLists.txt
- [ ] 步骤 3：`clang-tidy` 检查所有新增文件，修复 warning
- [ ] 步骤 4：确认 Doxygen 注释覆盖所有 PUBLIC 头文件中的公共类型和函数
- [ ] 步骤 5：提交格式化修正（如有）

**验证命令：** `clang-format --dry-run --Werror <files>` + `clang-tidy <files>`
**预期结果：** 无 format diff，无 tidy warning

---

## 依赖关系

```
任务 1 (CMake OCC) → 任务 2 (删旧代码) → 任务 3 (ShapeStore) → 任务 4 (Tessellator) → 任务 5 (Primitives) → 任务 6 (GeometryModule)
                                                                                                                              ↓
任务 7 (VAO) → 任务 8 (GeometryPass) → 任务 9 (WireframePass) → 任务 10 (RenderEngine SceneGraph)  →  任务 11 (App 布线)
                                                                                                              ↓
                                                                                                     任务 12 (E2E 验证)
                                                                                                              ↓
                                                                                                     任务 13 (代码质量)
```

注意：任务 7-9（render）可以与任务 3-5（geometry）**并行**开发，因为它们之间没有编译依赖。但任务 10 和 11 需要两条线都完成后才能开始。

---

## 估计影响

- 新增 ~12 个文件（6 geometry + 6 render）
- 修改 ~10 个文件
- 删除 5 个文件
- 新增 ~11+ 个测试用例
- 新增 2 个 GLSL shader pair（Phong + wireframe）

---

## 范围边界

本计划覆盖 Phase 2 的 **C++ 管线核心**（geometry + render + SceneGraph 联动 + app 布线 + pywrapper）。

**不在本计划范围内**（属于后续独立计划）：
- QML UI 组件：SceneTreeModel、CreateBoxDialog、SidebarPanel TreeView 升级（规格 §7.2-7.6）
- Ribbon 按钮绑定（规格 §7.6）
- PySide6 demo plugin UI 更新（规格 §7.5）
- SceneGraph `std::shared_mutex` 线程安全完善（规格 §6.4，本计划以单线程验证为主）

端到端验证（任务 12）通过 Python 控制台或已有 RequestService 路由发送 JSON 命令验证渲染，不依赖 QML 对话框。
