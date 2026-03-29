# Scene & Render Phase 1: Minimal Visualization

> 状态: Draft
> 日期: 2026-03-30
> 基础: docs/superpowers/specs/2026-03-27-render-scene-architecture-design.md (预研)

## 1. 目标与范围

### 本次范围 (Phase 1)

在 ViewportPanel 中嵌入 OpenGL 3D 视口，能够：

1. 显示 geometry 模块离散化和 mesh 模块剖分后的 VisualData
2. Phong 着色 + 线框叠加 (SolidWithEdges)
3. 轨道相机：鼠标左键旋转、中键平移、滚轮缩放
4. Fit All：自动适配场景范围
5. 数据变更自动刷新：geometry / mesh 模块 dataChanged → 视口重绘

### 不在本次范围

- 拾取 (Pick FBO)、选择 (SelectionManager)、框选
- EntityRegistry / PickResolver
- SceneModule JSON 协议 (add_object / remove_object 等 actions)
- RenderModule JSON 协议 (snapshot / set_camera 等 actions)
- 半透明渲染、PBR 材质、阴影、SSAO
- 地面网格 (grid shader)
- PointSet 渲染 (Phase 2)

---

## 2. 模块架构

### 2.1 依赖图

```
render  → scene, core, Qt6::OpenGL, Qt6::Quick, glm::glm, glad
scene   → core, glm::glm
```

### 2.2 Phase 1 组件总览

```
┌──────────────────────────────────────────────────────────────┐
│                         QML (App)                             │
│  ViewportPanel.qml  (嵌入 ViewportItem)                       │
│  Main.qml           (布局不变)                                │
└───────────────┬───────────────────────────────────────────────┘
          mouse/resize
┌───────────────▼───────────────────────────────────────────────┐
│              ViewportItem (QQuickFramebufferObject)             │
│              ← lives in libs/render, registered as QML type    │
└───────────────┬───────────────────────┬───────────────────────┘
          C++ API                  Scene API
┌─────────────────────┐       ┌──────────────────────────┐
│     libs/render      │       │       libs/scene          │
│  RenderEngine        │◄──────│  SceneGraph (线程安全)      │
│  Camera              │ reads │  SceneNode                │
│  ShaderProgram       │       │  Transform                │
│  GpuMesh             │       │  BoundingBox              │
│  RenderScene         │       │                           │
└─────────────────────┘       └──────────┬────────────────┘
                                         │ 接收 VisualData
                              ┌──────────┴────────────────┐
                              │ libs/geometry              │
                              │ libs/mesh                  │
                              │ → 产出 VisualData          │
                              └──────────┬────────────────┘
                              ┌──────────▼────────────────┐
                              │       libs/core            │
                              │ ModuleBase · IAction       │
                              │ VisualData 数据类型         │
                              │ EntityTag                  │
                              └───────────────────────────┘
```

---

## 3. libs/scene 设计 (Phase 1 轻量版)

### 3.1 文件结构

```
libs/scene/
├── include/opengeolab/scene/
│   ├── scene_graph.hpp         ← 线程安全场景容器
│   ├── scene_node.hpp          ← 场景节点
│   ├── transform.hpp           ← 位置/旋转/缩放
│   └── bounding_box.hpp        ← 轴对齐包围盒
├── src/
│   ├── scene_graph.cpp
│   └── transform.cpp
├── test/
│   └── scene_graph_test.cpp
└── CMakeLists.txt
```

### 3.2 Transform

```cpp
// scene/include/opengeolab/scene/transform.hpp

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace OpenGeoLab::Scene {

/// 位置/旋转/缩放变换
struct Transform {
    glm::vec3 translation{0.f};
    glm::vec3 rotation{0.f};       ///< Euler angles (degrees)
    glm::vec3 scale{1.f};

    /// 组合变换矩阵 T * R * S
    [[nodiscard]] glm::mat4 matrix() const;

    /// 恒等变换
    static Transform identity();
};

} // namespace OpenGeoLab::Scene
```

### 3.3 BoundingBox

```cpp
// scene/include/opengeolab/scene/bounding_box.hpp

#include <glm/vec3.hpp>
#include <optional>

namespace OpenGeoLab::Scene {

/// 轴对齐包围盒 (AABB)
struct BoundingBox {
    glm::vec3 min{0.f};
    glm::vec3 max{0.f};

    [[nodiscard]] glm::vec3 center() const;
    [[nodiscard]] float radius() const;

    /// 合并另一个包围盒
    void merge(const BoundingBox& other);

    /// 从 VisualData positions 计算包围盒
    static BoundingBox fromPositions(const std::vector<float>& positions);
};

} // namespace OpenGeoLab::Scene
```

### 3.4 SceneNode

```cpp
// scene/include/opengeolab/scene/scene_node.hpp

#include <opengeolab/core/visual_data.hpp>
#include <opengeolab/scene/bounding_box.hpp>
#include <opengeolab/scene/transform.hpp>

#include <memory>
#include <string>

namespace OpenGeoLab::Scene {

/// 场景图中的一个可渲染对象
struct SceneNode {
    std::string id;                              ///< 唯一标识
    std::string label;                           ///< 显示名
    std::shared_ptr<Core::VisualData> visual;    ///< 渲染数据
    Transform transform;                         ///< 世界变换
    BoundingBox bounds;                          ///< 局部包围盒
    bool visible{true};
};

} // namespace OpenGeoLab::Scene
```

### 3.5 SceneGraph (线程安全)

```cpp
// scene/include/opengeolab/scene/scene_graph.hpp

#include <opengeolab/scene/scene_node.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Scene {

class SceneGraph {
public:
    // ── 写操作 ───────────────────────────────────────
    /// 添加节点，返回分配的 ID
    std::string addNode(SceneNode node);

    /// 移除节点
    void removeNode(std::string_view id);

    /// 替换节点的 VisualData（数据变更时调用）
    void updateVisual(std::string_view id,
                      std::shared_ptr<Core::VisualData> visual);

    /// 设置变换
    void setTransform(std::string_view id, Transform t);

    /// 设置可见性
    void setVisibility(std::string_view id, bool visible);

    // ── 只读查询 ──────────────────────────────────────
    [[nodiscard]] std::optional<SceneNode> findNode(std::string_view id) const;
    [[nodiscard]] std::vector<SceneNode> allNodes() const;

    /// 全场景包围盒
    [[nodiscard]] BoundingBox sceneBounds() const;

    // ── 变更追踪 ──────────────────────────────────────
    struct Changeset {
        std::vector<std::string> added;
        std::vector<std::string> removed;
        std::vector<std::string> visualChanged;
        std::vector<std::string> transformChanged;
        std::vector<std::string> visibilityChanged;
    };

    /// 消费变更集（由 synchronize 调用，清空 pending 列表）
    Changeset consumeChangeset();

    /// 清空所有节点
    void clear();

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<std::string, SceneNode> m_nodes;
    Changeset m_pendingChanges;
    uint32_t m_nextIdCounter{0};
};

} // namespace OpenGeoLab::Scene
```

**线程安全策略：**
- 使用 `std::shared_mutex` 保护
- 写操作：`unique_lock`（addNode / removeNode / update / setTransform / setVisibility）
- 读操作：`shared_lock`（findNode / allNodes / sceneBounds）
- `consumeChangeset()` 使用 `unique_lock`（swap 后清空）

---

## 4. libs/render 设计 (Phase 1)

### 4.1 文件结构

```
libs/render/
├── include/opengeolab/render/
│   ├── camera.hpp              ← 相机状态 (eye-target-up)
│   ├── trackball_controller.hpp ← 四元数 trackball 交互控制器
│   ├── shader_program.hpp      ← GLSL 编译/链接封装
│   ├── gpu_mesh.hpp            ← VAO/VBO/EBO RAII
│   ├── render_scene.hpp        ← 渲染线程侧场景快照
│   ├── render_engine.hpp       ← OpenGL 核心渲染器
│   ├── viewport_item.hpp       ← QQuickFramebufferObject
│   └── viewport_renderer.hpp   ← FBO 内部渲染器
├── src/
│   ├── camera.cpp
│   ├── trackball_controller.cpp
│   ├── shader_program.cpp
│   ├── gpu_mesh.cpp
│   ├── render_scene.cpp
│   ├── render_engine.cpp
│   ├── viewport_item.cpp
│   └── viewport_renderer.cpp
├── shaders/
│   ├── phong.vert
│   ├── phong.frag
│   ├── edge.vert
│   └── edge.frag
├── test/
│   └── camera_test.cpp
└── CMakeLists.txt
```

### 4.2 Camera

Camera 使用 "eye + target + up" 模型（与参考项目一致），配合 TrackballController 实现旋转。

```cpp
// render/include/opengeolab/render/camera.hpp

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace OpenGeoLab::Render {

/// 相机状态：eye-target-up 模型
class Camera {
public:
    [[nodiscard]] glm::mat4 viewMatrix() const;
    [[nodiscard]] glm::mat4 projectionMatrix(float aspect_ratio) const;

    [[nodiscard]] const glm::vec3& position() const;
    [[nodiscard]] const glm::vec3& target() const;
    [[nodiscard]] const glm::vec3& up() const;
    [[nodiscard]] float fov() const;
    [[nodiscard]] float nearPlane() const;
    [[nodiscard]] float farPlane() const;

    void setPosition(const glm::vec3& pos);
    void setTarget(const glm::vec3& target);
    void setUp(const glm::vec3& up);
    void setFov(float degrees);

    /// 根据 eye-target 距离自动调整 near/far
    void updateClipping(float distance);

    /// 适配包围盒
    void fitToBoundingBox(const glm::vec3& center, float radius);

    /// 重置到默认状态
    void reset();

private:
    glm::vec3 m_position{0.f, 0.f, 50.f};
    glm::vec3 m_target{0.f, 0.f, 0.f};
    glm::vec3 m_up{0.f, 1.f, 0.f};
    float m_fov{45.f};
    float m_nearPlane{0.1f};
    float m_farPlane{10000.f};
};

} // namespace OpenGeoLab::Render
```

### 4.3 TrackballController

从参考项目 (`OGL/include/render/trackball_controller.hpp`) 适配，
使用四元数 trackball 旋转，提供更自然的 3D 交互体验。
核心算法保持不变，类型从 Qt 数学类型迁移到 glm。

```cpp
// render/include/opengeolab/render/trackball_controller.hpp

#include <opengeolab/render/camera.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace OpenGeoLab::Render {

/// Trackball 相机控制器
class TrackballController {
public:
    enum class Mode { None, Orbit, Pan, Zoom };

    TrackballController();

    void setViewportSize(float width, float height);
    void setSpeed(float speed);

    /// 从当前 Camera 状态同步内部四元数
    void syncFromCamera(const Camera& camera);

    [[nodiscard]] bool isActive() const;
    [[nodiscard]] Mode mode() const;

    /// 开始拖拽交互
    void begin(float x, float y, Mode mode, const Camera& camera);

    /// 拖拽中更新 Camera
    void update(float x, float y, Camera& camera);

    /// 结束拖拽
    void end();

    /// 滚轮缩放
    void wheelZoom(float steps, Camera& camera);

private:
    void computePointOnSphere(const glm::vec2& p, glm::vec3& out) const;
    glm::quat rotationBetweenVectors(const glm::vec3& u, const glm::vec3& v) const;
    void freezeFromCamera(const Camera& camera);

    void updateCameraEyeUp(bool update_eye, bool update_up, Camera& camera);
    glm::vec3 computeCameraEye(const Camera& camera);
    glm::vec3 computeCameraUp();
    glm::vec3 computePan(const Camera& camera, const glm::vec2& delta) const;

    void applyOrbit(Camera& camera);
    void applyPan(const glm::vec2& delta, Camera& camera);
    void applyZoomFromDelta(const glm::vec2& delta, Camera& camera);
    void addScrollImpulse(float steps);

    float m_viewportWidth{1.f};
    float m_viewportHeight{1.f};
    float m_speed{1.f};

    Mode m_mode{Mode::None};
    bool m_dragging{false};

    glm::vec2 m_clickPos{0.f};
    glm::vec2 m_prevPos{0.f};

    glm::vec3 m_startVec{0.f, 0.f, 1.f};
    glm::vec3 m_stopVec{0.f, 0.f, 1.f};

    glm::quat m_rotation{1.f, 0.f, 0.f, 0.f};
    glm::quat m_rotationSum{1.f, 0.f, 0.f, 0.f};

    float m_translateLength{50.f};

    float m_orbitScale{2.2f};
    float m_panScale{0.0015f};
    float m_zoomSpeed{1.5f};
    float m_zoomBase{0.90f};
    float m_zoomPixelsPerStep{60.f};

    float m_zoomSum{0.f};
};

} // namespace OpenGeoLab::Render
```

**与参考实现的关系：**
- 核心算法（`computePointOnSphere` → 四元数旋转 → `freezeFromCamera` 同步）100% 保留
- 类型迁移：`QVector3D` → `glm::vec3`，`QQuaternion` → `glm::quat`，`QMatrix4x4` → `glm::mat4`
- 去掉对 `Render::CameraState` 的引用，直接使用 `Camera` 类

### 4.4 ShaderProgram

```cpp
// render/include/opengeolab/render/shader_program.hpp

#include <glad/gl.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>
#include <string_view>

namespace OpenGeoLab::Render {

/// GLSL 着色器编译/链接 RAII 封装
class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    /// 从源码字符串编译链接，失败返回 false
    bool compile(std::string_view vertex_source, std::string_view fragment_source);

    void bind() const;
    void release() const;

    // ── Uniform 设置 ──────────────────────────────
    void setUniform(const char* name, const glm::mat4& mat) const;
    void setUniform(const char* name, const glm::mat3& mat) const;  ///< 法线矩阵
    void setUniform(const char* name, const glm::vec3& vec) const;
    void setUniform(const char* name, const glm::vec4& vec) const;
    void setUniform(const char* name, float value) const;
    void setUniform(const char* name, int value) const;

    [[nodiscard]] GLuint programId() const;

private:
    GLuint m_program{0};

    static GLuint compileShader(GLenum type, std::string_view source);
};

} // namespace OpenGeoLab::Render
```

### 4.5 GpuMesh

```cpp
// render/include/opengeolab/render/gpu_mesh.hpp

#include <opengeolab/core/visual_data.hpp>

#include <glad/gl.h>
#include <cstdint>

namespace OpenGeoLab::Render {

/// VAO/VBO/EBO RAII 封装
class GpuMesh {
public:
    /// 从 SurfaceMesh 创建（position + normal，带 index）
    static GpuMesh fromSurface(const Core::SurfaceMesh& mesh);

    /// 从 EdgeMesh 创建（position，带 index）
    static GpuMesh fromEdges(const Core::EdgeMesh& mesh);

    ~GpuMesh();

    GpuMesh(GpuMesh&& other) noexcept;
    GpuMesh& operator=(GpuMesh&& other) noexcept;

    // 禁止拷贝
    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;

    void draw() const;              ///< glDrawElements(GL_TRIANGLES)
    void drawLines() const;         ///< glDrawElements(GL_LINES)

    [[nodiscard]] int indexCount() const;
    [[nodiscard]] bool isValid() const;

private:
    GpuMesh() = default;

    GLuint m_vao{0};
    GLuint m_vbo{0};
    GLuint m_ebo{0};
    int m_indexCount{0};
    GLenum m_mode{GL_TRIANGLES};

    void destroy();
};

} // namespace OpenGeoLab::Render
```

**顶点布局约定 (SurfaceMesh)：**
```
VBO layout: [pos.x, pos.y, pos.z, norm.x, norm.y, norm.z] per vertex
             stride = 6 * sizeof(float)
attrib 0 (position): offset 0, 3 floats
attrib 1 (normal):   offset 12, 3 floats
```

**顶点布局约定 (EdgeMesh)：**
```
VBO layout: [pos.x, pos.y, pos.z] per vertex
             stride = 3 * sizeof(float)
attrib 0 (position): offset 0, 3 floats
```

### 4.6 RenderScene — 渲染线程侧场景快照

```cpp
// render/include/opengeolab/render/render_scene.hpp

#include <opengeolab/render/gpu_mesh.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

namespace OpenGeoLab::Render {

/// 渲染线程持有的单个场景对象
struct RenderNode {
    std::string id;
    glm::mat4 modelMatrix{1.f};
    std::vector<GpuMesh> surfaces;        ///< GPU 面网格
    std::vector<GpuMesh> edges;           ///< GPU 边线
    Core::RenderStyle style{Core::RenderStyle::SolidWithEdges};
    bool visible{true};
};

/// 渲染线程场景快照，在 synchronize() 时从 SceneGraph 更新
class RenderScene {
public:
    /// 应用 SceneGraph 的变更集
    void applyChangeset(const Scene::SceneGraph::Changeset& changes,
                        const Scene::SceneGraph& graph);

    [[nodiscard]] const std::vector<RenderNode>& nodes() const;

    void clear();

private:
    std::vector<RenderNode> m_nodes;

    /// 从 SceneNode 创建 RenderNode（上传 GPU 缓冲）
    static RenderNode createRenderNode(const Scene::SceneNode& scene_node);
};

} // namespace OpenGeoLab::Render
```

### 4.7 RenderEngine

```cpp
// render/include/opengeolab/render/render_engine.hpp

#include <opengeolab/render/camera.hpp>
#include <opengeolab/render/render_scene.hpp>
#include <opengeolab/render/shader_program.hpp>

namespace OpenGeoLab::Render {

/// OpenGL 核心渲染器，仅在 Render Thread 调用
class RenderEngine {
public:
    /// 首次调用时初始化 GL 状态、编译 shader
    void initialize();

    /// 窗口大小变化
    void resize(int width, int height);

    /// 主渲染通道
    void render(const RenderScene& scene, const Camera& camera);

    [[nodiscard]] bool isInitialized() const;

private:
    ShaderProgram m_phongShader;
    ShaderProgram m_edgeShader;
    int m_width{0};
    int m_height{0};
    bool m_initialized{false};

    void renderSurfaces(const RenderScene& scene, const Camera& camera);
    void renderEdges(const RenderScene& scene, const Camera& camera);
};

} // namespace OpenGeoLab::Render
```

### 4.8 ViewportItem

```cpp
// render/include/opengeolab/render/viewport_item.hpp

#include <opengeolab/render/camera.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <QQuickFramebufferObject>
#include <QPointF>

namespace OpenGeoLab::Render {

/// QML 可见的 3D 视口
class ViewportItem : public QQuickFramebufferObject {
    Q_OBJECT

public:
    explicit ViewportItem(QQuickItem* parent = nullptr);

    Renderer* createRenderer() const override;

    // ── Scene 绑定 ────────────────────────────────
    void setSceneGraph(Scene::SceneGraph* graph);
    [[nodiscard]] Scene::SceneGraph* sceneGraph() const;

    // ── Camera 访问（主线程）──────────────────────
    Camera& camera();
    const Camera& camera() const;

    // ── 适配场景 ──────────────────────────────────
    Q_INVOKABLE void fitAll();

signals:
    void sceneChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void geometryChange(const QSizeF& newGeometry,
                        const QSizeF& oldGeometry) override;
    ///< 在此调用 m_trackball.setViewportSize()

private:
    Camera m_camera;
    TrackballController m_trackball;         ///< 四元数 trackball 控制器
    Scene::SceneGraph* m_sceneGraph{nullptr};
    Qt::MouseButton m_activeButton{Qt::NoButton};
    Qt::KeyboardModifiers m_activeModifiers;
};

} // namespace OpenGeoLab::Render
```

**鼠标交互映射 (Phase 1 — Navigate Only，与参考项目一致)：**

| 操作 | 效果 | 说明 |
|------|------|------|
| Ctrl+左键拖拽 | TrackballController::Orbit | 四元数 trackball 旋转 |
| Shift+左键拖拽 / 中键拖拽 | TrackballController::Pan | 视平面内平移 |
| 右键拖拽 | TrackballController::Zoom (drag) | 上下拖拽调整距离 |
| 滚轮 | TrackballController::wheelZoom | 指数缩放，无需修饰键 |

交互流程：
1. `mousePressEvent` → 根据按键+修饰符确定 `TrackballController::Mode`
2. `m_trackball.begin(x, y, mode, camera)` → 冻结当前 Camera 状态
3. `mouseMoveEvent` → `m_trackball.update(x, y, camera)` → Camera 更新
4. `mouseReleaseEvent` → `m_trackball.end()`
5. 每次 Camera 更新后调用 `update()` 请求重绘

### 4.9 ViewportRenderer

```cpp
// render/include/opengeolab/render/viewport_renderer.hpp

#include <opengeolab/render/camera.hpp>
#include <opengeolab/render/render_engine.hpp>
#include <opengeolab/render/render_scene.hpp>

#include <QOpenGLFramebufferObject>
#include <QQuickFramebufferObject>

namespace OpenGeoLab::Render {

/// QQuickFramebufferObject::Renderer 实现，运行在 Render Thread
class ViewportRenderer : public QQuickFramebufferObject::Renderer {
public:
    ViewportRenderer();

    QOpenGLFramebufferObject* createFramebufferObject(
        const QSize& size) override;

    /// synchronize: 主线程被阻塞时拷贝 Camera + 消费 Changeset
    void synchronize(QQuickFramebufferObject* item) override;

    /// render: 执行 OpenGL 绘制
    void render() override;

private:
    RenderEngine m_engine;
    RenderScene m_renderScene;
    Camera m_cameraCopy;
    int m_width{0};
    int m_height{0};
};

} // namespace OpenGeoLab::Render
```

**synchronize/render 流程：**

```
synchronize() [主线程阻塞]:
  1. 从 ViewportItem 拷贝 Camera 状态
  2. 从 SceneGraph 获取并消费 Changeset
  3. 应用 Changeset 到 RenderScene（创建/删除/更新 GPU 缓冲）

render() [Render Thread]:
  1. RenderEngine.initialize()  (首次)
  2. RenderEngine.resize(w, h)  (如变化)
  3. RenderEngine.render(scene, camera)
     a. 清屏 (背景色)
     b. 绑定 Phong Shader → 设置 MVP + 光照 → 遍历节点绘制 surfaces
     c. 绑定 Edge Shader → 设置 MVP → 遍历节点绘制 edges
```

---

## 5. 着色器设计

### 5.1 Phong 顶点着色器

```glsl
// shaders/phong.vert
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;   // transpose(inverse(model)) 的 3x3

out vec3 vWorldPos;
out vec3 vNormal;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    gl_Position = uProjection * uView * worldPos;
}
```

### 5.2 Phong 片段着色器

```glsl
// shaders/phong.frag
#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uEyePos;
uniform vec4 uObjectColor;   // Phase 1: 始终使用 SurfaceMesh.defaultColor
                              // Phase 2: 如有 per-vertex color 则改用顶点属性

// 固定方向光 + 环境光
const vec3 LIGHT_DIR = normalize(vec3(0.3, 0.8, 0.5));
const vec3 LIGHT_COLOR = vec3(1.0);
const float AMBIENT = 0.2;
const float SPECULAR_STRENGTH = 0.3;
const float SHININESS = 32.0;

out vec4 fragColor;

void main() {
    vec3 N = normalize(vNormal);
    // 双面光照
    if (!gl_FrontFacing) N = -N;

    vec3 L = LIGHT_DIR;
    vec3 V = normalize(uEyePos - vWorldPos);
    vec3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), SHININESS) * SPECULAR_STRENGTH;

    vec3 color = uObjectColor.rgb * (AMBIENT + diff * LIGHT_COLOR) + spec * LIGHT_COLOR;
    fragColor = vec4(color, uObjectColor.a);
}
```

### 5.3 Edge 顶点着色器

```glsl
// shaders/edge.vert
#version 330 core

layout(location = 0) in vec3 aPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
```

### 5.4 Edge 片段着色器

```glsl
// shaders/edge.frag
#version 330 core

uniform vec4 uLineColor;

out vec4 fragColor;

void main() {
    fragColor = uLineColor;
}
```

**深度偏移策略：**
渲染 edges 时启用 `glPolygonOffset(-1.0, -1.0)` + `glEnable(GL_POLYGON_OFFSET_LINE)` 或在 edge shader 中对深度做微量偏移，避免 z-fighting。
实际实现中将在 RenderEngine 的 renderEdges 阶段使用 `glDepthRange(0.0, 0.9999)` 来确保线框渲染在面之上。

---

## 6. 线程模型

### Phase 1 只涉及两个线程

```
Main Thread (QML Event Loop)
  │
  ├─ 鼠标事件 → ViewportItem.mouseMoveEvent()
  │              → TrackballController.update() → Camera 更新
  │              → item->update()  (请求重绘)
  │
  ├─ RequestService → Worker Thread
  │   → GeometryModule / MeshModule 产出 VisualData
  │   → 回到 Main Thread (signal)
  │   → SceneGraph.addNode() / updateVisual()   ← 这里触发 Changeset
  │   → ViewportItem->update()                   ← 请求重绘
  │
Render Thread (Qt Quick Scene Graph)
  │
  ├─ synchronize()  ← 主线程被阻塞
  │  1. 拷贝 Camera 状态
  │  2. SceneGraph.consumeChangeset()
  │  3. RenderScene.applyChangeset() → 上传新 Mesh 到 GPU
  │
  ├─ render()
  │  1. RenderEngine.render(scene, camera)
  │  2. Phong pass → Edge pass
  │
  └─ createFramebufferObject()
     → 窗口大小变化时重建 FBO
```

**线程安全规则：**

| 组件 | 写入线程 | 读取线程 | 保护方式 |
|------|---------|---------|---------|
| SceneGraph | Main Thread | Render (via sync) | `std::shared_mutex` |
| Camera | Main Thread | Render (via sync) | synchronize() 值拷贝 |
| RenderEngine | Render only | — | 单线程，无需锁 |
| GpuMesh | Render only | — | 单线程，无需锁 |
| RenderScene | Render only | — | 单线程，无需锁 |

---

## 7. 数据流桥接

### 7.1 VisualData → SceneGraph → Viewport 的数据流

Phase 1 不创建 SceneModule (ModuleBase)。
数据流桥接由 App 层 C++ 完成，而不是通过 JSON 协议。

**VisualData 数据粒度约定：**
每个 VisualData 包含最多一个 SurfaceMesh、最多一个 EdgeMesh、最多一个 PointSet。
它们可以为空（positions 为空数组）。RenderScene 在创建 GpuMesh 时
跳过空的子网格即可。

```
1. 用户在 QML 触发 geometry/mesh action
2. RequestService 在 Worker Thread 调用 CommandDispatcher
3. GeometryModule / MeshModule 产出 VisualData
4. dataChanged signal → ModuleDataNotifier → Main Thread
5. App 层监听到变更:
   a. 从 GeometryStore / MeshStore 获取 VisualData
   b. 调用 SceneGraph.addNode() 或 updateVisual()
   c. 调用 ViewportItem->update()
6. Qt 触发 synchronize() + render()
```

### 7.2 SceneBridge (App 层桥接类)

在 App 层新增一个 `SceneBridge` 类，负责：
- 监听 geometry / mesh 模块的 dataChanged
- 从 Store 中获取最新 VisualData
- 自动同步到 SceneGraph
- 为新增/删除/更新的数据维护 sceneNodeId 映射

**获取 Store 引用路径：**
- `dispatcher.findModule("geometry")` → `dynamic_pointer_cast<GeometryModule>` → `shapeStore()`
- `dispatcher.findModule("mesh")` → `dynamic_pointer_cast<MeshModule>` → `meshStore()`

```cpp
// app/src/scene_bridge.h

class SceneBridge : public QObject {
    Q_OBJECT
public:
    explicit SceneBridge(OpenGeoLab::Command::CommandDispatcher& dispatcher,
                         OpenGeoLab::Scene::SceneGraph& sceneGraph,
                         QObject* parent = nullptr);

public slots:
    void onGeometryDataChanged();
    void onMeshDataChanged();

private:
    void syncGeometryToScene();
    void syncMeshToScene();

    /// 通用同步：对比 currentIds 与 nodeMap，新增 addNode、删除 removeNode、更新 updateVisual
    /// syncGeometryToScene 和 syncMeshToScene 均使用此模式：
    /// 1. 从 Store 获取当前所有 ID 集合
    /// 2. 遍历 nodeMap，若 ID 不在当前集合中 → sceneGraph.removeNode() + 删除映射
    /// 3. 遍历当前集合，若 ID 不在 nodeMap 中 → sceneGraph.addNode() + 添加映射
    /// 4. 遍历当前集合，若 ID 已在 nodeMap 中 → sceneGraph.updateVisual()

    OpenGeoLab::Command::CommandDispatcher& m_dispatcher;
    OpenGeoLab::Scene::SceneGraph& m_sceneGraph;

    // shapeId → sceneNodeId 映射
    std::unordered_map<uint32_t, std::string> m_shapeNodeMap;
    std::unordered_map<uint32_t, std::string> m_meshNodeMap;
};
```

### 7.3 main.cpp 集成

```cpp
// 新增初始化:
Scene::SceneGraph sceneGraph;
SceneBridge sceneBridge(dispatcher, sceneGraph);

// 手动注册 ViewportItem QML 类型（不使用 QML_ELEMENT，因为 render 是普通库而非 QML module）
qmlRegisterType<Render::ViewportItem>("OpenGeoLab.Render", 1, 0, "ViewportItem");

// 在 QML engine 初始化后将 sceneGraph 传给 ViewportItem
// (通过 property 或 rootContext 注入)
```

### 7.4 ModuleDataNotifier 扩展

现有 `ModuleDataNotifier` 仅有 `geometryDataChanged()` 信号。
Phase 1 需新增 `meshDataChanged()` 信号，并在构造函数中订阅 mesh 模块的 dataChanged：

```cpp
// module_data_notifier.h 新增:
Q_SIGNALS:
    void geometryDataChanged();  // 已有
    void meshDataChanged();       // 新增
```

SceneBridge 的槽函数连接到 ModuleDataNotifier 的 Qt 信号（而非直接订阅 dispatcher），
确保在 Main Thread 上执行 SceneGraph 写操作：

```cpp
connect(&notifier, &ModuleDataNotifier::geometryDataChanged,
        &sceneBridge, &SceneBridge::onGeometryDataChanged);
connect(&notifier, &ModuleDataNotifier::meshDataChanged,
        &sceneBridge, &SceneBridge::onMeshDataChanged);
```

---

## 8. QML 集成

### 8.1 ViewportPanel.qml 改造

```qml
// 替换原有的 Canvas 占位符

import QtQuick
import OpenGeoLab.Render 1.0

Item {
    id: root

    required property AppTheme theme

    ViewportItem {
        id: viewport
        anchors.fill: parent
    }

    // Fit All 按钮
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        width: 32; height: 32
        radius: root.theme.radiusSmall
        color: fitMouse.containsMouse ? root.theme.surfaceStrong : root.theme.surface
        z: 10

        Text {
            anchors.centerIn: parent
            text: "⊞"
            color: root.theme.textPrimary
            font.pixelSize: 16
        }

        MouseArea {
            id: fitMouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: viewport.fitAll()
        }
    }
}
```

### 8.2 Main.qml 无需修改

ViewportPanel 保持原有 Layout 位置，只是内部从 Canvas → ViewportItem。

---

## 9. CMake 集成

### 9.1 新增第三方依赖 — GLM 和 glad

```cmake
# 根 CMakeLists.txt 新增，在 Third-party 区域

# GLM (已有 version 变量，新增 resolve 调用)
opengeolab_resolve_package(
    glm
    glm::glm
    VERSION ${OPENGEOLAB_GLM_VERSION}
    GITHUB_REPOSITORY g-truc/glm
    GIT_TAG ${OPENGEOLAB_GLM_VERSION}
)

# glad (OpenGL loader, 仅 render 使用)
# 注意: glad2 使用子目录模式集成，不通过 opengeolab_resolve_package，
# 因为 glad2 需要特殊的 GLAD_API 选项来指定 GL profile。
# 如果 CPM 方式有问题，备选方案是使用 glad 在线生成器生成 glad.c + glad/gl.h，
# 直接放入 libs/render/third_party/glad/ 作为源文件编译。
set(OPENGEOLAB_GLAD_VERSION 2.0.8)
CPMAddPackage(
    NAME glad
    GITHUB_REPOSITORY Dav1dde/glad
    VERSION ${OPENGEOLAB_GLAD_VERSION}
    GIT_TAG v${OPENGEOLAB_GLAD_VERSION}
    OPTIONS
        "GLAD_API gl:core=4.6"
        "GLAD_INSTALL ON"
)
```

### 9.2 libs/scene/CMakeLists.txt

```cmake
opengeolab_add_module(
    opengeolab_scene
    ALIAS_NAME Scene
    PUBLIC_HEADERS
        include/opengeolab/scene/scene_graph.hpp
        include/opengeolab/scene/scene_node.hpp
        include/opengeolab/scene/transform.hpp
        include/opengeolab/scene/bounding_box.hpp
    SOURCES
        src/scene_graph.cpp
        src/transform.cpp
    PUBLIC_LINKS
        OpenGeoLab::Core
        glm::glm
)
```

### 9.3 libs/render/CMakeLists.txt

```cmake
opengeolab_add_module(
    opengeolab_render
    ALIAS_NAME Render
    PUBLIC_HEADERS
        include/opengeolab/render/camera.hpp
        include/opengeolab/render/trackball_controller.hpp
        include/opengeolab/render/shader_program.hpp
        include/opengeolab/render/gpu_mesh.hpp
        include/opengeolab/render/render_scene.hpp
        include/opengeolab/render/render_engine.hpp
        include/opengeolab/render/viewport_item.hpp
        include/opengeolab/render/viewport_renderer.hpp
    SOURCES
        src/camera.cpp
        src/trackball_controller.cpp
        src/shader_program.cpp
        src/gpu_mesh.cpp
        src/render_scene.cpp
        src/render_engine.cpp
        src/viewport_item.cpp
        src/viewport_renderer.cpp
    PUBLIC_LINKS
        OpenGeoLab::Core
        OpenGeoLab::Scene
        Qt6::Quick
        Qt6::OpenGL
        glm::glm
    PRIVATE_LINKS
        glad        # OpenGL loader, 不暴露给其他模块
)

# 着色器作为资源嵌入（Qt6 qt_add_resources 自动将文件关联到目标，无需额外 target_sources）
qt_add_resources(opengeolab_render "shaders"
    PREFIX "/shaders"
    FILES
        shaders/phong.vert
        shaders/phong.frag
        shaders/edge.vert
        shaders/edge.frag
)
```

### 9.4 根 CMakeLists.txt 新增 subdirectory

```cmake
add_subdirectory(src/libs/core)
add_subdirectory(src/libs/io)
add_subdirectory(src/libs/geometry)
add_subdirectory(src/libs/mesh)
add_subdirectory(src/libs/scene)      # ← 新增
add_subdirectory(src/libs/render)     # ← 新增
add_subdirectory(src/libs/command)
add_subdirectory(src/libs/python)
add_subdirectory(src/app)
```

### 9.5 app CMakeLists.txt 链接更新

```cmake
target_link_libraries(
    opengeolab_app
    PRIVATE ...existing...
            OpenGeoLab::Render
            OpenGeoLab::Scene
)
```

---

## 10. 测试策略

### 10.1 libs/scene 测试

```
scene_graph_test.cpp:
  - Transform: matrix() 正确性 (identity / translate / rotate / scale)
  - BoundingBox: fromPositions / center / radius / merge
  - SceneGraph: addNode → findNode → allNodes
  - SceneGraph: removeNode → findNode 返回 nullopt
  - SceneGraph: updateVisual → changeset 包含 visualChanged
  - SceneGraph: consumeChangeset → 消费后清空
  - SceneGraph: sceneBounds → 正确合并所有可见节点
```

### 10.2 libs/render 测试

```
camera_test.cpp (同时覆盖 Camera 和 TrackballController):
  Camera:
  - 默认状态 → viewMatrix / projectionMatrix 非零
  - fitToBoundingBox → position/target 更新
  - updateClipping → near/far 合理
  - reset → 回到默认值
  TrackballController:
  - begin + update(orbit) → Camera position 变化
  - begin + update(pan) → Camera target 平移
  - wheelZoom → Camera distance 变化
  - syncFromCamera → 内部四元数一致性
```

> **注意**: RenderEngine / GpuMesh / ShaderProgram 需要 OpenGL context，
> 在 CI 环境可能无法运行。这些组件的测试以手动验证为主，
> 或在未来添加 offscreen GL context 测试设施。

---

## 11. 实现顺序建议

```
Task 0: CMake 基础设施
  ├─ 根 CMakeLists.txt 添加 GLM resolve + glad
  ├─ 根 CMakeLists.txt 添加 scene + render subdirectory
  └─ 验证: cmake 配置通过

Task 1: Scene 核心类型
  ├─ Transform + BoundingBox (纯数学，可独立测试)
  └─ 验证: 编译 + 单元测试

Task 2: SceneGraph
  ├─ 线程安全容器 + Changeset 机制
  └─ 验证: 编译 + 单元测试

Task 3: Camera + TrackballController
  ├─ Camera (eye-target-up 状态 + viewMatrix/projectionMatrix)
  ├─ TrackballController (从参考项目适配，Qt→glm 类型迁移)
  └─ 验证: 编译 + 单元测试

Task 4: ShaderProgram
  ├─ GLSL 编译链接封装
  └─ (需要 GL context，无单元测试)

Task 5: GpuMesh
  ├─ VAO/VBO/EBO RAII
  └─ (需要 GL context，无单元测试)

Task 6: RenderScene
  ├─ Changeset → GPU 缓冲管理
  └─ (需要 GL context，无单元测试)

Task 7: RenderEngine
  ├─ Phong + Edge 两阶段渲染
  └─ (需要 GL context，无单元测试)

Task 8: 着色器文件
  ├─ phong.vert/frag + edge.vert/frag
  └─ 嵌入 Qt 资源

Task 9: ViewportItem + ViewportRenderer
  ├─ QQuickFramebufferObject 集成
  ├─ 鼠标交互
  └─ 验证: 手动 — 窗口中显示空白 GL 视口

Task 10: QML 集成
  ├─ ViewportPanel.qml 改造
  ├─ QML 类型注册
  └─ 验证: 手动 — 窗口中 ViewportItem 正确嵌入

Task 11: SceneBridge + main.cpp 集成
  ├─ App 层桥接 geometry/mesh → SceneGraph
  ├─ ModuleDataNotifier 扩展
  └─ 验证: 手动 — 创建 Box → 自动渲染到视口

Task 12: 最终验证
  ├─ 全量构建 + 全量测试
  ├─ 手动验证: 创建几何体 → 渲染 + 鼠标交互 + Fit All
  └─ clang-format + clang-tidy
```

---

## 12. Phase 2 预留接口

Phase 1 完成后，以下接口已预留，供 Phase 2 扩展：

| 接口 | Phase 1 状态 | Phase 2 扩展 |
|------|-------------|-------------|
| SceneGraph::Changeset | ✅ 完整 | Phase 2 无需修改 |
| SceneNode 结构体 | ✅ 字段完整 | Phase 2 添加 selected / EntityTag 缓存 |
| Camera JSON 序列化 | ❌ 未实现 | Phase 2 添加 toJson/fromJson |
| EntityRegistry | ❌ 不在范围 | Phase 2 新增到 scene |
| SelectionManager | ❌ 不在范围 | Phase 2 新增到 scene |
| PickRenderer | ❌ 不在范围 | Phase 2 新增到 render |
| SceneModule | ❌ 不在范围 | Phase 2 新增 ModuleBase + actions |
| RenderModule | ❌ 不在范围 | Phase 2 新增 ModuleBase + actions |
| PointSet 渲染 | ❌ 不在范围 | Phase 2 添加 point shader + GpuMesh::fromPoints |
| 地面网格 | ❌ 不在范围 | Phase 2 添加 grid shader |
