# OpenGeoLab Render & Scene Architecture Design

> 状态: Draft
> 日期: 2026-03-27

## 1. 目标与范围

为 OpenGeoLab 增加 OpenGL 3D 渲染能力，使其成为可交互的 CAE 级可视化平台。

### 本次范围

- **libs/scene** — 场景图、变换、选择状态、实体注册、拾取解析
- **libs/render** — OpenGL 渲染引擎、相机、着色器、GPU 缓冲、视口、拾取渲染
- **core 扩展** — VisualData 共享数据类型（SurfaceMesh / EdgeMesh / EntityTag）
- **QML 集成** — ViewportItem（QQuickFramebufferObject）、鼠标交互、框选
- **ModuleBase 协议** — SceneModule 和 RenderModule 的 JSON 接口

### 不在本次范围

- libs/geometry（OCC 管理、离散化）— 独立任务，本文只定义其输出接口
- libs/mesh（FEM 网格、GMSH）— 独立任务，本文只定义其输出接口
- PBR 材质、阴影、SSAO 等高级渲染效果

---

## 2. 模块架构

### 2.1 依赖图（无环）

```
render   → scene, core, Qt6::OpenGL, Qt6::Quick, GLM, glad
scene    → core
mesh     → geometry, core       (未来)
geometry → core                 (未来)
command  → core, io, scene, render  (注册所有 ModuleBase)
app      → render, scene, Python_Embed, Qt6
```

### 2.2 全局模块关系

```
┌──────────────────────────────────────────────────────────────┐
│                          QML (App)                            │
│  ViewportPanel.qml · SidebarPanel.qml · Main.qml             │
│  ← 用户交互、状态展示                                          │
└───────────────┬──────────────────────────┬───────────────────┘
       mouse/resize                Q_PROPERTY/signals
┌───────────────▼──────────────────────────▼───────────────────┐
│               ViewportItem (QQuickFramebufferObject)          │
│               ← lives in libs/render, registered as QML type  │
└───────────────┬──────────────────────────┬───────────────────┘
          C++ API                     Scene API
┌─────────────────────┐       ┌──────────────────────────┐
│     libs/render      │       │       libs/scene          │
│  RenderEngine        │◄──────│  SceneGraph (thread-safe) │
│  Camera              │ reads │  SceneNode                │
│  ShaderProgram       │       │  EntityRegistry           │
│  GPUMesh             │       │  PickResolver             │
│  PickRenderer        │       │  SelectionManager         │
│  RenderModule        │       │  SceneModule              │
│  (ModuleBase)        │       │  (ModuleBase)             │
└─────────────────────┘       └──────────┬───────────────┘
                                         │ 接收 VisualData
                              ┌──────────┴───────────────┐
                              │ libs/geometry (未来)       │
                              │ libs/mesh    (未来)       │
                              │ → 产出 VisualData + Tags  │
                              └──────────┬───────────────┘
                              ┌──────────▼───────────────┐
                              │       libs/core           │
                              │ ModuleBase · IAction      │
                              │ VisualData 数据类型        │
                              │ EntityTag · Logger        │
                              └──────────────────────────┘
```

---

## 3. 核心共享数据类型（libs/core 扩展）

这些类型放在 core 中，因为 geometry、mesh、scene、render 都需要使用，
避免反向依赖。

### 3.1 VisualData — 可渲染数据

```cpp
// core/include/opengeolab/core/visual_data.hpp

#include <glm/vec4.hpp>
#include <cstdint>
#include <vector>

namespace OpenGeoLab::Core {

/**
 * @brief 三角面网格数据（用于 CAD 面显示或 FEM 表面显示）
 */
struct SurfaceMesh {
    std::vector<float> positions;      ///< [x,y,z, ...] 顶点坐标
    std::vector<float> normals;        ///< [nx,ny,nz, ...] 法线
    std::vector<uint32_t> indices;     ///< 三角形索引
    std::vector<float> colors;         ///< 可选逐顶点 [r,g,b,a, ...]
    glm::vec4 defaultColor{0.7f, 0.7f, 0.7f, 1.0f};
};

/**
 * @brief 线段数据（用于 CAD 边线或 FEM 网格线框）
 */
struct EdgeMesh {
    std::vector<float> positions;      ///< [x,y,z, ...]
    std::vector<uint32_t> indices;     ///< 线段索引（每两个一组）
    glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};
};

/**
 * @brief 渲染风格
 */
enum class RenderStyle : uint8_t {
    Solid,            ///< 仅实体面
    Wireframe,        ///< 仅线框
    SolidWithEdges,   ///< 实体面 + 边线叠加
    Transparent       ///< 半透明
};

/**
 * @brief 一个场景对象的完整可视化数据
 *
 * geometry 模块和 mesh 模块都产出此类型，scene 持有、render 消费。
 */
struct VisualData {
    std::vector<SurfaceMesh> surfaces; ///< 面数据（可多组：每个 OCC Face 一组）
    std::vector<EdgeMesh> edges;       ///< 边线数据
    RenderStyle style{RenderStyle::SolidWithEdges};
};

} // namespace OpenGeoLab::Core
```

### 3.2 EntityTag — 可拾取实体语义标注

```cpp
// core/include/opengeolab/core/entity_tag.hpp

namespace OpenGeoLab::Core {

/**
 * @brief 可拾取实体类型
 */
enum class EntityType : uint8_t {
    // 几何实体
    GeoVertex,     ///< OCC 顶点
    GeoEdge,       ///< OCC 边
    GeoFace,       ///< OCC 面
    GeoSolid,      ///< OCC 体

    // 网格实体
    MeshNode,      ///< FEM 节点
    MeshEdge,      ///< FEM 元素边
    MeshElement,   ///< FEM 元素

    // 场景级
    SceneNode      ///< 整个场景对象
};

/**
 * @brief 实体语义标签
 *
 * geometry/mesh 模块在产出 VisualData 时，同时产出对应的 EntityTag 数组，
 * 标注"第 N 个三角形属于 Face_3"或"第 M 个元素是 Element_5"。
 */
struct EntityTag {
    EntityType type;
    int localId;           ///< 模块内局部 ID（如 OCC Face Index 或 GMSH Element ID）
};

} // namespace OpenGeoLab::Core
```

### 3.3 VisualData 与 EntityTag 的关系

```
SurfaceMesh.indices: [0,1,2,  3,4,5,  6,7,8, ...]
                      ─────   ─────   ─────
                       tri0    tri1    tri2

EntityTag[]:          [{GeoFace, 3}, {GeoFace, 3}, {GeoFace, 5}, ...]
                       tri0属于Face3  tri1属于Face3  tri2属于Face5

→ Pick 时命中 tri1 → 查 EntityTag → 得知是 GeoFace #3
```

每个三角形对应一个 EntityTag。geometry/mesh 产出 VisualData 时同时产出
`std::vector<EntityTag> triangleTags` 和 `std::vector<EntityTag> edgeTags`。

---

## 4. libs/scene 设计

### 4.1 文件结构

```
libs/scene/
├── include/opengeolab/scene/
│   ├── scene_module.hpp           ← ModuleBase 实现
│   ├── scene_graph.hpp            ← 场景图容器（线程安全）
│   ├── scene_node.hpp             ← 场景节点
│   ├── transform.hpp              ← 位置/旋转/缩放
│   ├── selection_manager.hpp      ← 选择状态管理
│   ├── entity_registry.hpp        ← Pick ID ↔ 语义实体映射
│   └── pick_resolver.hpp          ← 从 raw pick ID 解析语义结果
├── src/
│   ├── scene_module.cpp
│   ├── scene_graph.cpp
│   ├── selection_manager.cpp
│   ├── entity_registry.cpp
│   ├── pick_resolver.cpp
│   └── actions/
│       ├── add_object_action.cpp
│       ├── remove_object_action.cpp
│       ├── list_objects_action.cpp
│       ├── set_transform_action.cpp
│       ├── get_transform_action.cpp
│       ├── set_visibility_action.cpp
│       ├── select_objects_action.cpp
│       └── get_selection_action.cpp
├── test/
│   └── scene_module_test.cpp
└── CMakeLists.txt
```

### 4.2 SceneNode

```cpp
// scene/include/opengeolab/scene/scene_node.hpp

struct SceneNode {
    std::string id;                              ///< 唯一标识 "Box_1"
    std::string label;                           ///< 显示名
    std::shared_ptr<Core::VisualData> visual;    ///< 可视化数据
    std::vector<Core::EntityTag> triangleTags;   ///< 每三角形语义标注
    std::vector<Core::EntityTag> edgeTags;       ///< 每线段语义标注
    Transform transform;                         ///< 位置/旋转/缩放
    bool visible{true};
    bool selected{false};
    std::string parentId;                        ///< 父节点 ID（""=根）
    std::vector<std::string> childIds;           ///< 子节点 ID 列表
};
```

### 4.3 SceneGraph（线程安全）

```cpp
class SceneGraph {
public:
    // 写操作（加锁）
    std::string addNode(SceneNode node);              ///< 返回分配的 ID
    void removeNode(std::string_view id);
    void setTransform(std::string_view id, Transform t);
    void setVisibility(std::string_view id, bool visible);

    // 只读查询（加锁）
    std::optional<SceneNode> findNode(std::string_view id) const;
    std::vector<SceneNode> allNodes() const;          ///< 快照

    // 变更追踪（synchronize 时使用）
    struct Changeset {
        std::vector<std::string> added;
        std::vector<std::string> removed;
        std::vector<std::string> transformChanged;
        std::vector<std::string> visualChanged;
        std::vector<std::string> visibilityChanged;
        bool cameraChanged{false};
    };
    Changeset consumeChangeset();

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<std::string, SceneNode> m_nodes;
    Changeset m_pendingChanges;
};
```

### 4.4 EntityRegistry — Pick ID 管理

```cpp
/**
 * @brief 管理 Pick ID → 语义实体的全局映射
 *
 * 当场景添加对象时，EntityRegistry 为该对象的每个可拾取三角形/线段
 * 分配唯一的 Pick ID（用于 Pick FBO 颜色编码）。
 */
class EntityRegistry {
public:
    struct PickEntry {
        std::string sceneNodeId;       ///< 所属场景节点
        Core::EntityTag tag;           ///< 语义标签（GeoFace #3, Element #5...）
        int primitiveIndex;            ///< 在 VisualData 中的索引
    };

    /// 为一个场景节点的所有可拾取实体分配 ID，返回起始 pickId
    uint32_t registerNode(const std::string& nodeId,
                          const std::vector<Core::EntityTag>& triangleTags,
                          const std::vector<Core::EntityTag>& edgeTags);

    /// 注销场景节点的所有 Pick ID
    void unregisterNode(const std::string& nodeId);

    /// 解析 Pick ID → 语义实体
    std::optional<PickEntry> resolve(uint32_t pickId) const;

private:
    uint32_t m_nextPickId{1};   ///< 0 保留为"未命中"
    std::unordered_map<uint32_t, PickEntry> m_entries;
    std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> m_nodeRanges;
};
```

### 4.5 SelectionManager

SelectionManager 是纯 C++ 类（**不是 QObject**），不依赖 Qt。
变更通知使用 `std::function` 回调，由 app 层桥接到 QML 信号。

```cpp
/**
 * @brief 管理当前选择状态（纯 C++，无 Qt 依赖）
 *
 * 线程安全：内部使用 std::mutex 保护状态。
 * 变更通知通过回调传递，app 层负责将回调桥接到 QML 信号。
 */
class SelectionManager {
public:
    /// 拾取模式
    enum class PickFilter : uint8_t {
        SceneNode = 0,   ///< 选择整个场景对象
        GeoVertex = 1,   ///< OCC 顶点
        GeoEdge = 2,     ///< OCC 边
        GeoFace = 3,     ///< OCC 面
        GeoSolid = 4,    ///< OCC 体
        MeshNode = 5,    ///< FEM 节点
        MeshEdge = 6,    ///< FEM 元素边
        MeshElement = 7  ///< FEM 元素
    };

    using SelectionChangedCallback =
        std::function<void(const std::vector<PickResult>&)>;

    void setPickFilter(PickFilter filter);
    PickFilter pickFilter() const;

    void selectSingle(const PickResult& result);
    void selectAppend(const std::vector<PickResult>& results);
    void clearSelection();

    /// 返回当前选择的快照副本（线程安全）
    std::vector<PickResult> selection() const;

    /// 注册变更回调（app 层桥接到 QML）
    void setOnSelectionChanged(SelectionChangedCallback cb);

private:
    mutable std::mutex m_mutex;
    std::vector<PickResult> m_selection;
    PickFilter m_pickFilter{PickFilter::SceneNode};
    SelectionChangedCallback m_onChanged;
};
```

### 4.6 PickResult

```cpp
struct PickResult {
    uint32_t pickId;                 ///< 原始 Pick ID
    std::string sceneNodeId;         ///< 所属场景节点 "Box_1"
    Core::EntityType entityType;     ///< GeoFace / MeshElement / ...
    int localEntityId;               ///< 模块内局部 ID
    glm::vec3 hitPoint;              ///< 3D 世界坐标交点
};
```

### 4.7 SceneModule — JSON 协议接口

```
module: "scene"

支持的 actions:
  add_object      — 添加对象（通常由 geometry/mesh 调用）
  remove_object   — 删除对象
  list_objects    — 列出所有对象
  set_transform   — 设置变换（LLM: "旋转 Box_1 到 45°"）
  get_transform   — 获取变换
  set_visibility  — 显示/隐藏
  select_objects  — 程序化选择（LLM: "选中所有面"）
  get_selection   — 获取当前选择
```

---

## 5. libs/render 设计

### 5.1 文件结构

```
libs/render/
├── include/opengeolab/render/
│   ├── render_module.hpp           ← ModuleBase 实现
│   ├── render_engine.hpp           ← 核心渲染器（仅 Render Thread）
│   ├── camera.hpp                  ← 相机（轨道/平移/缩放）
│   ├── shader_program.hpp          ← GLSL 着色器管理
│   ├── gpu_mesh.hpp                ← GPU 缓冲 RAII 封装
│   ├── render_scene.hpp            ← 渲染线程侧场景快照
│   ├── pick_renderer.hpp           ← 颜色拾取渲染
│   ├── viewport_item.hpp           ← QQuickFramebufferObject 子类
│   └── viewport_renderer.hpp       ← FBO 内部渲染器
├── src/
│   ├── render_engine.cpp
│   ├── camera.cpp
│   ├── shader_program.cpp
│   ├── gpu_mesh.cpp
│   ├── render_scene.cpp
│   ├── pick_renderer.cpp
│   ├── viewport_item.cpp
│   ├── viewport_renderer.cpp
│   └── actions/
│       ├── snapshot_action.cpp     ← 截图返回 base64
│       ├── set_camera_action.cpp   ← 设置相机位姿
│       └── get_camera_action.cpp   ← 获取相机状态
├── shaders/
│   ├── phong.vert                  ← Phong 着色顶点
│   ├── phong.frag                  ← Phong 着色片段
│   ├── flat.vert                   ← 平面着色（逐顶点色）
│   ├── flat.frag
│   ├── edge.vert                   ← 线段绘制
│   ├── edge.frag
│   ├── pick.vert                   ← 拾取（颜色编码）
│   ├── pick.frag
│   ├── grid.vert                   ← 无限地面网格
│   └── grid.frag
├── test/
│   └── render_module_test.cpp
└── CMakeLists.txt
```

### 5.2 Camera

```cpp
/**
 * @brief 轨道相机，支持 orbit / pan / zoom
 *
 * 相机使用"目标点 + 球面坐标"模型：
 * - target: 相机看向的 3D 点
 * - distance: 相机到 target 的距离
 * - azimuth: 水平旋转角（绕 Y 轴）
 * - elevation: 垂直仰角
 */
class Camera {
public:
    void orbit(float deltaAzimuth, float deltaElevation);
    void pan(float deltaX, float deltaY);
    void zoom(float delta);
    void fitAll(const glm::vec3& sceneCenter, float sceneRadius);

    void setPosition(const glm::vec3& eye, const glm::vec3& target,
                     const glm::vec3& up);

    [[nodiscard]] glm::mat4 viewMatrix() const;
    [[nodiscard]] glm::mat4 projectionMatrix(float aspectRatio) const;
    [[nodiscard]] glm::vec3 eyePosition() const;
    [[nodiscard]] glm::vec3 targetPosition() const;

    /// 将窗口坐标反投影为世界空间射线（用于 CPU pick）
    [[nodiscard]] Ray screenToWorldRay(const glm::vec2& screenPos,
                                       const glm::vec2& viewportSize) const;

    // 序列化（用于 ModuleBase JSON 接口）
    [[nodiscard]] nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);

private:
    glm::vec3 m_target{0.f};
    float m_distance{10.f};
    float m_azimuth{45.f};       ///< 度
    float m_elevation{30.f};     ///< 度
    float m_fov{45.f};           ///< 透视 FOV（度）
    float m_nearPlane{0.1f};
    float m_farPlane{10000.f};
};
```

### 5.3 RenderEngine

```cpp
/**
 * @brief 核心渲染器，仅在 Render Thread 上调用
 *
 * 职责：
 * 1. 管理 OpenGL 状态
 * 2. 上传/更新 GPU 缓冲
 * 3. 执行场景绘制
 * 4. 执行拾取绘制
 */
class RenderEngine {
public:
    void initialize();     ///< 首次调用时初始化 GL 状态、编译 shader
    void resize(int w, int h);

    /// 主渲染通道
    void render(const RenderScene& scene, const Camera& camera);

    /// 拾取渲染通道（离屏 FBO）
    void renderPickBuffer(const RenderScene& scene, const Camera& camera);

    /// 读取拾取结果
    uint32_t readPickId(int x, int y) const;

    /// 框选：读取矩形区域内所有不同 Pick ID
    std::vector<uint32_t> readPickIds(int x, int y, int w, int h) const;

    /// 截图
    QImage captureSnapshot(int width, int height);

private:
    ShaderProgram m_phongShader;
    ShaderProgram m_edgeShader;
    ShaderProgram m_pickShader;
    ShaderProgram m_gridShader;
    GLuint m_pickFBO{0};
    GLuint m_pickTexture{0};
    GLuint m_pickDepth{0};
    int m_width{0}, m_height{0};
    bool m_initialized{false};
};
```

### 5.4 GPUMesh — RAII 封装

```cpp
/**
 * @brief 管理 VAO/VBO/EBO 的 RAII 封装
 *
 * 从 SurfaceMesh 或 EdgeMesh 上传到 GPU。
 */
class GPUMesh {
public:
    static GPUMesh fromSurface(const Core::SurfaceMesh& mesh);
    static GPUMesh fromEdges(const Core::EdgeMesh& mesh);

    ~GPUMesh();
    GPUMesh(GPUMesh&& other) noexcept;
    GPUMesh& operator=(GPUMesh&& other) noexcept;

    void bind() const;
    void draw() const;              ///< glDrawElements
    void drawLines() const;         ///< glDrawElements(GL_LINES)
    [[nodiscard]] int indexCount() const;

private:
    GLuint m_vao{0}, m_vbo{0}, m_ebo{0};
    int m_indexCount{0};
    GLenum m_mode{GL_TRIANGLES};
};
```

### 5.5 RenderScene — 渲染线程侧场景快照

```cpp
/**
 * @brief 渲染线程持有的场景数据副本
 *
 * 在 synchronize() 时从 SceneGraph 更新。
 * 管理 GPU 缓冲的生命周期。
 */
struct RenderNode {
    std::string id;
    glm::mat4 modelMatrix;           ///< 世界变换矩阵
    std::vector<GPUMesh> surfaces;    ///< GPU 面网格
    std::vector<GPUMesh> edges;       ///< GPU 边线
    Core::RenderStyle style;
    bool visible{true};
    bool selected{false};

    /// 拾取相关
    uint32_t pickIdBase{0};           ///< 该节点的 Pick ID 起始值
    std::vector<uint32_t> surfacePickIds;  ///< 每三角形的 Pick ID
};

class RenderScene {
public:
    void applyChangeset(const SceneGraph::Changeset& changes,
                        const SceneGraph& graph,
                        const EntityRegistry& registry);

    const std::vector<RenderNode>& nodes() const;
    void clear();

private:
    std::vector<RenderNode> m_nodes;
};
```

### 5.6 PickRenderer

```cpp
/**
 * @brief 颜色拾取渲染器
 *
 * 将每个可拾取基元编码为唯一颜色（RGB24 → pickId）。
 * 支持单点拾取和矩形框选。
 */
class PickRenderer {
public:
    void initialize(int width, int height);
    void resize(int width, int height);

    /// 渲染拾取帧
    void render(const RenderScene& scene, const Camera& camera);

    /// 单点拾取
    uint32_t pickAt(int x, int y) const;

    /// 框选：读取矩形区域内所有唯一 Pick ID
    std::vector<uint32_t> pickInRect(int x, int y, int w, int h) const;

private:
    GLuint m_fbo{0};
    GLuint m_colorTex{0};
    GLuint m_depthRb{0};
    int m_width{0}, m_height{0};
    ShaderProgram m_pickShader;

    /// 编码/解码 Pick ID ↔ RGB
    static glm::vec3 encodePickId(uint32_t id);
    static uint32_t decodePickId(uint8_t r, uint8_t g, uint8_t b);
};
```

### 5.7 ViewportItem — QQuickFramebufferObject

```cpp
/**
 * @brief QML 可见的 3D 视口
 *
 * 在 QML 中使用:
 * @code
 * import OpenGeoLab.Render 1.0
 * ViewportItem {
 *     anchors.fill: parent
 *     sceneGraph: sceneGraphInstance
 * }
 * @endcode
 */
class ViewportItem : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    // 交互状态（QML 绑定）
    Q_PROPERTY(int interactionMode READ interactionMode
               WRITE setInteractionMode NOTIFY interactionModeChanged)
    Q_PROPERTY(int pickFilter READ pickFilter
               WRITE setPickFilter NOTIFY pickFilterChanged)
    Q_PROPERTY(bool isBoxSelecting READ isBoxSelecting NOTIFY boxSelectingChanged)
    Q_PROPERTY(QPointF boxSelectStart READ boxSelectStart NOTIFY boxSelectChanged)
    Q_PROPERTY(QPointF boxSelectEnd READ boxSelectEnd NOTIFY boxSelectChanged)

public:
    explicit ViewportItem(QQuickItem* parent = nullptr);

    Renderer* createRenderer() const override;

    /// 设置场景图引用
    void setSceneGraph(Scene::SceneGraph* graph);
    void setEntityRegistry(Scene::EntityRegistry* registry);
    void setSelectionManager(Scene::SelectionManager* selMgr);

    // 交互模式
    enum InteractionMode { Navigate, Select };
    Q_ENUM(InteractionMode)

    // Camera 访问（main thread）
    Camera& camera();

signals:
    void interactionModeChanged();
    void pickFilterChanged();
    void boxSelectingChanged();
    void boxSelectChanged();
    void pickResultReady(const QString& resultJson);   ///< 通知 QML 拾取结果

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    Camera m_camera;
    Scene::SceneGraph* m_sceneGraph{nullptr};
    Scene::EntityRegistry* m_entityRegistry{nullptr};
    Scene::SelectionManager* m_selectionManager{nullptr};

    InteractionMode m_interactionMode{Navigate};
    bool m_isBoxSelecting{false};
    QPointF m_boxStart, m_boxEnd;
    QPointF m_lastMousePos;
};
```

### 5.8 ViewportRenderer — 内部渲染器

```cpp
/**
 * @brief QQuickFramebufferObject::Renderer 实现
 *
 * 运行在 Render Thread 上。
 * synchronize() 拷贝主线程数据 → render() 执行 OpenGL 绘制。
 */
class ViewportRenderer : public QQuickFramebufferObject::Renderer {
public:
    ViewportRenderer();

    QOpenGLFramebufferObject* createFramebufferObject(
        const QSize& size) override;

    void synchronize(QQuickFramebufferObject* item) override;

    void render() override;

private:
    RenderEngine m_engine;
    PickRenderer m_pickRenderer;
    RenderScene m_renderScene;
    Camera m_cameraCopy;              ///< synchronize 拷贝的相机状态
    int m_width{0}, m_height{0};

    // 拾取请求队列
    struct PickRequest {
        enum Type { Single, Rect } type;
        int x, y, w, h;
    };
    std::vector<PickRequest> m_pendingPicks;
    std::vector<PickResult> m_pickResults;
};
```

### 5.9 RenderModule — JSON 协议接口

```
module: "render"

支持的 actions:
  snapshot      — 截图返回 base64 PNG（LLM: "给我看当前画面"）
  set_camera    — 设置相机位姿（LLM: "从正上方俯视"）
  get_camera    — 获取相机状态
  set_render_mode — 设置渲染模式（线框/实体/实体+边线）
  fit_all       — 缩放到显示全部对象
```

---

## 6. QML UI 框架设计

### 6.1 ViewportPanel.qml 重设计

```qml
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import OpenGeoLab.Render 1.0
import "../theme"
import "../components"

/**
 * @brief 3D 视口面板
 *
 * 包含：
 * 1. ViewportItem（OpenGL FBO 渲染）
 * 2. 框选矩形叠加层
 * 3. 交互模式工具栏
 * 4. 视角快捷按钮
 */
Item {
    id: root

    required property AppTheme theme

    // ── OpenGL 视口 ────────────────────────────────────
    ViewportItem {
        id: viewport
        anchors.fill: parent
    }

    // ── 框选矩形叠加层 ─────────────────────────────────
    Rectangle {
        visible: viewport.isBoxSelecting
        x: Math.min(viewport.boxSelectStart.x, viewport.boxSelectEnd.x)
        y: Math.min(viewport.boxSelectStart.y, viewport.boxSelectEnd.y)
        width: Math.abs(viewport.boxSelectEnd.x - viewport.boxSelectStart.x)
        height: Math.abs(viewport.boxSelectEnd.y - viewport.boxSelectStart.y)
        color: Qt.rgba(0.27, 0.53, 1.0, 0.15)
        border.color: "#4488ff"
        border.width: 1
        radius: 2
    }

    // ── 交互模式工具栏 ─────────────────────────────────
    Row {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 8
        spacing: 4
        z: 10

        ViewportToolButton {
            icon: "navigate"
            checked: viewport.interactionMode === ViewportItem.Navigate
            onClicked: viewport.interactionMode = ViewportItem.Navigate
            ToolTip.text: qsTr("Navigate (orbit / pan / zoom)")
        }
        ViewportToolButton {
            icon: "select"
            checked: viewport.interactionMode === ViewportItem.Select
            onClicked: viewport.interactionMode = ViewportItem.Select
            ToolTip.text: qsTr("Select (click / box select)")
        }

        // 拾取模式选择（仅 Select 模式可用）
        // PickFilter 枚举值: SceneNode=0, GeoVertex=1, GeoEdge=2, GeoFace=3
        Row {
            visible: viewport.interactionMode === ViewportItem.Select
            spacing: 2

            Repeater {
                model: [
                    { label: qsTr("Node"), filter: 1 },
                    { label: qsTr("Edge"), filter: 2 },
                    { label: qsTr("Face"), filter: 3 },
                    { label: qsTr("Solid"), filter: 4 }
                ]
                ViewportToolButton {
                    required property var modelData
                    text: modelData.label
                    checked: viewport.pickFilter === modelData.filter
                    onClicked: viewport.pickFilter = modelData.filter
                }
            }
        }
    }

    // ── 视角快捷键 ─────────────────────────────────────
    Column {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        spacing: 4
        z: 10

        ViewportToolButton {
            text: qsTr("Fit")
            onClicked: viewport.fitAll()
        }
        ViewportToolButton {
            text: qsTr("Top")
            onClicked: viewport.setStandardView("top")
        }
        ViewportToolButton {
            text: qsTr("Front")
            onClicked: viewport.setStandardView("front")
        }
        ViewportToolButton {
            text: qsTr("Right")
            onClicked: viewport.setStandardView("right")
        }
    }
}
```

### 6.2 鼠标交互状态机

```
┌──────────────────────────────────────────────────────┐
│                   Navigate 模式                       │
├──────────────────────────────────────────────────────┤
│ 左键拖拽        → Camera.orbit(dx, dy)               │
│ 中键拖拽        → Camera.pan(dx, dy)                  │
│ 右键拖拽        → Camera.zoom(dy)                     │
│ 滚轮            → Camera.zoom(delta)                  │
│ Shift+左键拖拽  → Camera.pan(dx, dy)                  │
└──────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────┐
│                   Select 模式                         │
├──────────────────────────────────────────────────────┤
│ 左键单击        → 单点拾取（根据 pickFilter）           │
│ Ctrl+左键单击   → 追加/切换选择                        │
│ 左键拖拽        → 框选（显示矩形叠加层）               │
│                   释放时提交框选区域                    │
│ 中键拖拽        → Camera.pan（Select 模式也保留导航）   │
│ 滚轮            → Camera.zoom（Select 模式也保留缩放） │
│ Esc             → 清空选择                             │
└──────────────────────────────────────────────────────┘
```

**单击 vs 拖拽判定：**
- mousePress 记录起始位置
- mouseMove 如果距离 > 5px → 进入拖拽（Navigate 模式 = orbit, Select 模式 = 框选）
- mouseRelease 如果未进入拖拽 → 视为单击 → 触发拾取

### 6.3 框选流程

```
1. Select 模式下 mousePress
   → 记录 boxStart = (x, y)
   → isBoxSelecting = false（尚未确认是框选）

2. mouseMove, 距离 > 5px
   → isBoxSelecting = true
   → 更新 boxEnd = (x, y)
   → QML 显示矩形叠加层

3. mouseRelease
   → 提交框选请求:
     ViewportRenderer 在下一帧的 render() 中:
       a. renderPickBuffer()       ← 渲染拾取 FBO
       b. readPickIds(x,y,w,h)     ← 读取矩形区域所有 Pick ID
       c. 去重
       d. EntityRegistry.resolve() → 语义结果
       e. 按 pickFilter 过滤       ← 只保留当前模式匹配的实体类型
       f. SelectionManager.selectAppend(results)
   → isBoxSelecting = false
```

### 6.4 拾取结果通知 QML

```
Pick/BoxSelect 结果通过信号传递:

C++ ViewportItem::pickResultReady(QString resultJson)
                     ↓
QML Connections {
    target: viewport
    function onPickResultReady(resultJson) {
        // 更新侧边栏选择信息
        // 更新状态栏文本
        let result = JSON.parse(resultJson)
        // result = { selected: [...], count: N }
    }
}
```

---

## 7. 线程模型

### 7.1 三线程交互

```
Main Thread (QML Event Loop)
  │
  ├─ 鼠标事件 → ViewportItem.mouseMoveEvent()
  │              → Camera.orbit() / 更新 boxEnd
  │              → item->update()  (请求重绘)
  │
  ├─ QML Property 变更 → interactionMode, pickFilter
  │
  └─ RequestService.submitAsync() → Worker Thread
                                      ↓
Worker Thread (QtConcurrent)          │
  │                                   │
  ├─ SceneModule.process()            │
  │  → SceneGraph.addNode()  [加锁写] │
  │  → SceneGraph.markDirty()         │
  │                                   │
  ├─ RenderModule.process()           │
  │  → Camera.setPosition() [主线程?] │ ← 需通过 QMetaObject::invokeMethod
  │  → ViewportItem.update()          │   保证线程安全
  │                                   │
Render Thread (Qt Quick Scene Graph)  │
  │                                   │
  ├─ synchronize()  ← 主线程被阻塞！  │
  │  1. 拷贝 Camera 状态              │
  │  2. SceneGraph.consumeChangeset() │
  │  3. 上传新 Mesh 到 GPU            │
  │  4. 接收 Pick 请求                │
  │                                   │
  ├─ render()                         │
  │  1. RenderEngine.render()         │
  │  2. 如有 Pick 请求:               │
  │     PickRenderer.render()         │
  │     PickRenderer.pickAt/pickRect()│
  │     EntityRegistry.resolve()      │
  │     SelectionManager.select()     │
  │  3. 完成 → QMetaObject::invokeMethod → 主线程通知 QML
  │
  └─ createFramebufferObject()
     → 窗口大小变化时重建 FBO
```

### 7.2 线程安全规则

| 组件 | 写入线程 | 读取线程 | 保护方式 |
|------|---------|---------|---------|
| SceneGraph | Main + Worker | Render (via sync) | `std::shared_mutex` |
| Camera | Main | Render (via sync) | synchronize() 拷贝 |
| EntityRegistry | Main + Worker | Render (via sync) | `std::shared_mutex` |
| SelectionManager | Render + Main | Main (QML callback) | `std::mutex` + callback 桥接 |
| RenderEngine | Render only | — | 单线程，无需锁 |
| GPUMesh | Render only | — | 单线程，无需锁 |
| PickRenderer | Render only | — | 单线程，无需锁 |

### 7.3 RenderModule 的 Worker Thread 安全

RenderModule 的 JSON actions 在 Worker Thread 上执行，
但某些操作需要在 Main Thread 或 Render Thread 上完成：

```
render.set_camera:
  Worker Thread → QMetaObject::invokeMethod(viewportItem, Qt::QueuedConnection)
                  → Main Thread → Camera.setPosition()
                  → item->update() → 触发 synchronize + render

render.snapshot:
  Worker Thread → 设置 "snapshot requested" 标志
               → 等待 Render Thread 完成截图
               → Render Thread 在 render() 后保存像素
               → 返回 base64
```

---

## 8. 数据流示例

### 8.1 导入文件 → 渲染

```
1. LLM/用户: {"module": "io", "action": "read_brep", "param": {"path": "box.brep"}}
2. IO 模块读取 BRep → 返回几何数据
3. Geometry 模块（未来）离散化 → 产出 VisualData + EntityTag[]
4. {"module": "scene", "action": "add_object", "param": {"label": "Box_1", ...}}
5. SceneGraph.addNode() → EntityRegistry 分配 Pick ID → markDirty
6. 下一帧 synchronize() → RenderScene 接收新节点
7. render() → 上传 GPU → 绘制
```

### 8.2 鼠标旋转

```
1. mouseMoveEvent(dx=5, dy=3)
2. Camera.orbit(5 * sensitivity, 3 * sensitivity)
3. item->update()
4. synchronize() → 拷贝 Camera
5. render() → 使用新 viewMatrix → 重绘
延迟: < 16ms (一帧)
```

### 8.3 单点拾取

```
1. mouseReleaseEvent at (200, 150), 未拖拽
2. ViewportItem 提交 PickRequest{Single, 200, 150}
3. item->update()
4. synchronize() → 接收 PickRequest
5. render():
   a. PickRenderer.render() → 渲染 Pick FBO
   b. PickRenderer.pickAt(200, 150) → pickId = 42
   c. EntityRegistry.resolve(42) → {nodeId="Box_1", type=GeoFace, localId=3}
   d. 按 pickFilter 过滤 → 匹配
   e. SelectionManager.selectSingle(result)
6. QMetaObject::invokeMethod → 主线程 → pickResultReady signal → QML 更新
```

### 8.4 框选

```
1. mousePress at (100, 100) → 记录 boxStart
2. mouseMove to (300, 250) → isBoxSelecting = true
3. QML 显示 200×150 蓝色半透矩形
4. mouseRelease → 提交 PickRequest{Rect, 100, 100, 200, 150}
5. synchronize() → 接收
6. render():
   a. PickRenderer.render()
   b. PickRenderer.pickInRect(100, 100, 200, 150)
      → 读取 200×150 像素 → 去重 → {42, 43, 50, 51, ...}
   c. 逐一 EntityRegistry.resolve()
   d. 按 pickFilter 过滤
   e. SelectionManager.selectAppend(filteredResults)
7. 通知 QML
```

### 8.5 LLM 截图

```
1. {"module": "render", "action": "snapshot", "param": {"width": 800, "height": 600}}
2. Worker Thread → RenderModule.process()
3. 设置 m_snapshotRequested = true, 尺寸 = 800x600
4. 等待 (条件变量)
5. Render Thread synchronize() → 发现 snapshot 请求
6. render() → 正常绘制 → 调用 glReadPixels → 保存为 QImage
7. 编码为 base64 PNG → 通知条件变量
8. Worker Thread 返回: {"status": "ok", "action": "snapshot", "image": "data:image/png;base64,..."}
```

---

## 9. CMake 集成

### 9.1 libs/scene/CMakeLists.txt

```cmake
opengeolab_add_module(
    opengeolab_scene
    ALIAS_NAME Scene
    PUBLIC_HEADERS
        include/opengeolab/scene/scene_module.hpp
        include/opengeolab/scene/scene_graph.hpp
        include/opengeolab/scene/scene_node.hpp
        include/opengeolab/scene/transform.hpp
        include/opengeolab/scene/selection_manager.hpp
        include/opengeolab/scene/entity_registry.hpp
        include/opengeolab/scene/pick_resolver.hpp
    SOURCES
        src/scene_module.cpp
        src/scene_graph.cpp
        src/selection_manager.cpp
        src/entity_registry.cpp
        src/pick_resolver.cpp
        src/actions/add_object_action.cpp
        src/actions/remove_object_action.cpp
        src/actions/list_objects_action.cpp
        src/actions/set_transform_action.cpp
        src/actions/get_transform_action.cpp
        src/actions/set_visibility_action.cpp
        src/actions/select_objects_action.cpp
        src/actions/get_selection_action.cpp
    PUBLIC_LINKS OpenGeoLab::Core glm::glm
)
```

### 9.2 libs/render/CMakeLists.txt

```cmake
opengeolab_add_module(
    opengeolab_render
    ALIAS_NAME Render
    PUBLIC_HEADERS
        include/opengeolab/render/render_module.hpp
        include/opengeolab/render/render_engine.hpp
        include/opengeolab/render/camera.hpp
        include/opengeolab/render/shader_program.hpp
        include/opengeolab/render/gpu_mesh.hpp
        include/opengeolab/render/render_scene.hpp
        include/opengeolab/render/pick_renderer.hpp
        include/opengeolab/render/viewport_item.hpp
        include/opengeolab/render/viewport_renderer.hpp
    SOURCES
        src/render_engine.cpp
        src/camera.cpp
        src/shader_program.cpp
        src/gpu_mesh.cpp
        src/render_scene.cpp
        src/pick_renderer.cpp
        src/viewport_item.cpp
        src/viewport_renderer.cpp
        src/actions/snapshot_action.cpp
        src/actions/set_camera_action.cpp
        src/actions/get_camera_action.cpp
    PUBLIC_LINKS
        OpenGeoLab::Core
        OpenGeoLab::Scene
        Qt6::Quick
        Qt6::OpenGL
        glm::glm
    PRIVATE_LINKS
        glad            # OpenGL loader, see §9.6
)
```

### 9.3 根 CMakeLists.txt 新增

```cmake
# Sub-projects (新增 scene 和 render)
add_subdirectory(src/libs/core)
add_subdirectory(src/libs/io)
add_subdirectory(src/libs/scene)      # ← 新增
add_subdirectory(src/libs/render)     # ← 新增
add_subdirectory(src/libs/command)
add_subdirectory(src/libs/python)
add_subdirectory(src/app)
```

### 9.4 app/CMakeLists.txt 更新

```cmake
target_link_libraries(
    opengeolab_app
    PRIVATE ...existing...
            OpenGeoLab::Render   # ← 新增
            OpenGeoLab::Scene    # ← 新增
            Qt6::OpenGL          # ← 新增
)
```

### 9.5 command 模块更新

```cmake
# command/CMakeLists.txt
opengeolab_add_module(
    opengeolab_command
    ALIAS_NAME Command
    SOURCES src/command_dispatcher.cpp src/module_registry.cpp
    PUBLIC_LINKS OpenGeoLab::Core OpenGeoLab::IO
                 OpenGeoLab::Scene OpenGeoLab::Render  # ← 新增
)
```

### 9.6 glad (OpenGL Loader) 引入方式

glad 通过 CPM 引入，在根 CMakeLists.txt 中添加：

```cmake
# 在 Third-party 区域新增
opengeolab_resolve_package(
    glad
    glad::glad
    VERSION 2.0.8
    GITHUB_REPOSITORY Dav1dde/glad
    GIT_TAG v2.0.8
    CPM_OPTIONS
    "GLAD_API gl:core=4.6"
    "GLAD_INSTALL ON"
)
```

glad 仅被 libs/render PRIVATE 链接，不暴露给其他模块。
这确保 OpenGL 符号不会泄漏到不需要 GL 的模块中。

### 9.7 模块注册方式

新模块沿用 `registerBuiltinModules()` 集中注册模式（与 IOModule 一致）：

```cpp
// module_registry.cpp
void registerBuiltinModules(Kangaroo::Util::PluginComponentFactory& factory) {
    factory.bindSingleton<Core::ModuleBase, IO::IOModule>(IO::IOModule::MODULE_NAME);
    factory.bindSingleton<Core::ModuleBase, Scene::SceneModule>(Scene::SceneModule::MODULE_NAME);
    factory.bindSingleton<Core::ModuleBase, Render::RenderModule>(Render::RenderModule::MODULE_NAME);
}
```

---

## 10. 测试策略

### 10.1 libs/scene 测试

```
scene_module_test.cpp:
  - SceneGraph: addNode / removeNode / setTransform / snapshot
  - EntityRegistry: registerNode / unregisterNode / resolve
  - SelectionManager: selectSingle / selectAppend / clearSelection
  - SceneModule JSON protocol: add_object / list_objects / set_transform
  - 线程安全: 并发 addNode + consumeChangeset
```

### 10.2 libs/render 测试

```
render_module_test.cpp:
  - Camera: orbit / pan / zoom / viewMatrix / projectionMatrix
  - Camera: toJson / fromJson 往返
  - RenderModule JSON protocol: set_camera / get_camera
  (注: OpenGL 渲染测试需要 GL context, 可用 offscreen context 或 mock)
```

### 10.3 集成测试

```
integration_test:
  - 完整流程: add_object → render → snapshot → 验证图片非空
  - 拾取: add_object → pickAt(center) → 验证返回正确节点
  (注: 需要 offscreen GL context)
```

---

## 11. 实现优先级建议

```
Phase 1: 最小可视化
  ├─ core: VisualData / EntityTag 数据类型
  ├─ scene: SceneGraph + SceneNode + Transform (无拾取)
  ├─ render: Camera + RenderEngine + ViewportItem + Phong shader + glad
  ├─ QML: ViewportPanel 替换为 ViewportItem
  └─ 验证: C++ 中直接构造 VisualData（硬编码三角形），
           通过 SceneGraph.addNode() 添加到场景，
           ViewportItem 中能旋转/缩放查看

Phase 2: 场景管理
  ├─ scene: SceneModule (ModuleBase) + all actions
  ├─ render: RenderScene + synchronize 流程
  ├─ command: 注册 SceneModule
  └─ 验证: JSON 添加多个对象，视口实时显示

Phase 3: 拾取与选择
  ├─ scene: EntityRegistry + SelectionManager + PickResolver
  ├─ render: PickRenderer + Pick FBO
  ├─ QML: Navigate/Select 模式切换 + 框选
  └─ 验证: 点选/框选对象，侧边栏显示选择

Phase 4: LLM 集成
  ├─ render: RenderModule (ModuleBase) + snapshot / set_camera
  ├─ command: 注册 RenderModule
  └─ 验证: Python 脚本截图 + 设置相机
```
