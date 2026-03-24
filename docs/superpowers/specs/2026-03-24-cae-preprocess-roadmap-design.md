# OpenGeoLab CAE 前处理开发路线 — 设计规格

> **状态**: Draft
> **范围**: OCC 几何内核 · GMSH 网格剖分 · OpenGL 4.5 渲染 · 拾取/框选 · Python 脚本录制
> **目标**: 搭建从 CAD 几何建模到有限元网格剖分的完整前处理管线

---

## 1. 全局架构

```
┌─────────────────────────────────────────────────────────────┐
│  QML UI Layer  (app/resource/qml/)                          │
│  Main.qml · 参数对话框 · 场景树 · Viewport · Activity      │
└──────────────┬──────────────────────────────────┬───────────┘
               │ Qt 桥接层 (app/)                 │
               │ GLViewportItem                   │
               │ ViewportController               │
               │ SceneTreeModel                   │
               │ CommandBridge                     │
               ▼                                  ▼
┌──────────────────────────┐  ┌───────────────────────────────┐
│  libs/render/            │  │  libs/command/                │
│  (纯 C++, 无 Qt)         │  │  (纯 C++, 无 Qt)              │
│  RenderEngine            │  │  Command / CommandStack       │
│  PassManager + Passes    │  │  CommandRecorder              │
│  Camera                  │  │  PythonScriptExporter         │
│  PickEngine              │  │                               │
│  SceneGraph              │  │                               │
└─────────┬────────────────┘  └───────────────────────────────┘
          │ 消费 RenderMeshData
          ▼
┌──────────────────────────┐
│  libs/scene/             │
│  (纯 C++, 数据中间表示)   │
│  SceneNode / SceneGraph  │
│  RenderMeshData          │
│  EntityRegistry          │
│  SelectionSet            │
└─────────┬────────────────┘
          │ 适配器
          ▼
┌──────────────┐  ┌──────────────┐
│ libs/occ/    │  │ libs/mesh/   │
│ OCC 几何内核  │  │ GMSH 网格    │
│ ShapeStore   │  │ MeshStore    │
│ OccActions   │  │ GmshActions  │
│ Tessellator  │  │ MeshAdapter  │
└──────────────┘  └──────────────┘
```

### 1.1 模块依赖关系

```
libs/scene ← libs/occ (OCC 适配器写入 SceneNode)
libs/scene ← libs/mesh (GMSH 适配器写入 SceneNode)
libs/scene ← libs/render (Render 消费 SceneNode/RenderMeshData)
libs/scene ← libs/command (Command 操作 Scene 数据)
libs/render ← libs/command (Camera 命令操作 Render)
以上全部 ← app/ (Qt 桥接层)
```

### 1.2 关键约束

- `libs/render`、`libs/command`、`libs/scene`、`libs/occ`、`libs/mesh` 均为**纯 C++**，不依赖 Qt。
- Qt 相关代码（QQuickFramebufferObject、QAbstractItemModel、信号槽）仅存在于 `app/`。
- 所有模块通过 `opengeolab_add_module()` CMake 宏定义，遵循现有构建模式。
- C++20 标准，遵循仓库 `.clang-format` 和 `.clang-tidy`。
- 注释使用 `/** */` 块样式。

---

## 2. libs/render — 渲染模块

### 2.1 图形 API

- **OpenGL 4.5+ Core Profile**
- 使用 DSA (Direct State Access) 减少状态切换
- 支持 Compute Shader 和 SSBO
- GLAD 或 GL3W 作为 loader

### 2.2 Render Pass 架构

采用**可组合 Pass 图**设计：

```cpp
/**
 * @brief 单个渲染通道的抽象接口。
 *
 * 每个 Pass 声明自己的输入/输出资源需求，
 * PassManager 根据优先级和依赖关系自动排序执行。
 */
class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    /** @brief 描述 Pass 的资源需求（输入/输出纹理、FBO 等）。 */
    virtual PassDescriptor describe() const = 0;

    /** @brief 分配 GPU 资源（在 GL context 可用后调用）。 */
    virtual void setup(PassResources& resources) = 0;

    /** @brief 执行渲染。 */
    virtual void execute(RenderContext& ctx) = 0;

    /** @brief 释放 GPU 资源。 */
    virtual void teardown() = 0;
};

/**
 * @brief Pass 注册中心与执行调度器。
 */
class PassManager {
public:
    void registerPass(std::string name,
                      std::unique_ptr<IRenderPass> pass,
                      int priority);
    void setPassEnabled(std::string_view name, bool enabled);
    void execute(RenderContext& ctx);
};
```

**初始 Pass 列表：**

| Pass | 优先级 | 描述 |
|------|--------|------|
| `GridPass` | 100 | 无限网格（XZ 平面） |
| `GeometryPass` | 200 | 三角面片实体渲染（Phong / PBR） |
| `WireframePass` | 300 | 线框叠加 |
| `HighlightPass` | 400 | 选中实体高亮（轮廓 / 叠色） |
| `PickPass` | 500 | GPU 拾取：渲染 entity ID 到 `GL_R32UI` 纹理 |
| `GizmoPass` | 600 | 坐标轴、变换手柄 |

后续可扩展：
- `MeshQualityPass` — 网格质量热力图
- `BoundaryConditionPass` — 边界条件可视化（箭头、颜色区域）
- `SectionCutPass` — 剖面可视化
- `OutlinePass` — 后处理轮廓描边

### 2.3 Camera

```cpp
class Camera {
public:
    /** @brief 设置透视投影。 */
    void setPerspective(float fovDeg, float aspect, float nearPlane, float farPlane);

    /** @brief 设置正交投影。 */
    void setOrthographic(float width, float aspect, float nearPlane, float farPlane);

    /** @brief 轨道旋转（绕 target）。 */
    void orbit(float deltaTheta, float deltaPhi);

    /** @brief 平移。 */
    void pan(float dx, float dy);

    /** @brief 缩放。 */
    void zoom(float factor);

    /** @brief 适配视图到包围盒。 */
    void fitAll(const BoundingBox& bbox);

    /** @brief 获取完整相机状态快照（用于录制）。 */
    CameraState captureState() const;

    /** @brief 恢复相机状态（用于回放）。 */
    void restoreState(const CameraState& state);

    /** @brief 屏幕坐标 → 世界射线（用于拾取）。 */
    Ray screenToWorldRay(float screenX, float screenY,
                         int viewportWidth, int viewportHeight) const;
};
```

### 2.4 PickEngine（混合拾取）

```cpp
class PickEngine {
public:
    /**
     * @brief GPU 拾取：读取 GL_R32UI 纹理的像素值。
     *
     * PickPass 将每个可拾取实体编码为 uint32 ID 渲染到离屏 FBO。
     * 此方法读取指定像素处的 ID 并查找对应实体。
     *
     * @param x, y 屏幕像素坐标。
     * @param filter 拾取过滤器（Vertex/Edge/Face/Body）。
     */
    PickResult pickAtPixel(int x, int y, PickFilter filter);

    /**
     * @brief CPU 视锥框选：将屏幕矩形转为视锥体，查询 BVH。
     *
     * @param region 屏幕空间矩形。
     * @param filter 拾取过滤器。
     */
    std::vector<PickResult> pickInRegion(Rect2D region, PickFilter filter);
};

enum class PickFilter { Vertex, Edge, Face, Body, All };

struct PickResult {
    int entityId;
    PickFilter entityType;
    glm::vec3 worldPosition;
    float depth;
};
```

### 2.5 SceneGraph（渲染侧）

```cpp
/**
 * @brief 渲染侧场景图，持有 GPU 资源引用。
 *
 * 从 libs/scene 的 SceneNode 树转换而来。
 * 每个 RenderNode 对应一个 VAO + 变换矩阵。
 */
class RenderSceneGraph {
public:
    void updateFromScene(const Scene::SceneGraph& sceneGraph);
    void draw(RenderContext& ctx);
    RenderNode* findNode(int entityId);
};
```

### 2.6 RenderEngine 顶层

```cpp
class RenderEngine {
public:
    void initialize();
    void resize(int width, int height);
    void render();

    Camera& camera();
    PassManager& passManager();
    PickEngine& pickEngine();
    RenderSceneGraph& sceneGraph();
};
```

---

## 3. libs/scene — 场景数据中间表示

### 3.1 核心数据结构

```cpp
/** @brief 渲染用网格数据，由 OCC/GMSH 适配器生成。 */
struct RenderMeshData {
    std::vector<float> positions;    // xyz interleaved
    std::vector<float> normals;      // xyz interleaved
    std::vector<uint32_t> indices;
    PrimitiveType topology;          // Triangles, Lines, Points
};

enum class EntityType { Body, Face, Edge, Vertex, MeshRegion };

/** @brief 场景节点，可包含几何体或网格。 */
struct SceneNode {
    int id;
    std::string name;
    EntityType type;
    glm::mat4 transform{1.0f};
    BoundingBox bounds;
    std::vector<RenderMeshData> meshes;
    std::vector<SceneNode> children;
    bool visible = true;
    bool selected = false;
};

/** @brief 全局场景图，持有所有节点。 */
class SceneGraph {
public:
    SceneNode& root();
    SceneNode* findById(int id);
    void addNode(SceneNode node, int parentId = 0);
    void removeNode(int id);
    Signal<void()> changed;  // Kangaroo signal
};
```

### 3.2 EntityRegistry

```cpp
/**
 * @brief 全局实体 ID 注册表。
 *
 * 每个可拾取实体（Body、Face、Edge、Vertex）获得唯一 uint32 ID。
 * PickPass 使用此 ID 渲染到 GL_R32UI 纹理。
 * PickEngine 通过此表将像素 ID 映射回实体。
 */
class EntityRegistry {
public:
    uint32_t registerEntity(int sceneNodeId, EntityType type);
    EntityInfo lookup(uint32_t pickId) const;
    void unregisterNode(int sceneNodeId);
};
```

### 3.3 SelectionSet

```cpp
/**
 * @brief 当前选中实体集合。
 *
 * 支持多种选择模式（替换、添加、移除、切换）。
 * 选择变更发出信号，驱动 HighlightPass 更新。
 */
class SelectionSet {
public:
    void select(int entityId, SelectionMode mode = SelectionMode::Replace);
    void selectMultiple(std::span<const int> entityIds, SelectionMode mode);
    void clear();

    [[nodiscard]] bool isSelected(int entityId) const;
    [[nodiscard]] std::vector<int> selectedIds() const;

    Signal<void()> changed;
};

enum class SelectionMode { Replace, Add, Remove, Toggle };
```

---

## 4. libs/command — 命令系统

### 4.1 Command 基类

```cpp
/**
 * @brief 可撤销、可录制的命令基类。
 *
 * 每个命令记录 source（user/script/replay）用于控制录制行为。
 */
class Command {
public:
    virtual ~Command() = default;

    virtual void execute() = 0;
    virtual void undo() = 0;

    /** @brief 序列化为 Python 代码片段。 */
    virtual std::string toPython() const = 0;

    /** @brief 命令的人类可读描述。 */
    virtual std::string description() const = 0;

    CommandSource source() const;
    void setSource(CommandSource source);
};

enum class CommandSource { User, Script, Replay };
```

### 4.2 CommandStack

```cpp
class CommandStack {
public:
    void execute(std::unique_ptr<Command> cmd);
    void undo();
    void redo();

    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;

    Signal<void(const Command&)> commandExecuted;
    Signal<void()> stackChanged;
};
```

### 4.3 CommandRecorder

```cpp
/**
 * @brief 监听 CommandStack，仅录制 source=User 的命令。
 *
 * 输出 Python 脚本代码，可用于 headless 回放。
 */
class CommandRecorder {
public:
    void startRecording();
    void stopRecording();
    [[nodiscard]] bool isRecording() const;

    /** @brief 导出录制的命令序列为 Python 脚本。 */
    std::string exportPython() const;
    void clear();
};
```

### 4.4 预定义命令示例

| 命令类 | toPython() 输出 | 模块 |
|--------|-----------------|------|
| `CreateBoxCommand` | `ogl.geometry.create_box(center=[0,0,0], size=[1,1,1])` | OCC |
| `SetCameraStateCommand` | `ogl.camera.set_state(eye=[...], target=[...])` | Render |
| `SelectEntitiesCommand` | `ogl.selection.select([42, 43], mode="add")` | Scene |
| `FitAllCommand` | `ogl.camera.fit_all()` | Render |
| `GenerateMeshCommand` | `ogl.mesh.generate(shape_id=1, size=0.5)` | GMSH |
| `SetBoundaryConditionCommand` | `ogl.bc.set_load(face_id=12, force=[0,0,-100])` | Scene |

---

## 5. libs/occ — OCC 几何内核

### 5.1 定位

- **唯一几何内核**，当前 `libs/geometry` 的 BoxData 仅为测试占位。
- 依赖 OpenCASCADE 7.8+（通过 `find_package(OpenCASCADE)` 或 CPM）。
- 暴露 JSON 分发接口，与现有 `GeometryModule::process()` 模式一致。

### 5.2 ShapeStore

```cpp
/**
 * @brief 线程安全的 OCC 形状存储。
 *
 * 持有 TopoDS_Shape 对象，分配唯一 ID。
 * 提供拓扑查询（faces, edges, vertices）。
 */
class ShapeStore {
public:
    int addShape(TopoDS_Shape shape, std::string label);
    void removeShape(int id);
    [[nodiscard]] TopoDS_Shape getShape(int id) const;
    [[nodiscard]] std::vector<std::pair<int, ShapeInfo>> allShapes() const;
};
```

### 5.3 Tessellator（OCC → RenderMeshData 适配器）

```cpp
/**
 * @brief 将 OCC TopoDS_Shape 三角化为 RenderMeshData。
 *
 * 使用 BRepMesh_IncrementalMesh 进行离散化，
 * 提取三角面片、法线和拓扑边界线。
 */
class Tessellator {
public:
    struct Options {
        double linearDeflection = 0.1;
        double angularDeflection = 0.5;
        bool relative = true;
    };

    /** @brief 三角化实体面。 */
    Scene::RenderMeshData tessellate(const TopoDS_Shape& shape,
                                      const Options& options);

    /** @brief 提取拓扑边界线。 */
    Scene::RenderMeshData extractEdges(const TopoDS_Shape& shape);
};
```

### 5.4 OccActions

| Action | 描述 | 参数 |
|--------|------|------|
| `create_box` | 创建长方体 | center, size |
| `create_cylinder` | 创建圆柱 | center, radius, height, axis |
| `create_sphere` | 创建球体 | center, radius |
| `create_torus` | 创建环面体 | center, majorR, minorR |
| `boolean_fuse` | 布尔并集 | shapeA_id, shapeB_id |
| `boolean_cut` | 布尔差集 | shapeA_id, shapeB_id |
| `boolean_common` | 布尔交集 | shapeA_id, shapeB_id |
| `fillet` | 倒圆角 | shape_id, edge_ids, radius |
| `chamfer` | 倒斜角 | shape_id, edge_ids, distance |
| `import_step` | 导入 STEP | filePath |
| `export_step` | 导出 STEP | shape_ids, filePath |
| `list_shapes` | 列出所有形状 | — |

---

## 6. libs/mesh — GMSH 网格模块

### 6.1 定位

- 依赖 GMSH SDK（通过 `find_package(GMSH)` 或编译集成）。
- 从 OCC ShapeStore 获取 TopoDS_Shape，传给 GMSH 进行网格剖分。
- 结果存入 MeshStore 并通过 MeshAdapter 转换为 RenderMeshData。

### 6.2 MeshStore

```cpp
class MeshStore {
public:
    int addMesh(MeshData mesh, int sourceShapeId);
    [[nodiscard]] MeshData getMesh(int id) const;
    [[nodiscard]] std::vector<std::pair<int, MeshInfo>> allMeshes() const;
};
```

### 6.3 GmshActions

| Action | 描述 | 参数 |
|--------|------|------|
| `generate` | 生成网格 | shape_id, elementSize, order, algorithm |
| `set_local_size` | 设置局部网格尺寸 | shape_id, entity_ids, size |
| `refine` | 全局细化 | mesh_id |
| `optimize` | 网格优化 | mesh_id, algorithm |
| `quality_report` | 网格质量报告 | mesh_id |
| `list_meshes` | 列出所有网格 | — |

### 6.4 MeshAdapter（GMSH → RenderMeshData）

```cpp
class MeshAdapter {
public:
    /** @brief 将 GMSH 网格转为渲染用三角面片。 */
    Scene::RenderMeshData toTriangles(const MeshData& mesh);

    /** @brief 将 GMSH 网格转为线框。 */
    Scene::RenderMeshData toWireframe(const MeshData& mesh);
};
```

---

## 7. app/ 桥接层

### 7.1 GLViewportItem

```cpp
/**
 * @brief QQuickFramebufferObject 子类，桥接 QML 与 RenderEngine。
 *
 * 位于 app/ 中，是唯一依赖 Qt 的渲染入口。
 * 在 Renderer 的 synchronize() 中同步 camera/scene 状态，
 * 在 render() 中调用 RenderEngine::render()。
 */
class GLViewportItem : public QQuickFramebufferObject {
    Q_OBJECT

    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)

public:
    Renderer* createRenderer() const override;

    /** @brief 鼠标事件转发给 ViewportController。 */
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
};
```

### 7.2 ViewportController

```cpp
/**
 * @brief 鼠标事件 → 语义命令的翻译层。
 *
 * 根据当前交互模式（Orbit/Pan/Pick/BoxSelect），
 * 将鼠标事件转换为对应的 Command 对象并提交到 CommandStack。
 */
class ViewportController {
public:
    enum class InteractionMode { Orbit, Pan, Pick, BoxSelect };

    void setMode(InteractionMode mode);
    void onMousePress(int x, int y, Qt::MouseButtons buttons, Qt::KeyboardModifiers mods);
    void onMouseMove(int x, int y, Qt::MouseButtons buttons);
    void onMouseRelease(int x, int y);
    void onWheel(int delta);
};
```

### 7.3 SceneTreeModel

```cpp
/**
 * @brief QAbstractItemModel 适配器，将 SceneGraph 暴露给 QML TreeView。
 *
 * 监听 SceneGraph::changed 信号自动更新。
 * 支持选中同步：TreeView 选中 ↔ SelectionSet 同步。
 */
class SceneTreeModel : public QAbstractItemModel { ... };
```

### 7.4 CommandBridge

```cpp
/**
 * @brief 将 libs/command 暴露给 QML 的桥接。
 *
 * Q_PROPERTY: canUndo, canRedo, isRecording, recordedScript
 * Q_INVOKABLE: undo(), redo(), startRecording(), stopRecording(), exportScript()
 */
class CommandBridge : public QObject { ... };
```

---

## 8. QML UI 改造

### 8.1 参数对话框

为每个几何创建操作设计独立的参数对话框组件：

```
src/app/resource/qml/dialogs/
  ├── CreateBoxDialog.qml       — center, size
  ├── CreateCylinderDialog.qml  — center, radius, height, axis
  ├── CreateSphereDialog.qml    — center, radius
  ├── CreateTorusDialog.qml     — center, majorR, minorR
  ├── MeshSettingsDialog.qml    — elementSize, order, algorithm
  └── ImportDialog.qml          — filePath, format
```

**通用模式：**
- 浮动 Popup（非模态），锚定在 Viewport 上方
- 参数字段绑定到临时属性
- "确认" 按钮创建 Command → 提交到 CommandStack
- "取消" 关闭对话框

### 8.2 场景树改造

**SidebarPanel.qml** 从简单 ListView 升级为 TreeView：

```
SidebarPanel
  ├── TreeView (SceneTreeModel)
  │   ├── Body 1 (Box)
  │   │   ├── Face 1
  │   │   ├── Face 2
  │   │   └── ...
  │   ├── Body 2 (Cylinder)
  │   └── Mesh 1 (associated with Body 1)
  │       ├── Region: surface
  │       └── Region: volume
  └── PropertyPanel (选中实体的属性)
```

### 8.3 ViewportPanel 替换

**ViewportPanel.qml** 从 Canvas 占位替换为 `GLViewportItem`：

```qml
GLViewportItem {
    anchors.fill: parent
    darkMode: root.darkMode

    // 交互覆盖层（框选矩形、坐标显示）
    Rectangle {
        id: boxSelectOverlay
        visible: false
        color: "transparent"
        border.color: AppTheme.accent
    }
}
```

### 8.4 Ribbon 更新

- Geometry Tab: 点击 Box/Cylinder/Sphere/Torus → 弹出对应参数对话框
- Mesh Tab: 点击 Generate Mesh → 弹出 MeshSettingsDialog
- 新增 Undo/Redo 按钮（在 Header 或快捷键）

### 8.5 Script Recorder UI

MenuConfig 中已有的按钮连接到 CommandBridge：

| 按钮 | 绑定 |
|------|------|
| Start Script Record | `CommandBridge.startRecording()` |
| Replay Script | `CommandBridge.replayScript(path)` |
| Export Record | `CommandBridge.exportScript()` → 文件选择 → 保存 .py |
| Clear Script History | `CommandBridge.clear()` |

---

## 9. 开发阶段划分

### Phase 1: 渲染基础设施

**目标**: 在 QML Viewport 中看到带网格的 3D 空间。

- [ ] `libs/render`: RenderEngine、Camera、PassManager、GridPass
- [ ] `libs/scene`: SceneNode、RenderMeshData、SceneGraph（空场景）
- [ ] `app/`: GLViewportItem + ViewportController（Orbit/Pan/Zoom）
- [ ] QML: ViewportPanel 替换为 GLViewportItem
- [ ] 验证: 3D 网格 + 鼠标旋转/平移/缩放

### Phase 2: OCC 几何与显示

**目标**: 用 OCC 创建参数化几何体并在 Viewport 中渲染。

- [ ] 移除旧 `libs/geometry`（BoxData/SceneStore/GeometryModule 均为测试占位），其测试也一并删除
- [ ] `libs/occ`: ShapeStore、Tessellator、create_box/cylinder/sphere/torus
- [ ] `libs/scene`: EntityRegistry 集成
- [ ] `libs/render`: GeometryPass、WireframePass
- [ ] QML: CreateBoxDialog 等参数对话框
- [ ] QML: SidebarPanel → 场景树（SceneTreeModel）
- [ ] 验证: 通过对话框创建 Box → OCC → 三角化 → Viewport 渲染

### Phase 3: 拾取与选择

**目标**: 支持单击拾取和框选，选中高亮。

- [ ] `libs/render`: PickPass（GL_R32UI）、PickEngine
- [ ] `libs/scene`: SelectionSet
- [ ] `libs/render`: HighlightPass（选中实体轮廓/叠色）
- [ ] `app/`: ViewportController Pick/BoxSelect 模式
- [ ] QML: 框选覆盖层、选中状态反馈到场景树
- [ ] 验证: 单击选面/边 + 框选多实体 + TreeView 联动

### Phase 4: Command 系统与脚本录制

**目标**: 所有操作通过 Command 执行，支持 undo/redo 和 Python 脚本导出。

- [ ] `libs/command`: Command、CommandStack、CommandRecorder
- [ ] 将 Phase 2-3 的操作（创建几何、选择、相机）封装为 Command
- [ ] `app/`: CommandBridge → QML
- [ ] QML: Script Recorder 菜单连接
- [ ] Python: headless 回放入口
- [ ] 验证: 创建 Box → 旋转相机 → 选中面 → 导出 .py → headless 回放

### Phase 5: GMSH 网格剖分

**目标**: 对 OCC 几何体进行有限元网格剖分并可视化。

- [ ] `libs/mesh`: GMSH 集成、MeshStore、GmshActions、MeshAdapter
- [ ] `libs/render`: 网格渲染（三角面片 + 线框叠加）
- [ ] QML: MeshSettingsDialog、网格质量展示
- [ ] 场景树: 网格节点关联到几何体
- [ ] 验证: 创建 Box → 网格剖分 → Viewport 中显示网格

### Phase 6: CAE 扩展

**目标**: 边界条件设置、高级可视化。

- [ ] 边界条件数据模型
- [ ] `libs/render`: BoundaryConditionPass、MeshQualityPass
- [ ] QML: 边界条件设置面板
- [ ] 布尔操作 UI（Fuse/Cut/Common）
- [ ] STEP 导入/导出
- [ ] 验证: 完整 CAE 前处理工作流

---

## 10. 第三方依赖新增

| 依赖 | 版本 | 用途 | 集成方式 |
|------|------|------|----------|
| **OpenCASCADE** | 7.8+ | B-Rep 几何内核 | `find_package` / vcpkg |
| **GMSH** | 4.12+ | 有限元网格生成 | `find_package` / SDK |
| **GLM** | 1.0+ | 数学库（矩阵、向量） | CPM |
| **GLAD** | 0.1.36+ | OpenGL 4.5 loader | CPM 或生成代码 |

---

## 11. 测试策略

| 层 | 测试方式 |
|----|----------|
| `libs/scene` | doctest 单元测试：SceneGraph CRUD、EntityRegistry 分配/回收 |
| `libs/command` | doctest：Command execute/undo/toPython、CommandStack undo/redo、Recorder 过滤 |
| `libs/occ` | doctest：ShapeStore 存取、Tessellator 输出顶点数/法线、OccActions JSON 往返 |
| `libs/mesh` | doctest：MeshAdapter 转换、GmshActions JSON 往返 |
| `libs/render` | 离屏 GL context 测试（Camera 矩阵、Pass 注册排序） |
| `app/` | QML TestCase 或手动验证（GLViewportItem 集成） |
| 集成 | 端到端：创建 Box → 网格 → 选中 → 录制 → 回放 |

---

## 12. 录制与回放详细设计

### 12.1 命令源标记

```
enum class CommandSource {
    User,    // 用户通过 UI 操作产生 → 录制
    Script,  // Python 脚本调用产生 → 不录制
    Replay   // 回放引擎产生 → 不录制
};
```

### 12.2 录制流程

```
用户点击 "Create Box" 对话框 → 确认
  ↓
QML 创建 CreateBoxCommand(source=User)
  ↓
CommandStack.execute(cmd)
  ↓
cmd.execute() → OCC 创建 Box → SceneGraph 更新
  ↓
CommandRecorder 捕获 (source=User) → 记录 cmd.toPython()
  ↓
录制日志: "ogl.geometry.create_box(center=[0,0,0], size=[1,1,1])"
```

### 12.3 回放流程

```
Python 脚本:
  import opengeolab as ogl
  ogl.geometry.create_box(center=[0,0,0], size=[1,1,1])
  ogl.camera.set_state(eye=[5,5,5], target=[0,0,0])
  ogl.selection.select([1], mode="replace")
    ↓
每行调用 → 创建 Command(source=Script 或 Replay)
    ↓
CommandStack.execute(cmd) → 执行但不录制
    ↓
SceneGraph / Camera / Selection 更新
```

### 12.4 相机状态录制

相机操作特殊处理：**不录制每个 orbit/pan/zoom，而是在关键时刻插入快照**：

- 用户停止鼠标操作 0.5s 后 → 自动插入 `SetCameraStateCommand`
- 或用户手动点击 "Insert Camera Bookmark"
- 这样回放脚本简洁且状态确定

---

## 13. 风险与缓解

| 风险 | 影响 | 缓解 |
|------|------|------|
| OCC 编译体积大 | 构建时间增长 | 使用预编译包（vcpkg/conda） |
| GMSH 许可证 (GPL) | 传播约束 | GMSH SDK 通过动态链接使用 |
| OpenGL 4.5 兼容性 | macOS 不支持 | 先 Windows/Linux，macOS 用 MoltenVK 或降级 |
| SceneGraph 同步复杂 | 主线程阻塞 | 使用双缓冲：写入线程 → 快照 → 渲染线程 |
| Command 粒度设计 | 过细则冗余，过粗则不可 undo | 以用户可感知的最小操作为粒度 |
