# Phase 2: OCC 几何内核集成 — 设计规格

> **状态**: Draft
> **范围**: libs/geometry OCC 重构 · GeometryPass/WireframePass · SceneGraph 联动 · QML 对话框 · 场景树
> **前置**: Phase 1 渲染基础设施已完成（Camera, ShaderProgram, PassManager, GridPass, RenderEngine, GLViewportItem）
> **OCC 版本**: OpenCASCADE 7.9.2（用户本地安装，`find_package(OpenCASCADE)` 集成）

---

## 1. 目标

用 OpenCASCADE B-Rep 内核替换现有 `libs/geometry` 占位代码（BoxData/SceneStore/GeometryModule），实现：

- 四个参数化基本体创建（Box / Cylinder / Sphere / Torus）
- OCC → RenderMeshData 三角化管线
- Viewport 实体渲染（GeometryPass + WireframePass）
- SceneGraph 数据联动
- QML 参数对话框与场景树

> **命名说明**: Roadmap 曾规划 `libs/occ`，但经讨论决定复用 `libs/geometry` 路径，以保持上层 CMake 引用和 pywrapper 命名的连续性。`geometry` 在语义上也更准确——模块职责是"几何管理"，OCC 是实现细节。

**验收标准**: QML 对话框 → OCC 创建几何体 → 三角化 → SceneGraph → Viewport 渲染可见。

---

## 2. 子阶段划分

| 子阶段 | 目标 | 主要模块 |
|--------|------|----------|
| **2a** | OCC 几何层 | libs/geometry（删旧 + 新建 OCC） |
| **2b** | GPU 渲染 Pass | libs/render（VAO + GeometryPass + WireframePass） |
| **2c** | SceneGraph 联动 | libs/scene ↔ geometry ↔ render 集成 |
| **2d** | App/QML 集成 | SceneTreeModel + CreateBoxDialog + SidebarPanel + pywrapper |
| **2e** | 端到端验证 | 全链路测试 + 代码质量检查 |

---

## 3. 全局架构

```
┌─────────────────────────────────────────────────────────────┐
│  QML UI Layer  (app/resource/qml/)                          │
│  Main.qml · CreateBoxDialog · SidebarPanel(TreeView)        │
└──────────────┬──────────────────────────────────────────────┘
               │ Qt 桥接层 (app/)
               │ GLViewportItem + ViewportController
               │ SceneTreeModel (QAbstractItemModel)
               ▼
┌──────────────────────────┐  ┌───────────────────────────────┐
│  libs/render/            │  │  libs/geometry/               │
│  (纯 C++, 无 Qt)         │  │  (纯 C++, 无 Qt)              │
│  RenderEngine            │  │  ShapeStore                   │
│  PassManager             │  │  Tessellator                  │
│  GeometryPass (NEW)      │  │  GeometryModule               │
│  WireframePass (NEW)     │  │  makeBox / makeCylinder / ... │
│  VertexArrayObject (NEW) │  │  OCC: PRIVATE                 │
│  GridPass (existing)     │  │                               │
└─────────┬────────────────┘  └───────────┬───────────────────┘
          │ 读取                           │ 写入
          ▼                               ▼
┌──────────────────────────────────────────────────────────────┐
│  libs/scene/   (纯 C++, 数据中间表示)                         │
│  SceneGraph · SceneNode · RenderMeshData · BoundingBox       │
└──────────────────────────────────────────────────────────────┘
```

### 依赖方向

```
libs/geometry → libs/scene (PUBLIC) + OpenCASCADE (PRIVATE) + libs/base (PUBLIC)
libs/render   → libs/scene (PUBLIC) + glad (PRIVATE)
app/          → libs/geometry + libs/render + libs/scene + Qt
```

### 关键约束

- `libs/geometry`、`libs/render`、`libs/scene` 均为**纯 C++**，不依赖 Qt。
- OpenCASCADE 头文件和链接仅出现在 `libs/geometry` 的 PRIVATE 层，不泄漏到公共接口。
- 所有模块通过 `opengeolab_add_module()` CMake 宏定义。
- C++20、`/** */` Doxygen、前置返回类型、遵循 `.clang-format` 和 `.clang-tidy`。

---

## 4. Phase 2a: libs/geometry — OCC 几何层

### 4.1 迁移计划

**先删后建**：完全移除旧 `libs/geometry` 的占位代码，然后从零构建 OCC 版本。

删除文件：
- `include/opengeolab/geometry/box_data.hpp`
- `include/opengeolab/geometry/create_box_action.hpp`
- `include/opengeolab/geometry/scene_store.hpp`
- `include/opengeolab/geometry/geometry_module.hpp`（将重建）
- `src/create_box_action.cpp`
- `src/scene_store.cpp`
- `src/geometry_module.cpp`（将重建）
- `tests/geometry_module_test.cpp`（将重建）

临时影响：
- pywrapper 编译失败（依赖旧 GeometryModule） — 2d 修复
- PySide6 demo_ui_plugin "Create Box" 不可用 — 2d 修复
- SidebarPanel box list 无数据源 — 2d 修复

### 4.2 ShapeStore — 线程安全 OCC 形状存储

```cpp
// include/opengeolab/geometry/shape_store.hpp
namespace OpenGeoLab::Geometry {

/**
 * @brief Metadata associated with a stored shape.
 */
struct ShapeInfo {
    int id = 0;                                 /**< Unique shape identifier. */
    int sceneNodeId = 0;                        /**< Corresponding SceneGraph node id. */
    std::string label;                          /**< Human-readable label. */
    Scene::RenderMeshData faceMesh;             /**< Tessellated face triangles. */
    Scene::RenderMeshData edgeMesh;             /**< Extracted edge lines. */
    Scene::BoundingBox bounds;                  /**< Axis-aligned bounding box. */
};

/**
 * @brief Thread-safe storage for OCC shapes and their tessellated representations.
 *
 * Assigns monotonic integer IDs to each stored shape.
 * All public methods are guarded by an internal mutex.
 */
class OPENGEOLAB_GEOMETRY_EXPORT ShapeStore {
public:
    /**
     * @brief Add a shape with its tessellated meshes.
     * @param shape OCC B-Rep shape (moved in, stored by value).
     * @param label Human-readable label.
     * @param faceMesh Tessellated face triangles.
     * @param edgeMesh Extracted edge lines.
     * @param bounds Shape bounding box.
     * @return Assigned unique shape ID (positive integer).
     */
    int addShape(TopoDS_Shape shape, std::string label,
                 Scene::RenderMeshData faceMesh, Scene::RenderMeshData edgeMesh,
                 Scene::BoundingBox bounds);

    /** @brief Set the corresponding SceneGraph node ID after addNode(). */
    void setSceneNodeId(int shapeId, int nodeId);

    /** @brief Remove a shape by ID. Returns true if found and removed. */
    bool removeShape(int id);

    /** @brief Retrieve shape info by ID. Throws std::out_of_range if not found. */
    [[nodiscard]] ShapeInfo getInfo(int id) const;

    /** @brief Retrieve the OCC shape by ID. Throws std::out_of_range if not found. */
    [[nodiscard]] TopoDS_Shape getShape(int id) const;

    /** @brief Snapshot of all stored shape infos. */
    [[nodiscard]] std::vector<ShapeInfo> allInfos() const;

    /** @brief Number of stored shapes. */
    [[nodiscard]] int shapeCount() const;

    /** @brief Remove all shapes. */
    void clear();

private:
    struct Entry {
        ShapeInfo info;
        TopoDS_Shape shape;
    };
    mutable std::mutex mutex_;
    std::vector<Entry> entries_;
    int nextId_ = 1;
};

} // namespace OpenGeoLab::Geometry
```

**注意**: `TopoDS_Shape` 仅出现在 `.cpp` 文件和 PRIVATE 头文件中。公共头文件使用前置声明或将 OCC 类型限制在 `.cpp` 中。

**修正**: 由于 `addShape` 和 `getShape` 需要 `TopoDS_Shape` 参数/返回值，`ShapeStore` 的头文件必须包含 OCC 头文件。因此 `ShapeStore` 不应出现在 PUBLIC_HEADERS 中，而应放在 PRIVATE 头文件中（内部实现细节）。

**公共接口方案**: 对外暴露的 `GeometryModule` 不暴露 OCC 类型。外部消费者只通过 JSON `process()` 接口或通过 `ShapeInfo`（不含 OCC 类型）获取数据。

### 4.3 Tessellator — OCC 三角化工具

```cpp
// include/opengeolab/geometry/tessellator.hpp (PRIVATE header)
namespace OpenGeoLab::Geometry {

/**
 * @brief Tessellation quality options.
 */
struct TessellationOptions {
    double linearDeflection = 0.1;   /**< Max chord height from surface. */
    double angularDeflection = 0.5;  /**< Max angular deflection in radians. */
    bool relative = true;            /**< Deflection relative to edge length. */
};

/**
 * @brief Converts OCC shapes to render-ready mesh data.
 *
 * Stateless utility. Uses BRepMesh_IncrementalMesh internally.
 */
class Tessellator {
public:
    /**
     * @brief Triangulate all faces of an OCC shape.
     * @param shape OCC B-Rep shape to tessellate.
     * @param options Quality parameters.
     * @return RenderMeshData with PrimitiveType::Triangles.
     */
    [[nodiscard]] static Scene::RenderMeshData
    tessellate(const TopoDS_Shape& shape, const TessellationOptions& options = {});

    /**
     * @brief Extract topological edge curves as line segments.
     * @param shape OCC B-Rep shape.
     * @return RenderMeshData with PrimitiveType::Lines.
     */
    [[nodiscard]] static Scene::RenderMeshData extractEdges(const TopoDS_Shape& shape);

    /**
     * @brief Compute axis-aligned bounding box from OCC shape.
     * @param shape OCC B-Rep shape.
     * @return Scene bounding box.
     */
    [[nodiscard]] static Scene::BoundingBox computeBounds(const TopoDS_Shape& shape);
};

} // namespace OpenGeoLab::Geometry
```

**实现要点**:
- `tessellate()` 使用 `BRepMesh_IncrementalMesh` 进行离散化
- 通过 `TopExp_Explorer` 遍历 `TopAbs_FACE`，从 `BRep_Tool::Triangulation()` 提取三角片
- 法线从三角化结果的 `Poly_Triangulation::Normal()` 获取
- `extractEdges()` 通过 `TopExp_Explorer(TopAbs_EDGE)` + `BRepAdaptor_Curve` + `GCPnts_UniformAbscissa` 采样
- `computeBounds()` 使用 `Bnd_Box` + `BRepBndLib::Add()`

### 4.4 OCC 图元工厂函数

```cpp
// include/opengeolab/geometry/occ_primitives.hpp (PRIVATE header)
namespace OpenGeoLab::Geometry {

/**
 * @brief Create an OCC box centered at the given position.
 * @param center Box center coordinates.
 * @param size Box dimensions (width, height, depth).
 * @return OCC solid shape.
 */
[[nodiscard]] TopoDS_Shape makeBox(std::array<double, 3> center,
                                   std::array<double, 3> size);

/**
 * @brief Create an OCC cylinder at the given position.
 * @param center Base center coordinates.
 * @param radius Cylinder radius.
 * @param height Cylinder height (along +Z axis).
 * @return OCC solid shape.
 */
[[nodiscard]] TopoDS_Shape makeCylinder(std::array<double, 3> center,
                                        double radius, double height);

/**
 * @brief Create an OCC sphere at the given position.
 * @param center Sphere center coordinates.
 * @param radius Sphere radius.
 * @return OCC solid shape.
 */
[[nodiscard]] TopoDS_Shape makeSphere(std::array<double, 3> center,
                                      double radius);

/**
 * @brief Create an OCC torus at the given position.
 * @param center Torus center coordinates.
 * @param majorRadius Distance from center to tube center.
 * @param minorRadius Tube radius.
 * @return OCC solid shape.
 */
[[nodiscard]] TopoDS_Shape makeTorus(std::array<double, 3> center,
                                     double majorRadius, double minorRadius);

} // namespace OpenGeoLab::Geometry
```

**实现要点**:
- `makeBox()` — `BRepPrimAPI_MakeBox` + `gp_Ax2` 定位
- `makeCylinder()` — `BRepPrimAPI_MakeCylinder`
- `makeSphere()` — `BRepPrimAPI_MakeSphere`
- `makeTorus()` — `BRepPrimAPI_MakeTorus`

### 4.5 GeometryModule — JSON 分发接口

```cpp
// include/opengeolab/geometry/geometry_module.hpp (PUBLIC header)
namespace OpenGeoLab::Geometry {

class ShapeStore;  // forward declaration

/** @brief Progress callback: (fraction 0.0–1.0, message). */
using ModuleProgressCallback = std::function<void(double, std::string_view)>;

/**
 * @brief Geometry module JSON command processor backed by OCC.
 *
 * Thread-safe. All commands serialised by internal mutex.
 *
 * Supported actions:
 * - "create_box": { center: [x,y,z], size: [w,h,d] } → { id, label }
 * - "create_cylinder": { center: [x,y,z], radius, height } → { id, label }
 * - "create_sphere": { center: [x,y,z], radius } → { id, label }
 * - "create_torus": { center: [x,y,z], majorRadius, minorRadius } → { id, label }
 * - "list_shapes": {} → { count, shapes: [{ id, label, bounds }] }
 */
class OPENGEOLAB_GEOMETRY_EXPORT GeometryModule {
public:
    explicit GeometryModule(ShapeStore& store);

    [[nodiscard]] std::string process(std::string_view request_json,
                                      const ModuleProgressCallback& progress_callback = {});

private:
    ShapeStore& store_;
    std::mutex mutex_;
};

} // namespace OpenGeoLab::Geometry
```

**通知协议**（与旧模式一致）:
- 创建图元前：`notify("geometry.status", { "event": "started", "description": "Creating box..." })`
- 创建图元过程中：`notify("geometry.progress", { "fraction": 0.5, "message": "Tessellating..." })`
- 创建成功后：`notify("geometry.data_changed", { "event": "data_changed", "count": N })`

### 4.6 CMake 配置

```cmake
# src/libs/geometry/CMakeLists.txt
find_package(OpenCASCADE REQUIRED)

set(geometry_public_headers
    include/opengeolab/geometry/geometry_module.hpp)

set(geometry_private_headers
    src/shape_store.hpp
    src/tessellator.hpp
    src/occ_primitives.hpp)

set(geometry_sources
    src/shape_store.cpp
    src/tessellator.cpp
    src/occ_primitives.cpp
    src/geometry_module.cpp)

opengeolab_add_module(
    opengeolab_geometry
    ALIAS_NAME Geometry
    SOURCES ${geometry_sources}
    PUBLIC_HEADERS ${geometry_public_headers}
    PUBLIC_LINKS OpenGeoLab::Base OpenGeoLab::Scene
    PRIVATE_LINKS nlohmann_json::nlohmann_json
                  TKernel TKMath TKBRep TKTopAlgo TKPrim
                  TKMesh TKG3d)
```

**OCC 组件**:
| 组件 | 用途 |
|------|------|
| TKernel | 基础类型（gp_Pnt, gp_Ax2 等） |
| TKMath | 数学工具 |
| TKBRep | B-Rep 数据结构（TopoDS_Shape, BRep_Tool） |
| TKTopAlgo | 拓扑算法（TopExp_Explorer） |
| TKPrim | 图元创建（BRepPrimAPI_MakeBox 等） |
| TKMesh | 三角化（BRepMesh_IncrementalMesh） |
| TKG3d | 3D 几何（Geom_Surface, BRepAdaptor） |

**OpenCASCADE 路径配置**: 通过 `CMAKE_PREFIX_PATH` 或 `OpenCASCADE_DIR` 指向用户本地安装。

### 4.7 文件结构

```
src/libs/geometry/
├── include/opengeolab/geometry/
│   └── geometry_module.hpp          (PUBLIC — 无 OCC 类型)
├── src/
│   ├── shape_store.hpp              (PRIVATE — 含 OCC 类型)
│   ├── shape_store.cpp
│   ├── tessellator.hpp              (PRIVATE — 含 OCC 类型)
│   ├── tessellator.cpp
│   ├── occ_primitives.hpp           (PRIVATE — 含 OCC 类型)
│   ├── occ_primitives.cpp
│   └── geometry_module.cpp
├── tests/
│   └── geometry_module_test.cpp
└── CMakeLists.txt
```

### 4.8 测试

| 测试用例 | 验证内容 |
|---------|---------|
| `ShapeStore addShape and retrieve` | addShape × 2 → getInfo 正确; allInfos 返回 2; shapeCount == 2 |
| `ShapeStore removeShape` | addShape → removeShape → shapeCount == 0; removeShape 不存在 ID 返回 false |
| `ShapeStore clear` | addShape × 3 → clear → shapeCount == 0 |
| `Tessellator tessellate box` | makeBox → tessellate → positions 非空; indices 非空; 每 3 个 index 组成三角形; indices 在 vertex 范围内 |
| `Tessellator extractEdges box` | makeBox → extractEdges → positions 非空; topology == Lines |
| `Tessellator computeBounds` | makeBox([0,0,0],[2,2,2]) → bounds ≈ (-1,-1,-1)~(1,1,1) |
| `makePrimitive topology counts` | makeBox → 6 faces, 12 edges, 8 vertices; makeSphere → faces > 0; etc. |
| `GeometryModule create_box JSON` | JSON request → response ok; result.id > 0; store.shapeCount == 1 |
| `GeometryModule create_cylinder JSON` | 同上 |
| `GeometryModule list_shapes` | create × 2 → list_shapes → count == 2, shapes 数组含 2 项 |
| `GeometryModule unknown action` | 返回 error JSON |

---

## 5. Phase 2b: libs/render — GeometryPass + WireframePass

### 5.1 VertexArrayObject — RAII VAO/VBO/EBO 封装

```cpp
// include/opengeolab/render/vertex_array_object.hpp
namespace OpenGeoLab::Render {

/**
 * @brief RAII wrapper for OpenGL VAO + VBO + EBO (optional normals).
 *
 * Uses GL 4.5 DSA (Direct State Access) for buffer creation and attribute setup.
 * Owns GL resources; releases them in destructor.
 */
class OPENGEOLAB_RENDER_EXPORT VertexArrayObject {
public:
    VertexArrayObject() = default;
    ~VertexArrayObject();

    VertexArrayObject(const VertexArrayObject&) = delete;
    VertexArrayObject& operator=(const VertexArrayObject&) = delete;
    VertexArrayObject(VertexArrayObject&& other) noexcept;
    VertexArrayObject& operator=(VertexArrayObject&& other) noexcept;

    /**
     * @brief Upload mesh data to GPU buffers.
     * @param mesh CPU-side mesh data. Replaces any existing data.
     *
     * Creates VAO, position VBO, optional normal VBO, and EBO.
     * Attribute layout: location 0 = positions (vec3), location 1 = normals (vec3).
     */
    void upload(const Scene::RenderMeshData& mesh);

    /** @brief Bind VAO and issue glDrawElements. */
    void draw() const;

    /** @brief Delete all GL resources. */
    void release();

    /** @brief True if upload() has been called and data is valid. */
    [[nodiscard]] bool isValid() const;

private:
    uint32_t vao_ = 0;
    uint32_t positionVbo_ = 0;
    uint32_t normalVbo_ = 0;
    uint32_t ebo_ = 0;
    int indexCount_ = 0;
    uint32_t drawMode_ = 0; // GL_TRIANGLES, GL_LINES, etc.
};

} // namespace OpenGeoLab::Render
```

### 5.2 GeometryPass — 实体面渲染

```cpp
// include/opengeolab/render/geometry_pass.hpp
namespace OpenGeoLab::Render {

/**
 * @brief Renders solid face meshes using Phong lighting.
 *
 * Priority 200 (after GridPass at 100).
 * Reads a list of geometry entries (mesh + transform) set via setGeometry().
 */
class OPENGEOLAB_RENDER_EXPORT GeometryPass : public IRenderPass {
public:
    /**
     * @brief Geometry entry for rendering: mesh data + world transform.
     */
    struct Entry {
        Scene::RenderMeshData faceMesh;
        glm::mat4 transform{1.0F};
    };

    void setup(int width, int height) override;
    void execute(const RenderContext& ctx) override;
    void teardown() override;

    /** @brief Set geometry entries to render. Marks VAOs dirty for re-upload. */
    void setGeometry(std::vector<Entry> entries);

private:
    ShaderProgram shader_;
    std::vector<Entry> entries_;
    std::vector<VertexArrayObject> vaos_;
    bool dirty_ = true;
};

} // namespace OpenGeoLab::Render
```

**Phong 着色器**:
- Vertex: `in vec3 aPosition, aNormal; uniform mat4 uModel, uView, uProjection; out vec3 vNormal, vFragPos;`
- Fragment: ambient(0.15) + diffuse + specular(32), 固定灯光方向 = camera 方向
- 默认材质颜色: `vec3(0.6, 0.7, 0.8)`（浅蓝灰）

### 5.3 WireframePass — 边线叠加渲染

```cpp
// include/opengeolab/render/wireframe_pass.hpp
namespace OpenGeoLab::Render {

/**
 * @brief Renders edge lines as wireframe overlay.
 *
 * Priority 300 (after GeometryPass at 200).
 * Uses glLineWidth and depth offset to render on top of faces.
 */
class OPENGEOLAB_RENDER_EXPORT WireframePass : public IRenderPass {
public:
    struct Entry {
        Scene::RenderMeshData edgeMesh;
        glm::mat4 transform{1.0F};
    };

    void setup(int width, int height) override;
    void execute(const RenderContext& ctx) override;
    void teardown() override;

    void setGeometry(std::vector<Entry> entries);

private:
    ShaderProgram shader_;
    std::vector<Entry> entries_;
    std::vector<VertexArrayObject> vaos_;
    bool dirty_ = true;
};

} // namespace OpenGeoLab::Render
```

**线框着色器**: 简单的 MVP 变换 + 固定颜色 `vec3(0.1, 0.1, 0.1)`（深灰线条）。

### 5.4 RenderContext 扩展

无需修改。当前 `RenderContext` 已包含 GeometryPass/WireframePass 所需的全部信息。

### 5.5 测试

| 测试用例 | 验证内容 |
|---------|---------|
| `VertexArrayObject upload` | 无 GL context 下测试接口编译; 实际 GL 测试需 offscreen context（可选） |
| `GeometryPass registration` | PassManager 注册 GeometryPass → passCount 正确 |
| `WireframePass registration` | PassManager 注册 WireframePass → passCount 正确 |

**注意**: GL 相关测试（VAO 上传、shader 编译、绘制）需要 offscreen GL context。Phase 2e 通过手动运行 app 验证实际渲染。

---

## 6. Phase 2c: SceneGraph 联动

### 6.1 SceneGraph 增强

当前 `SceneGraph` 已具备完整的 CRUD + `onChanged` 回调。无需修改接口。

### 6.2 GeometryModule → SceneGraph 写入

`GeometryModule` 构造函数增加 `SceneGraph&` 参数：

```cpp
GeometryModule(ShapeStore& store, Scene::SceneGraph& graph);
```

`process("create_box")` 内部流程：
1. OCC 创建 `TopoDS_Shape`
2. `Tessellator::tessellate()` → faceMesh
3. `Tessellator::extractEdges()` → edgeMesh
4. `Tessellator::computeBounds()` → bounds
5. `store_.addShape(shape, label, faceMesh, edgeMesh, bounds)` → shape_id
6. 构建 `SceneNode { name=label, type=Body, meshes=[faceMesh, edgeMesh], bounds }`
7. `int nodeId = graph_.addNode(node)` → 写入场景图，SceneGraph 分配 nodeId
8. `store_.setSceneNodeId(shape_id, nodeId)` → 回写映射，后续可通过 `ShapeInfo::sceneNodeId` 反查
9. `notify("geometry.data_changed", ...)`

**ID 映射策略**: `ShapeStore` 和 `SceneGraph` 各自独立分配 ID。`ShapeInfo::sceneNodeId` 字段记录对应的 SceneGraph 节点 ID，用于删除、选中、属性面板等反查场景。`GeometryModule` 负责在写入 SceneGraph 后立即回写此映射。

### 6.3 RenderEngine → SceneGraph 读取

`RenderEngine` 增加 `SceneGraph*` 引用：

```cpp
void setSceneGraph(Scene::SceneGraph* graph);
```

每帧 `render()` 时：
1. 如果 SceneGraph dirty（RenderEngine 内部维护 `dirty_` flag，由 `SceneGraph::onChanged` 回调置位）：
   - 遍历 `graph->root().children`
   - 收集所有 SceneNode 的 faceMesh（`topology == Triangles`）→ `GeometryPass::setGeometry()`
   - 收集所有 SceneNode 的 edgeMesh（`topology == Lines`）→ `WireframePass::setGeometry()`
2. 执行 PassManager 渲染

### 6.4 线程安全

- `ShapeStore` 内部 mutex 保护（geometry 操作可能在 worker thread）
- `SceneGraph` 操作在 geometry worker thread 完成写入后，通过 `onChanged` 回调通知
- RenderEngine 在 GL thread 读取 SceneGraph — 需要加锁或 copy-on-read
- **Phase 2c 简化方案**: 外部持有 `std::shared_mutex`（由 App 层创建），`GeometryModule` 写入时 `unique_lock`，`RenderEngine` 读取时 `shared_lock`。此 mutex 不放在 SceneGraph 内部，以保持 SceneGraph 接口不变。

### 6.5 测试

| 测试用例 | 验证内容 |
|---------|---------|
| `GeometryModule writes to SceneGraph` | create_box → graph.root().children.size() == 1; node.meshes 非空 |
| `Multiple shapes` | create_box + create_sphere → children.size() == 2 |

---

## 7. Phase 2d: App/QML 集成

### 7.1 SceneGraph 实例所有权

App 创建并持有 `SceneGraph` 实例，传递给 `GeometryModule` 和 `RenderEngine`。

可能的位置：
- `GLViewportItem` 持有 `SceneGraph` + `ShapeStore` + `GeometryModule`
- 或 `main.cpp` 注册为 QML singleton

**推荐**: `main.cpp` 中创建，注册为 QML context property，传递给需要的 C++ 组件。

### 7.2 SceneTreeModel

```cpp
// src/app/include/opengeolab/app/scene_tree_model.hpp

/**
 * @brief QAbstractItemModel exposing SceneGraph to QML TreeView.
 *
 * Monitors SceneGraph::onChanged to refresh model data.
 * Provides roles: name, entityType, visible, selected, nodeId.
 */
class SceneTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit SceneTreeModel(Scene::SceneGraph& graph, QObject* parent = nullptr);

    enum Roles {
        NameRole = Qt::UserRole + 1,
        EntityTypeRole,
        VisibleRole,
        SelectedRole,
        NodeIdRole
    };

    // QAbstractItemModel overrides
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void refresh();

private:
    Scene::SceneGraph& graph_;
};
```

### 7.3 CreateBoxDialog.qml

```
Popup (非模态, 300x200)
  ├── Column
  │   ├── Text "Create Box"
  │   ├── Row: Label "Center" + 3x SpinBox (x, y, z)
  │   ├── Row: Label "Size"   + 3x SpinBox (w, h, d)
  │   ├── Row
  │   │   ├── Button "Create" → submit JSON request
  │   │   └── Button "Cancel" → close popup
  └── All text wrapped in qsTr()
```

类似对话框也提供给 Cylinder、Sphere、Torus（可参数化复用基类组件）。

### 7.4 SidebarPanel 升级

从简单的 `ListView + BoxListItem` 改为 `TreeView + SceneTreeModel`:
- 显示 SceneNode 树结构（Body → Face/Edge）
- 点击选中联动（Phase 3）
- 右键菜单：删除、重命名（可选）

### 7.5 Pywrapper 更新

更新 `python_wrapper_module.cpp` 以适配新 GeometryModule API：
- 静态 `ShapeStore` 实例
- 静态 `GeometryModule` 实例（暂不传 SceneGraph — pywrapper 是独立进程或需要特殊处理）
- GIL release/acquire 保持不变

### 7.6 Ribbon 更新

Geometry tab 按钮（Create Box / Cylinder / Sphere / Torus）绑定到对应对话框的 `open()` 方法。

---

## 8. Phase 2e: 端到端验证

### 检查清单

- [ ] `cmake --build build --config RelWithDebInfo --parallel 4` — 全量编译通过
- [ ] `ctest --test-dir build -C RelWithDebInfo --output-on-failure` — 全部测试通过
- [ ] QML 对话框创建 Box → Viewport 中看到实体面 + 线框
- [ ] 创建多个不同图元 → 场景树显示所有节点
- [ ] 鼠标 orbit / pan / zoom 仍然正常
- [ ] Grid 与几何体共存渲染正常
- [ ] PySide6 demo plugin "Create Box" → 几何体出现在 Viewport
- [ ] clang-format 所有新增/修改文件
- [ ] cmake-format 所有 CMakeLists.txt
- [ ] clang-tidy 无 warning

---

## 9. OCC 构建配置

### 9.1 CMake 集成

顶层 CMakeLists.txt 增加：

```cmake
option(OPENGEOLAB_USE_OCC "Enable OpenCASCADE geometry kernel" ON)

if(OPENGEOLAB_USE_OCC)
    find_package(OpenCASCADE 7.8 REQUIRED)  # 最低兼容版本；用户当前使用 7.9.2
    message(STATUS "OpenCASCADE found: ${OpenCASCADE_INSTALL_PREFIX}")
endif()
```

用户通过 `CMAKE_PREFIX_PATH` 或 `OpenCASCADE_DIR` 指定 OCC 安装路径。

### 9.2 DLL 拷贝

Windows 上 OCC DLL 需要在运行时 PATH 中。使用现有的 `opengeolab_add_module` 的 DLL 拷贝机制，或在 CMake 中显式拷贝所需 OCC DLL 到输出目录。

需要的 DLL: `TKernel.dll`, `TKMath.dll`, `TKBRep.dll`, `TKTopAlgo.dll`, `TKPrim.dll`, `TKMesh.dll`, `TKG3d.dll` 以及它们的传递依赖。

---

## 10. 风险与缓解

| 风险 | 影响 | 缓解 |
|------|------|------|
| OCC Debug/Release 不匹配 | 链接失败或运行时崩溃 | 用户构建 RelWithDebInfo 版本 OCC |
| OCC DLL 路径 | 运行时找不到 DLL | post-build 拷贝或 PATH 设置 |
| Tessellator 输出质量 | 粗糙三角化 | 可调 linearDeflection / angularDeflection |
| SceneGraph 线程安全 | 渲染/几何竞争 | shared_mutex + 后续可升级为双缓冲 |
| 旧代码删除破坏 pywrapper | pywrapper 编译失败 | 2a 删除后 2d 统一修复 |

---

## 11. 边界稳定要求

以下接口在整个 Phase 2 中**不变**：

- `INotificationSink` 接口
- `NotificationRegistry` 静态 API
- `IRenderPass` 接口
- `RenderContext` 结构体
- `RenderMeshData` 结构体
- `SceneNode` / `SceneGraph` 现有接口（仅增不减）
- `PassManager` 公共方法
