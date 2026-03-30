# Scene Graph + Render Module Design Specification

**日期**: 2026-03-30
**状态**: Draft
**范围**: `opengeolab_scene` + `opengeolab_render` + App 集成层

---

## 1. 目标与约束

### 1.1 目标

为 OpenGeoLabNew 新增场景图管理和 OpenGL 渲染能力：

- **场景图**：通用节点树 + 组合式组件，承载 CAD 几何、FEM 网格、标注等多种数据源
- **渲染**：基于 OpenGL 4.6 + glad 的多 Pass 前向渲染管线，不依赖 Qt
- **拾取**：GPU 颜色拾取（初期）+ 射线拾取接口预留
- **可见性控制**：节点级 visible/hidden + 显示模式（Solid / Wireframe / Points）过滤
- **VBO 批次优化**：flat buffer + DrawRange + glMultiDrawElements

### 1.2 约束

| 约束 | 说明 |
|------|------|
| OpenGL 版本 | glad 加载 4.6 context，初期使用 3.3 级别 API，后续渐进升级 DSA |
| 不依赖 Qt | scene lib 和 render lib 不链接 Qt；仅 app 层使用 Qt |
| 依赖 geometry | scene lib 依赖 geometry lib，通过 GeometrySceneBridge 自动同步 ShapeStore |
| 线程模型 | SceneGraph 使用读写锁保护；渲染线程在 synchronize 阶段持读锁同步 GPU 数据，拾取结果缓存到 Renderer 本地，在下次 synchronize() 时（GUI 线程已 blocked）回写 SceneGraph，避免渲染线程直接持写锁 |
| 顶点格式 | 40B 主 VBO (pos+normal+color) + 8B Pick VBO (pickId)，分离存储 |
| App 集成 | QQuickFramebufferObject 方式，渲染在 Qt Scene Graph 渲染线程执行 |

### 1.3 参考

- OGL 仓库 (`C:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OGL`) 的 render 模块
- OpenGeoLabNew 现有 VisualData / EntityTag / ShapeStore / Module-Action 框架

---

## 2. 模块架构

### 2.1 模块依赖图

```
                    ┌──────────────┐
                    │     app      │  (Qt/QML, GLViewport, GLViewportRenderer)
                    └──────┬───────┘
                           │ depends
              ┌────────────┴────────────┐
              │                         │
     ┌────────▼────────┐      ┌────────▼────────┐
     │ opengeolab_render│      │   Qt 6.9 libs   │
     │ (OpenGL + glad)  │      │                 │
     └────────┬─────────┘      └─────────────────┘
              │ depends
     ┌────────▼─────────┐
     │ opengeolab_scene  │
     └────────┬─────────┘
              │ depends
     ┌────────┴─────────────┐
     │                      │
┌────▼─────┐    ┌───────────▼──────────┐
│  core    │    │ opengeolab_geometry   │
└──────────┘    └──────────────────────┘
```

### 2.2 CMake 目标

| 目标 | Alias | 依赖 | 说明 |
|------|-------|------|------|
| `opengeolab_scene` | `OpenGeoLab::Scene` | `OpenGeoLab::Core`, `OpenGeoLab::Geometry`, `glm` | 场景图、节点、组件接口、拓扑索引、SceneBridge |
| `opengeolab_render` | `OpenGeoLab::Render` | `OpenGeoLab::Core`, `OpenGeoLab::Scene`, `glad`, `glm` | 渲染管线、Pass、GPU buffer、PickResolver |
| `opengeolab_app` | (existing) | +`OpenGeoLab::Render`, +`Qt6::OpenGL` | GLViewport、GLViewportRenderer、CameraController |

### 2.3 目录结构

```
src/libs/scene/
├── CMakeLists.txt
├── include/opengeolab/scene/
│   ├── scene_graph.hpp           // SceneGraph 根容器
│   ├── scene_node.hpp            // SceneNode 通用节点
│   ├── transform3d.hpp           // 局部变换
│   ├── display_mode.hpp          // DisplayMode 枚举
│   ├── render_component.hpp      // IRenderComponent 接口
│   ├── pick_component.hpp        // IPickComponent 接口
│   ├── pick_id.hpp               // PickId 编解码
│   ├── topology_index.hpp        // 拓扑关系索引
│   ├── render_mesh_data.hpp      // 渲染用 mesh 数据结构
│   └── geometry_scene_bridge.hpp // ShapeStore → SceneGraph 同步器
├── src/
│   ├── scene_graph.cpp
│   ├── scene_node.cpp
│   ├── transform3d.cpp
│   ├── pick_id.cpp
│   ├── topology_index.cpp
│   ├── geometry_scene_bridge.cpp
│   └── render_mesh_data.cpp
└── test/
    ├── scene_graph_test.cpp
    ├── pick_id_test.cpp
    └── topology_index_test.cpp

src/libs/render/
├── CMakeLists.txt
├── include/opengeolab/render/
│   ├── render_pipeline.hpp       // RenderPipeline 顶层入口
│   ├── frame_state.hpp           // 每帧状态
│   ├── pick_result.hpp           // 拾取结果
│   └── pick_mask.hpp             // 拾取模式掩码
├── src/
│   ├── render_pipeline.cpp
│   ├── core/
│   │   ├── gpu_buffer_manager.hpp/cpp  // VAO/VBO/IBO 管理
│   │   ├── shader_program.hpp/cpp      // Shader 编译/链接
│   │   └── pick_fbo.hpp/cpp            // 拾取离屏 FBO
│   ├── pass/
│   │   ├── render_pass_base.hpp        // Pass 基类
│   │   ├── opaque_pass.hpp/cpp         // 面着色
│   │   ├── wireframe_pass.hpp/cpp      // 线框/边/点
│   │   ├── highlight_pass.hpp/cpp      // 高亮叠加
│   │   ├── selection_pass.hpp/cpp      // GPU 拾取
│   │   └── draw_batch_utils.hpp/cpp    // glMultiDraw 批次工具
│   └── pick_resolver.hpp/cpp           // 拾取 ID 解析
└── test/
    └── pick_resolver_test.cpp

src/app/  (扩展现有)
├── include/opengeolab/app/
│   ├── gl_viewport.hpp               // QQuickFramebufferObject
│   ├── gl_viewport_renderer.hpp      // 渲染线程 Renderer
│   ├── camera_state.hpp              // 相机参数
│   └── trackball_controller.hpp      // 轨迹球控制器
├── src/
│   ├── gl_viewport.cpp
│   ├── gl_viewport_renderer.cpp
│   ├── camera_state.cpp
│   └── trackball_controller.cpp
└── resource/qml/sections/
    └── ViewportPanel.qml             // (更新现有)
```

---

## 3. Scene 模块详细设计

### 3.1 SceneNode — 通用场景节点

采用 **组合式设计**：节点本身只维护树结构 + 变换 + 可见性，领域数据通过组件接口挂载。

```cpp
namespace OpenGeoLab::Scene {

using NodeId = uint32_t;

class OPENGEOLAB_SCENE_EXPORT SceneNode final {
public:
    explicit SceneNode(NodeId id, std::string name = {});
    ~SceneNode();

    // ── 标识 ──
    NodeId id() const;
    std::string_view name() const;
    void setName(std::string name);

    // ── 变换 ──
    Transform3D& localTransform();
    const Transform3D& localTransform() const;
    glm::mat4 worldMatrix() const;  // 递归计算至根

    // ── 可见性 ──
    bool isVisible() const;
    void setVisible(bool visible);
    DisplayMode displayMode() const;
    void setDisplayMode(DisplayMode mode);

    // ── 层级 ──
    SceneNode* parent() const;
    std::span<const std::unique_ptr<SceneNode>> children() const;
    SceneNode* addChild(std::unique_ptr<SceneNode> child);
    std::unique_ptr<SceneNode> removeChild(NodeId child_id);
    SceneNode* findChild(NodeId child_id) const;

    // ── 包围盒 ──
    void setLocalBounds(const BoundingBox3D& bounds);
    BoundingBox3D localBounds() const;
    BoundingBox3D worldBounds() const;  // 变换后

    // ── 组件 (组合式) ──
    void setRenderComponent(std::unique_ptr<IRenderComponent> comp);
    void setPickComponent(std::unique_ptr<IPickComponent> comp);
    IRenderComponent* renderComponent() const;
    IPickComponent* pickComponent() const;

    // ── 选中状态 ──
    bool isSelected() const;
    void setSelected(bool selected);
    bool isHovered() const;
    void setHovered(bool hovered);

    // ── 脏标记 ──
    uint64_t version() const;
    void markDirty();

private:
    NodeId m_id;
    std::string m_name;
    Transform3D m_localTransform;
    bool m_visible{true};
    DisplayMode m_displayMode{DisplayMode::SolidWithEdges};
    BoundingBox3D m_localBounds;

    SceneNode* m_parent{nullptr};
    std::vector<std::unique_ptr<SceneNode>> m_children;

    std::unique_ptr<IRenderComponent> m_renderComponent;
    std::unique_ptr<IPickComponent> m_pickComponent;

    bool m_selected{false};
    bool m_hovered{false};
    uint64_t m_version{0};
};

} // namespace OpenGeoLab::Scene
```

### 3.2 SceneGraph — 场景图根容器

```cpp
namespace OpenGeoLab::Scene {

class OPENGEOLAB_SCENE_EXPORT SceneGraph final {
public:
    SceneGraph();
    ~SceneGraph();

    // ── 节点管理 ──
    NodeId allocateNodeId();
    SceneNode* root() const;
    SceneNode* addNode(std::string name, NodeId parent_id = 0);
    bool removeNode(NodeId id);
    SceneNode* findNode(NodeId id) const;

    // ── 遍历 ──
    void traverseVisible(std::function<void(const SceneNode&)> visitor) const;
    void traverseDirty(uint64_t since_version,
                       std::function<void(const SceneNode&)> visitor) const;

    // ── 选中管理 ──
    std::vector<NodeId> selectedNodes() const;
    void selectNode(NodeId id, bool append = false);
    void deselectNode(NodeId id);
    void clearSelection();
    std::optional<NodeId> hoveredNode() const;
    void setHoveredNode(std::optional<NodeId> id);

    // ── 包围盒 ──
    BoundingBox3D sceneBounds() const;

    // ── 线程安全 ──
    std::shared_lock<std::shared_mutex> readLock() const;
    std::unique_lock<std::shared_mutex> writeLock();

    // ── 版本 ──
    uint64_t version() const;

    // ── 信号 ──
    Kangaroo::Util::Signal<NodeId> nodeAdded;
    Kangaroo::Util::Signal<NodeId> nodeRemoved;
    Kangaroo::Util::Signal<NodeId> nodeUpdated;
    Kangaroo::Util::Signal<> selectionChanged;

private:
    std::unique_ptr<SceneNode> m_root;
    std::unordered_map<NodeId, SceneNode*> m_nodeIndex;
    uint32_t m_nextNodeId{1};
    uint64_t m_version{0};
    mutable std::shared_mutex m_mutex;
};

} // namespace OpenGeoLab::Scene
```

### 3.3 组件接口

```cpp
// ── 渲染组件 ──
class OPENGEOLAB_SCENE_EXPORT IRenderComponent {
public:
    virtual ~IRenderComponent() = default;

    /** 返回可渲染 mesh 数据 (三角面 + 线段 + 点) */
    virtual const RenderMeshData& meshData() const = 0;

    /** 脏检查版本号 */
    virtual uint64_t dataVersion() const = 0;
};

// ── 拾取组件 ──
enum class PickStrategy : uint8_t { Gpu, Ray, Both };

class OPENGEOLAB_SCENE_EXPORT IPickComponent {
public:
    virtual ~IPickComponent() = default;

    virtual PickStrategy strategy() const = 0;

    /** GPU 拾取: 每个图元的 PickId 条目 */
    virtual std::span<const PickIdEntry> pickEntries() const = 0;

    /** 射线拾取 (预留接口) */
    virtual std::optional<PickHit> rayPick(const Ray3D& ray) const;
};
```

### 3.4 RenderMeshData — 渲染数据结构

```cpp
namespace OpenGeoLab::Scene {

/** 轴对齐包围盒 */
struct BoundingBox3D {
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{std::numeric_limits<float>::lowest()};

    void expand(const glm::vec3& point);
    bool isValid() const;
    glm::vec3 center() const;
    glm::vec3 size() const;
};

/** 单个顶点的主渲染数据 (40 字节) */
struct RenderVertex {
    float position[3]{};   // 12 bytes
    float normal[3]{};     // 12 bytes
    float color[4]{};      // 16 bytes
};
static_assert(sizeof(RenderVertex) == 40);

/** 拾取 ID 条目 (8 字节) */
struct PickIdEntry {
    uint64_t pickId{0};    // 编码的 (shapeId, type, localId)
};
static_assert(sizeof(PickIdEntry) == 8);

/** 图元绘制范围 */
struct DrawRange {
    uint32_t shapeId;
    Core::EntityType entityType;
    uint32_t localId;

    uint32_t vertexOffset;
    uint32_t vertexCount;
    uint32_t indexOffset;
    uint32_t indexCount;

    PrimitiveTopology topology;  // Triangles, Lines, Points
};

/** 渲染用 mesh 数据 —— 打包了所有拓扑类型
 *
 * 不变量: pickIds.size() == vertices.size()，且同一三角形的三个顶点
 * 必须编码相同的 pickId（标识其所属 Face）。OCC tessellation 天然保证
 * per-face 独立顶点，不存在跨 face 的共享顶点。
 */
struct RenderMeshData {
    // Flat buffers
    std::vector<RenderVertex> vertices;
    std::vector<PickIdEntry> pickIds;     // 与 vertices 一一对应
    std::vector<uint32_t> indices;

    // Draw ranges per topology
    std::vector<DrawRange> triangleRanges;
    std::vector<DrawRange> lineRanges;
    std::vector<DrawRange> pointRanges;

    // 包围盒
    BoundingBox3D bounds;

    // 版本号 (脏检查)
    uint64_t version{0};
    void markUpdated() { ++version; }
};

} // namespace OpenGeoLab::Scene
```

### 3.5 PickId — 编解码

```cpp
namespace OpenGeoLab::Scene {

/** PickId 64-bit 编码: [shapeId(24) | localId(32) | type(8)]
 *  约束: shapeId <= 0x00FFFFFF (16M)，超出范围 assert 失败。
 */
struct PickId {
    static constexpr uint64_t encode(uint32_t shape_id,
                                     Core::EntityType type,
                                     uint32_t local_id) {
        assert(shape_id <= 0x00FFFFFFu && "shapeId exceeds 24-bit PickId capacity");
        return (static_cast<uint64_t>(shape_id & 0x00FFFFFFu) << 40u)
             | (static_cast<uint64_t>(local_id) << 8u)
             | static_cast<uint64_t>(type);
    }

    static constexpr Core::EntityType decodeType(uint64_t encoded) {
        return static_cast<Core::EntityType>(encoded & 0xFFu);
    }

    static constexpr uint32_t decodeLocalId(uint64_t encoded) {
        return static_cast<uint32_t>((encoded >> 8u) & 0xFFFFFFFFu);
    }

    static constexpr uint32_t decodeShapeId(uint64_t encoded) {
        return static_cast<uint32_t>((encoded >> 40u) & 0x00FFFFFFu);
    }

    static constexpr bool isValid(uint64_t encoded) {
        return encoded != 0;
    }
};

} // namespace OpenGeoLab::Scene
```

### 3.6 TopologyIndex — 拓扑关系索引

```cpp
namespace OpenGeoLab::Scene {

/**
 * 维护 CAD 拓扑层级关系，用于拾取模式向上查询。
 * 数据从 OCC ShapeEntry 的子形状索引中构建。
 */
class OPENGEOLAB_SCENE_EXPORT TopologyIndex final {
public:
    /** 从 ShapeEntry 构建索引 */
    void buildForShape(uint32_t shape_id, const Geometry::ShapeEntry& entry);

    /** 移除某个 shape 的所有索引 */
    void removeShape(uint32_t shape_id);

    // ── 向上查询 ──
    std::optional<uint32_t> edgeToWire(uint32_t shape_id, uint32_t edge_local_id) const;
    std::optional<uint32_t> wireToFace(uint32_t shape_id, uint32_t wire_local_id) const;
    std::optional<uint32_t> faceToSolid(uint32_t shape_id, uint32_t face_local_id) const;

    // ── 向下展开 ──
    std::vector<uint32_t> wireEdges(uint32_t shape_id, uint32_t wire_local_id) const;
    std::vector<uint32_t> solidFaces(uint32_t shape_id, uint32_t solid_local_id) const;

private:
    struct ShapeTopology {
        std::unordered_map<uint32_t, uint32_t> edgeToWire;
        std::unordered_map<uint32_t, uint32_t> wireToFace;
        std::unordered_map<uint32_t, uint32_t> faceToSolid;
        std::unordered_map<uint32_t, std::vector<uint32_t>> wireToEdges;
        std::unordered_map<uint32_t, std::vector<uint32_t>> solidToFaces;
    };
    std::unordered_map<uint32_t, ShapeTopology> m_shapes;
};

} // namespace OpenGeoLab::Scene
```

### 3.7 GeometrySceneBridge — ShapeStore 自动同步

```cpp
namespace OpenGeoLab::Scene {

/**
 * 监听 ShapeStore 信号，自动维护 SceneGraph 节点。
 * 每个 shape → 一个 SceneNode，附带 GeometryRenderComponent 和 GeometryPickComponent。
 */
class OPENGEOLAB_SCENE_EXPORT GeometrySceneBridge final {
public:
    GeometrySceneBridge(SceneGraph& scene,
                        Geometry::ShapeStore& store,
                        TopologyIndex& topoIndex);
    ~GeometrySceneBridge();

private:
    void onShapeAdded(uint32_t shape_id, const Geometry::ShapeEntry& entry);
    void onShapeRemoved(uint32_t shape_id);
    void onShapeUpdated(uint32_t shape_id, const Geometry::ShapeEntry& entry);

    /** 将 VisualData + EntityTag → RenderMeshData + PickIdEntry */
    static RenderMeshData buildRenderData(uint32_t shape_id,
                                          const Geometry::ShapeEntry& entry);

    SceneGraph& m_scene;
    Geometry::ShapeStore& m_store;
    TopologyIndex& m_topoIndex;
    std::unordered_map<uint32_t, NodeId> m_shapeToNode;
    std::vector<Kangaroo::Util::ScopedConnection> m_connections;
};

} // namespace OpenGeoLab::Scene
```

### 3.8 DisplayMode 枚举

> **注意**：仓库已有 `Core::RenderStyle { Solid, Wireframe, SolidWithEdges, Transparent }`。
> `Scene::DisplayMode` 是 Scene 层独立的概念，用于节点级可见性控制。
> 实现时 GeometrySceneBridge 负责将 `Core::RenderStyle` 映射到 `Scene::DisplayMode`。

```cpp
namespace OpenGeoLab::Scene {

enum class DisplayMode : uint8_t {
    Solid          = 0x01,  // 只有面
    Wireframe      = 0x02,  // 只有线
    SolidWithEdges = 0x03,  // 面 + 线 (默认)
    Points         = 0x04,  // 只有点
};

enum class DisplayModeMask : uint8_t {
    None      = 0,
    Surface   = 1 << 0,
    Wireframe = 1 << 1,
    Points    = 1 << 2,
};

} // namespace OpenGeoLab::Scene
```

---

## 4. Render 模块详细设计

### 4.1 RenderPipeline — 渲染管线入口

```cpp
namespace OpenGeoLab::Render {

/**
 * 渲染管线的顶层入口。
 * 不依赖 Qt —— 仅使用 glad + glm + scene 库。
 * 由 app 层（GLViewportRenderer）在渲染线程中调用。
 */
class OPENGEOLAB_RENDER_EXPORT RenderPipeline final {
public:
    RenderPipeline();
    ~RenderPipeline();

    /** glad 已加载后调用。创建 shader, FBO, GPU buffer。 */
    void initialize();

    /** 视口尺寸变化时调用 */
    void resize(int width, int height);

    /**
     * 从场景图同步脏数据到 GPU。
     * 调用方必须持有 SceneGraph 的读锁。
     */
    void synchronize(const Scene::SceneGraph& scene);

    /** 执行四个 Pass 完成一帧渲染 */
    void render(const FrameState& state);

    // ── 拾取 ──

    /** 单点拾取 */
    PickResult pickAt(int x, int y, PickMask mask) const;

    /** 区域拾取 (圆形/矩形) */
    std::vector<PickResult> pickRegion(int cx, int cy,
                                       int radius, PickMask mask) const;

    /** 释放 GPU 资源 */
    void cleanup();

private:
    GpuBufferManager m_bufferManager;
    OpaquePass m_opaquePass;
    WireframePass m_wireframePass;
    HighlightPass m_highlightPass;
    SelectionPass m_selectionPass;
    PickResolver m_pickResolver;

    int m_viewportWidth{0};
    int m_viewportHeight{0};
    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
```

### 4.2 FrameState — 每帧状态

```cpp
namespace OpenGeoLab::Render {

struct FrameState {
    // 相机
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projMatrix{1.0f};
    glm::vec3 cameraPos{0.0f};
    float devicePixelRatio{1.0f};

    // 视口
    int viewportWidth{0};
    int viewportHeight{0};

    // 显示选项
    bool xRayMode{false};
    Scene::DisplayModeMask displayMask{
        Scene::DisplayModeMask::Surface | Scene::DisplayModeMask::Wireframe};

    // 高亮（由调用方从 SceneGraph 中提取）
    std::vector<Scene::DrawRange> selectedDrawRanges;
    std::vector<Scene::DrawRange> hoveredDrawRanges;
};

} // namespace OpenGeoLab::Render
```

### 4.3 GpuBufferManager — VBO/IBO 管理

```cpp
namespace OpenGeoLab::Render {

/**
 * 管理所有场景节点几何数据的 GPU buffer。
 *
 * 分离存储:
 *   Main VBO (40B/vertex): position + normal + color
 *   Pick VBO (8B/vertex):  pickId (uint64 as uvec2)
 *   IBO (4B/index):        triangle + line indices
 *
 * 每帧检查脏标记，增量上传变更的节点数据。
 */
class GpuBufferManager final {
public:
    void initialize();
    void cleanup();

    /**
     * 同步场景图数据到 GPU。
     * 遍历所有可渲染节点，检查 version → 增量上传。
     */
    void synchronize(const Scene::SceneGraph& scene);

    /** 绑定 Main VBO + IBO 的 VAO (用于非拾取 Pass) */
    void bindMainVao() const;

    /** 绑定 Main VBO(position) + Pick VBO + IBO 的 VAO (用于 SelectionPass) */
    void bindPickVao() const;

    void unbind() const;

    // ── 批次缓存 ──
    const std::vector<Scene::DrawRange>& triangleRanges() const;
    const std::vector<Scene::DrawRange>& lineRanges() const;
    const std::vector<Scene::DrawRange>& pointRanges() const;

private:
    GLuint m_mainVao{0};       // Main 渲染用 VAO
    GLuint m_pickVao{0};       // Selection Pass 用 VAO
    GLuint m_mainVbo{0};       // 40B/vertex (pos + normal + color)
    GLuint m_pickVbo{0};       // 8B/vertex (pickId)
    GLuint m_ibo{0};           // 共享 index buffer

    // 上传版本跟踪
    uint64_t m_uploadedVersion{0};

    // 合并后的 draw ranges
    std::vector<Scene::DrawRange> m_triangleRanges;
    std::vector<Scene::DrawRange> m_lineRanges;
    std::vector<Scene::DrawRange> m_pointRanges;
};

} // namespace OpenGeoLab::Render
```

### 4.4 Render Pass 架构

#### 4.4.1 RenderPassBase

```cpp
namespace OpenGeoLab::Render {

class RenderPassBase {
public:
    virtual ~RenderPassBase() = default;

    void initialize() {
        if (!m_initialized) {
            m_initialized = onInitialize();
        }
    }

    void cleanup() {
        if (m_initialized) {
            onCleanup();
            m_initialized = false;
        }
    }

    virtual void render(const FrameState& state,
                        const GpuBufferManager& buffers) = 0;

protected:
    virtual bool onInitialize() = 0;
    virtual void onCleanup() {}

    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
```

#### 4.4.2 OpaquePass — 面着色

```
输入:  Main VAO (pos + normal + color)
输出:  默认 framebuffer
功能:  带光照的面着色渲染
       - Headlamp + ambient + sky/ground bounce
       - 支持 X-Ray 半透明模式 (alpha = 0.25)
       - glMultiDrawElements 批次绘制所有三角面

Shader (GLSL 330 core):
  Vertex:  a_position, a_normal, a_color → 变换 + 传递
  Fragment: Headlamp 光照模型 + premultiplied alpha
```

#### 4.4.3 WireframePass — 线框/边/点

```
输入:  Main VAO
输出:  默认 framebuffer (叠加)
功能:  线框和顶点渲染
       - GL_LINES: 所有 Edge 线段 (glMultiDrawElements)
       - GL_POINTS: 所有 Vertex 点 (glMultiDrawArrays)
       - 启用 depth test, 禁用 depth write (避免遮挡面)
       - Line width / point size 可配置
```

#### 4.4.4 HighlightPass — 高亮叠加

```
输入:  Main VAO + 选中/悬停的 DrawRange 列表
输出:  默认 framebuffer (叠加)
功能:  选中/悬停实体的高亮重绘
       - 选中: 固定蓝色 (0.2, 0.4, 0.9, 0.6)
       - 悬停: 固定浅蓝 (0.4, 0.7, 1.0, 0.4)
       - 对选中的 DrawRange 重新绘制三角和线段
       - 使用 additive blending 叠加颜色
       - 支持 depth offset 避免 z-fighting
```

#### 4.4.5 SelectionPass — GPU 拾取

```
输入:  Main VAO (position only) + Pick VAO (pickId)
输出:  离屏 PickFBO (RG32UI + Depth24)
功能:  渲染所有可拾取图元的 pickId 到离屏 FBO
       - 按 Face → Edge → Vertex 顺序绘制 (后画覆盖先画)
       - Vertex 绘制为 GL_POINTS (较大 point size = 7)
       - Edge 绘制为 GL_LINES (较大 line width = 3)

Shader (GLSL 330 core):
  Vertex:  a_position (from main VBO) + a_pickId (from pick VBO) → pass-through
  Fragment: fragPickId = v_pickId  (直接输出 uvec2)
```

### 4.5 PickFBO — 拾取 FBO

```cpp
namespace OpenGeoLab::Render {

class PickFbo final {
public:
    bool initialize(int width, int height);
    void resize(int width, int height);
    void cleanup();

    void bind() const;
    void unbind() const;

    /** 读取单个像素的 pickId */
    uint64_t readPickId(int x, int y) const;

    /** 读取以 (cx, cy) 为中心、半径 radius 的区域 */
    std::vector<uint64_t> readPickRegion(int cx, int cy, int radius) const;

private:
    GLuint m_fbo{0};
    GLuint m_colorTex{0};  // GL_RG32UI
    GLuint m_depthRbo{0};  // GL_DEPTH_COMPONENT24
    int m_width{0};
    int m_height{0};
};

} // namespace OpenGeoLab::Render
```

### 4.6 PickResolver — 拾取解析

```cpp
namespace OpenGeoLab::Render {

/** 拾取模式 */
enum class PickMode : uint8_t {
    VEF,    // 直接 Vertex > Edge > Face
    Wire,   // Edge → 向上查 Wire
    Solid,  // Face → 向上查 Solid
    Part,   // 任何 → 向上查 Part (shape_id)
};

/** 拾取掩码 (控制哪些拓扑类型参与拾取) */
enum class PickMask : uint32_t {
    None    = 0,
    Vertex  = 1 << 0,
    Edge    = 1 << 1,
    Wire    = 1 << 2,
    Face    = 1 << 3,
    Solid   = 1 << 4,
    Part    = 1 << 5,
    All     = 0xFFFFFFFF,
};

struct PickResult {
    uint32_t shapeId{0};
    Core::EntityType entityType{};
    uint32_t localId{0};
    bool valid{false};
};

/**
 * 将 PickFBO 读出的原始 pickId 列表解析为最终拾取结果。
 * 使用 TopologyIndex 进行层级解析。
 */
class OPENGEOLAB_RENDER_EXPORT PickResolver final {
public:
    explicit PickResolver(const Scene::TopologyIndex& topoIndex);

    /**
     * 解析一组原始 pickId，根据 PickMode 返回最终实体。
     *
     * VEF 模式: 优先级 Vertex > Edge > Face
     * Wire 模式: 命中 Edge → 向上查 Wire
     * Solid 模式: 命中 Face → 向上查 Solid
     * Part 模式: 任何命中 → 返回 shapeId
     */
    PickResult resolve(const std::vector<uint64_t>& raw_pick_ids,
                       PickMode mode) const;

    /** 批量解析 (用于框选) */
    std::vector<PickResult> resolveAll(const std::vector<uint64_t>& raw_pick_ids,
                                       PickMode mode) const;

    /**
     * 给定拾取结果，展开该实体所覆盖的所有子 DrawRange。
     * 用于 HighlightPass 知道要高亮哪些几何。
     */
    std::vector<Scene::DrawRange> expandToDrawRanges(
        const PickResult& pick,
        const std::vector<Scene::DrawRange>& allTriRanges,
        const std::vector<Scene::DrawRange>& allLineRanges,
        const std::vector<Scene::DrawRange>& allPointRanges) const;

private:
    const Scene::TopologyIndex& m_topoIndex;
};

} // namespace OpenGeoLab::Render
```

### 4.7 VBO 批次优化 — glMultiDraw

```cpp
namespace OpenGeoLab::Render {

/**
 * 将 DrawRange 列表转为 glMultiDrawElements 参数。
 * 根据可见性/显示模式过滤后，一次提交所有同拓扑 draw call。
 */
namespace BatchUtils {

struct IndexedBatch {
    std::vector<GLsizei> counts;
    std::vector<const void*> offsets;
    GLsizei drawCount() const { return static_cast<GLsizei>(counts.size()); }
};

struct ArrayBatch {
    std::vector<GLint> firsts;
    std::vector<GLsizei> counts;
    GLsizei drawCount() const { return static_cast<GLsizei>(counts.size()); }
};

/**
 * 从 DrawRange 列表构建 glMultiDrawElements 批次。
 * predicate 用于过滤 (可见性、选中状态等)。
 */
template <typename Predicate>
IndexedBatch buildIndexedBatch(const std::vector<Scene::DrawRange>& ranges,
                               Predicate&& predicate);

template <typename Predicate>
ArrayBatch buildArrayBatch(const std::vector<Scene::DrawRange>& ranges,
                           Predicate&& predicate);

/** 执行 glMultiDrawElements */
void multiDrawElements(GLenum mode, const IndexedBatch& batch);

/** 执行 glMultiDrawArrays */
void multiDrawArrays(GLenum mode, const ArrayBatch& batch);

} // namespace BatchUtils

} // namespace OpenGeoLab::Render
```

---

## 5. App 集成层详细设计

### 5.1 GLViewport — QML 视口控件

```cpp
namespace OpenGeoLab::App {

/**
 * QML 中的 3D 视口控件。
 * 在 GUI 线程处理输入事件，在渲染线程执行 OpenGL 渲染。
 */
class GLViewport : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool pickingEnabled READ isPickingEnabled
               WRITE setPickingEnabled NOTIFY pickingEnabledChanged)
    Q_PROPERTY(int pickMode READ pickMode
               WRITE setPickMode NOTIFY pickModeChanged)

public:
    explicit GLViewport(QQuickItem* parent = nullptr);
    ~GLViewport() override;

    Renderer* createRenderer() const override;

    // ── QML 可调用 ──
    Q_INVOKABLE void fitToScene();
    Q_INVOKABLE void setViewPreset(int preset);  // 0=Front, 1=Back, ...
    Q_INVOKABLE void toggleXRay();

    // ── 属性 ──
    bool isPickingEnabled() const;
    void setPickingEnabled(bool enabled);
    int pickMode() const;
    void setPickMode(int mode);

    // ── 供渲染线程在 synchronize 中消费 ──
    CameraState cameraState() const;
    std::optional<QPointF> consumePendingPickPosition();
    QPointF cursorPosition() const;
    qreal devicePixelRatio() const;

Q_SIGNALS:
    void pickingEnabledChanged();
    void pickModeChanged();

    /** 拾取结果信号 → QML 处理 */
    void entityPicked(int shapeId, int entityType, int localId);
    void entityHovered(int shapeId, int entityType, int localId);
    void pickCleared();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;

private:
    CameraState m_cameraState;
    TrackballController m_trackball;
    bool m_pickingEnabled{true};
    Render::PickMode m_pickMode{Render::PickMode::VEF};
    std::optional<QPointF> m_pendingPickPos;
    QPointF m_cursorPos;
};

} // namespace OpenGeoLab::App
```

### 5.2 GLViewportRenderer — 渲染线程

```cpp
namespace OpenGeoLab::App {

class GLViewportRenderer : public QQuickFramebufferObject::Renderer {
public:
    GLViewportRenderer(Scene::SceneGraph& scene,
                       Scene::TopologyIndex& topoIndex);
    ~GLViewportRenderer() override;

    QOpenGLFramebufferObject*
    createFramebufferObject(const QSize& size) override;

    void synchronize(QQuickFramebufferObject* item) override;
    void render() override;

private:
    /** glad 初始化 (首次 synchronize 时) */
    bool ensureGladInitialized();

    Scene::SceneGraph& m_scene;
    Scene::TopologyIndex& m_topoIndex;
    std::unique_ptr<Render::RenderPipeline> m_pipeline;

    // 从 GUI 线程同步来的状态
    Render::FrameState m_frameState;
    Render::PickMode m_pickMode{Render::PickMode::VEF};
    std::optional<QPointF> m_pendingPickPos;
    QPointF m_cursorPos;

    bool m_gladInitialized{false};
};

} // namespace OpenGeoLab::App
```

### 5.3 每帧渲染流程

```
GUI Thread                              Render Thread
─────────────                           ─────────────
mouse/wheel events                  
  → TrackballController 更新 CameraState
  → 记录 pendingPickPos / cursorPos
          ↓
    Qt Scene Graph 调度
          ↓
      synchronize(item)  ──────────→  synchronize():
                                        1. ensureGladInitialized()
                                        2. 从 GLViewport 拷贝:
                                           - CameraState → m_frameState
                                           - pendingPickPos
                                           - cursorPos
                                           - pickMode
                                        3. 读锁 SceneGraph:
                                           pipeline.synchronize(scene)
                                           (上传脏节点到 GPU)
            ↓
          render()  ──────────────→  render():
                                        1. pipeline.render(m_frameState)
                                           ├─ OpaquePass
                                           ├─ WireframePass
                                           ├─ HighlightPass
                                           └─ SelectionPass
                                        2. 如果有 pendingPickPos:
                                           m_pickResult = pipeline.pickAt(x, y, mask)
                                           (缓存到 Renderer 本地，不直接写 SceneGraph)
                                        3. hover:
                                           m_hoverResult = pipeline.pickAt(cursor, mask)
                                           (缓存到 Renderer 本地)
            ↓
    下一帧 synchronize()  ─────→  synchronize():
                                        1. 将上帧 m_pickResult/m_hoverResult 回写:
                                           (此时 GUI 线程已 blocked，安全写 SceneGraph)
                                           scene.setHoveredNode(m_hoverResult)
                                           scene.selectNode(m_pickResult)
                                        2. GLViewport 在 synchronize 后 emit:
                                           entityPicked / entityHovered 信号
                                           (在 GUI 线程上下文中发射，无跨线程风险)
                                        3. 继续正常同步流程...
```

### 5.4 CameraState 与 TrackballController

```cpp
namespace OpenGeoLab::App {

struct CameraState {
    glm::vec3 target{0.0f};       // 观察目标点
    float distance{100.0f};        // 相机到目标的距离
    float azimuth{0.0f};          // 水平旋转角 (rad)
    float elevation{0.3f};        // 垂直仰角 (rad)
    float fovY{45.0f};            // 垂直视场角 (deg)
    float nearPlane{0.1f};
    float farPlane{10000.0f};

    glm::mat4 viewMatrix() const;
    glm::mat4 projMatrix(float aspect) const;
    glm::vec3 eyePosition() const;
};

/**
 * 轨迹球控制器 —— 将鼠标事件转为相机状态变更。
 * 左键拖拽 = 旋转, 中键拖拽 = 平移, 滚轮 = 缩放。
 */
class TrackballController {
public:
    void onMousePress(QMouseEvent* event, CameraState& state);
    void onMouseMove(QMouseEvent* event, CameraState& state);
    void onMouseRelease(QMouseEvent* event, CameraState& state);
    void onWheel(QWheelEvent* event, CameraState& state);

    /** FitToScene: 根据包围盒调整 target + distance */
    void fitToScene(const Scene::BoundingBox3D& bounds, CameraState& state);

    /** 标准视角预设 */
    enum class ViewPreset { Front, Back, Top, Bottom, Left, Right, Isometric };
    void setViewPreset(ViewPreset preset, CameraState& state);

private:
    QPointF m_lastMousePos;
    bool m_rotating{false};
    bool m_panning{false};
};

} // namespace OpenGeoLab::App
```

### 5.5 Glad 初始化

```cpp
bool GLViewportRenderer::ensureGladInitialized() {
    if (m_gladInitialized) return true;

    // Qt 已在渲染线程创建了 GL context
    // glad 从当前 context 加载函数指针
    if (!gladLoadGL()) {
        Core::getLogger()->error("Failed to initialize glad");
        return false;
    }

    Core::getLogger()->info("OpenGL {}.{} loaded via glad",
                            GLVersion.major, GLVersion.minor);

    m_pipeline = std::make_unique<Render::RenderPipeline>();
    m_pipeline->initialize();
    m_gladInitialized = true;
    return true;
}
```

### 5.6 QML 注册与使用

```cpp
// main.cpp 中新增
#include <opengeolab/app/gl_viewport.hpp>

// GLViewport 通过 QML_ELEMENT 宏自动注册
// QML 中使用:
```

```qml
// ViewportPanel.qml
import OpenGeoLab.App

GLViewport {
    id: viewport
    anchors.fill: parent
    pickingEnabled: true
    pickMode: 0  // VEF

    onEntityPicked: (shapeId, entityType, localId) => {
        console.log("Picked:", shapeId, entityType, localId)
    }

    onEntityHovered: (shapeId, entityType, localId) => {
        statusBar.text = qsTr("Hovering: Shape %1, %2 #%3")
            .arg(shapeId).arg(entityType).arg(localId)
    }
}
```

---

## 6. 数据流全景

```
ShapeStore                      SceneGraph                    GPU
──────────                      ──────────                    ───
shapeAdded(id, entry)  ────→  GeometrySceneBridge:
                               1. buildRenderData()
                                  - VisualData → RenderVertex[]
                                  - EntityTag → PickIdEntry[]
                                  - 生成 DrawRange[]
                               2. 写锁 SceneGraph
                                  - addNode(name)
                                  - setRenderComponent(data)
                                  - setPickComponent(picks)
                               3. buildTopologyIndex()

                               SceneNode.version++
                                         ↓
                               pipeline.synchronize():
                                  读锁 SceneGraph
                                  - 检查 node.version
                                  - 增量上传到 Main VBO + Pick VBO
                                  - 更新 DrawRange 合并列表
                                         ↓
                               pipeline.render():              ────→  GL Draw Calls
                                  OpaquePass    (Main VAO, triangles)
                                  WireframePass (Main VAO, lines+points)
                                  HighlightPass (Main VAO, selected ranges)
                                  SelectionPass (Pick VAO, all pickable)
                                         ↓
                               pickAt() / hover:
                                  SelectionPass → PickFBO.readPixel()
                                  → PickResolver.resolve()
                                  → 缓存到 Renderer 本地
                                         ↓
                               下一帧 synchronize():
                                  回写 SceneGraph 选中/悬停 (GUI 线程 blocked)
                                  → GLViewport emit Qt Signal → QML
```

---

## 7. 第三方依赖

| 依赖 | 版本 | 用途 | 获取方式 |
|------|------|------|---------|
| glad | 2.x | OpenGL 4.6 函数加载 | CPM 或 vendored |
| glm | 1.0.1 | 3D 数学 (已有) | CPM (已配置) |
| doctest | 2.5.0 | 单元测试 (已有) | CPM (已配置) |

**不新增的依赖**: Qt（仅 app 层使用已有 Qt6::OpenGL）、OpenCASCADE（仅 scene 通过 geometry 间接依赖）

---

## 8. 测试策略

| 测试目标 | 方法 | 依赖 GL |
|---------|------|---------|
| PickId 编解码 | 纯单元测试，验证 encode/decode 对称性和边界值 | ❌ |
| TopologyIndex | 构建索引后验证 edgeToWire/wireToFace 等查询正确性 | ❌ |
| SceneGraph | 节点增删改查、选中/悬停状态、脏标记、线程安全 | ❌ |
| GeometrySceneBridge | 配合 ShapeStore 测试信号同步 | ❌ |
| RenderMeshData | 验证 VisualData → RenderVertex 转换、DrawRange 生成 | ❌ |
| PickResolver | 模拟 pickId 列表，验证各 PickMode 的解析逻辑 | ❌ |
| BatchUtils | 验证 DrawRange → IndexedBatch/ArrayBatch 转换 | ❌ |
| RenderPipeline | 集成测试 (需 GL context) | ✅ 离屏 |
| GLViewport | 手动 UI 测试 | ✅ 窗口 |

---

## 9. 实现阶段建议

| 阶段 | 内容 | 可验证产出 |
|------|------|-----------|
| **Phase 1** | scene lib: SceneNode, SceneGraph, PickId, DisplayMode, RenderMeshData | 单元测试全部通过 |
| **Phase 2** | scene lib: TopologyIndex, GeometrySceneBridge, IRenderComponent/IPickComponent | Bridge 功能测试 |
| **Phase 3** | render lib: ShaderProgram, GpuBufferManager, PickFbo, RenderPassBase | 编译通过 + 空帧渲染 |
| **Phase 4** | render lib: OpaquePass, WireframePass, RenderPipeline | 可见面/线渲染 |
| **Phase 5** | render lib: SelectionPass, PickResolver, HighlightPass, BatchUtils | GPU 拾取 + 高亮 |
| **Phase 6** | app: GLViewport, GLViewportRenderer, CameraState, TrackballController | QML 中可交互 3D 视口 |
| **Phase 7** | app: 拾取交互, QML 信号, ViewportPanel.qml 更新 | 端到端拾取 + 高亮 |
