# Geometry Module Design — Phase 1: Geometry Core

> Status: Draft
> Date: 2026-03-28

## 1. 目标与范围

为 OpenGeoLab 提供基于 OpenCASCADE (OCC) 的几何内核模块，统一管理 CAD 几何对象的
创建、导入、离散化和拾取标注，为后续的场景图渲染和 GMSH 网格剖分提供完整的几何基础设施。

### 本次范围（阶段 1）

- **ShapeStore** — OCC 几何存储与子形状索引
- **文件导入** — import_brep / import_step（从 IO 模块迁入 geometry）
- **参数化创建** — create_box / create_cylinder / create_sphere / create_torus（替换现有 mock）
- **离散化** — BRepMesh 三角化 + 提取点/边/面渲染数据
- **VisualData 生成** — 面（SurfaceMesh）+ 边（EdgeMesh）+ 点（PointSet）
- **EntityTag 标注** — 为拾取系统提供 GeoVertex / GeoEdge / GeoWire / GeoFace / GeoSolid 语义
- **信号通知** — ShapeStore 发出 shapeAdded / shapeRemoved / shapeUpdated 信号供 scene 订阅
- **查询与管理** — list_shapes / query_shape / delete_shape / tessellate

### 不在本次范围

- 几何操作（布尔运算、倒角、去重、缝合、清理）— 阶段 2
- GMSH 网格剖分联动 — 阶段 3
- scene / render 模块实现（本文只定义 geometry 的输出接口和信号协议）

---

## 2. 架构位置与依赖

### 2.1 依赖图

```
scene    → geometry, core          (订阅 geometry 信号；相对 render-scene 规格新增 geometry 依赖)
geometry → core, OpenCASCADE       (几何内核)
render   → scene, core, Qt6::OpenGL, GLM, glad
mesh     → geometry, core          (未来阶段 3)
command  → core, io, geometry, scene, render
io       → core                    (保留用于非几何 IO)
```

> **注意：** render-scene 架构设计中 scene 仅依赖 core。本规格新增了 scene → geometry
> 依赖（用于订阅 ShapeStore 信号获取 VisualData）。这是依赖图的增量演进，scene 模块
> 实现时需包含 geometry 头文件。

### 2.2 关键设计决策

| 决策项 | 结论 | 理由 |
|--------|------|------|
| 几何持有层 | geometry 模块内部 ShapeStore | OCC 依赖隔离在 geometry 内，scene/render 零 OCC 依赖 |
| ShapeId | uint32_t 自增，槽位分配器 | O(1) 查找，cache-friendly，支持高效删除和复用 |
| 文件导入 | 归 geometry 模块 | OCC Shape 无法通过 JSON 传递；FreeCAD/Salome 同模式 |
| 模块通信 | 信号/观察者模式（Kangaroo::Util::Signal） | geometry 不知道 scene 存在，无反向依赖 |
| 离散化 | BRepMesh_IncrementalMesh + 数据提取 | OCC 内置，零外部依赖，质量足够 CAD 显示 |
| 拾取编码 | 直接编码 shapeId+localId+entityType 到拾取缓冲区 | 消除 EntityRegistry map 查找热点，O(1) 解码 |
| 子形状索引 | TopTools_IndexedMapOfShape | OCC 生态标准，FindKey(localId) = O(1) 数组访问 |

### 2.3 全局数据流

```
用户请求 (JSON)
     │
     ▼
GeometryModule.process()
     │
     ├─ import_step/import_brep → OCC 读入 → ShapeStore.add()
     ├─ create_box/cylinder/... → OCC 建模 → ShapeStore.add()
     ├─ tessellate              → BRepMesh  → 更新 VisualData
     ├─ query_shape             → ShapeStore 查询 → JSON 返回
     ├─ delete_shape            → ShapeStore.remove()
     └─ list_shapes             → ShapeStore 遍历 → JSON 返回
                                      │
                            ShapeStore::Signal
                                      │
                      ┌───────────────┼───────────────┐
                      ▼               ▼               ▼
               shapeAdded      shapeUpdated     shapeRemoved
               (scene 订阅)   (scene 订阅)     (scene 订阅)
                      │               │               │
                      ▼               ▼               ▼
              SceneGraph.addNode  updateNode     removeNode
              (VisualData+Tags)
```

---

## 3. ShapeStore — 几何存储核心

### 3.1 ShapeEntry

```cpp
// geometry/include/opengeolab/geometry/shape_entry.hpp

#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS_Shape.hxx>
#include <opengeolab/core/visual_data.hpp>
#include <opengeolab/core/entity_tag.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief 每个顶层几何对象的完整索引与缓存
 *
 * 持有 OCC 原始几何、子形状索引（用于拾取回查）和离散化缓存（用于渲染）。
 * 子形状索引使用 TopTools_IndexedMapOfShape，FindKey(localId) 为 O(1) 数组访问。
 */
struct ShapeEntry {
    uint32_t id{0};                        ///< ShapeStore 分配的唯一 ID
    std::string name;                      ///< 用户可见名称 "Box_1"
    TopoDS_Shape shape;                    ///< 顶层 OCC 几何

    // ── 子形状索引（创建/导入时构建，修改后重建）──
    TopTools_IndexedMapOfShape vertexMap;   ///< localId (1-based) → TopoDS_Vertex
    TopTools_IndexedMapOfShape edgeMap;     ///< localId (1-based) → TopoDS_Edge
    TopTools_IndexedMapOfShape wireMap;     ///< localId (1-based) → TopoDS_Wire
    TopTools_IndexedMapOfShape faceMap;     ///< localId (1-based) → TopoDS_Face
    TopTools_IndexedMapOfShape solidMap;    ///< localId (1-based) → TopoDS_Solid

    // ── 离散化缓存（tessellate 后填充）──
    std::shared_ptr<Core::VisualData> visualData;
    std::vector<Core::EntityTag> triangleTags;  ///< 每三角形 → {GeoFace, faceLocalId}
    std::vector<Core::EntityTag> edgeTags;      ///< 每线段 → {GeoEdge, edgeLocalId}
    std::vector<Core::EntityTag> vertexTags;    ///< 每点 → {GeoVertex, vertexLocalId}
};

} // namespace OpenGeoLab::Geometry
```

### 3.2 ShapeStore

```cpp
// geometry/include/opengeolab/geometry/shape_store.hpp

#include <opengeolab/geometry/shape_entry.hpp>
#include <kangaroo/util/signal.hpp>

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief 几何对象的集中存储与管理
 *
 * 使用槽位分配器实现 shapeId → ShapeEntry 的 O(1) 查找。
 * 删除的槽位通过 freeList 复用。
 *
 * 线程安全：内部使用 std::mutex 保护所有写操作。
 * 信号在锁外发出，避免在持锁期间执行订阅者回调。
 */
class ShapeStore {
public:
    ShapeStore();
    ~ShapeStore();

    // ── 写操作 ──

    /**
     * @brief 添加一个 shape 到 store
     *
     * 自动构建子形状索引（TopExp::MapShapes）。
     * 不会自动离散化——调用 tessellate() 触发。
     *
     * @param name  用户可见名称
     * @param shape OCC 几何对象
     * @return 分配的 shapeId
     */
    uint32_t add(const std::string& name, const TopoDS_Shape& shape);

    /**
     * @brief 删除指定 shape
     * @param shape_id 要删除的 shapeId
     * @throws std::out_of_range 如果 shapeId 不存在
     */
    void remove(uint32_t shape_id);

    /**
     * @brief 替换指定 shape 的几何（用于布尔运算等修改操作）
     *
     * 重建子形状索引，清除离散化缓存。
     *
     * @param shape_id 要更新的 shapeId
     * @param new_shape 新的 OCC 几何
     * @throws std::out_of_range 如果 shapeId 不存在
     */
    void update(uint32_t shape_id, const TopoDS_Shape& new_shape);

    /**
     * @brief 对指定 shape 执行离散化
     *
     * 使用 BRepMesh_IncrementalMesh 三角化，然后提取
     * SurfaceMesh / EdgeMesh / PointSet 和对应的 EntityTag。
     * 完成后发出 shapeUpdated 信号。
     *
     * @param shape_id     目标 shapeId
     * @param linear_deflection  BRepMesh 线性偏差（默认 0.1）
     * @param angular_deflection BRepMesh 角度偏差（默认 0.5，弧度）
     * @throws std::out_of_range 如果 shapeId 不存在
     */
    void tessellate(uint32_t shape_id,
                    double linear_deflection = 0.1,
                    double angular_deflection = 0.5);

    // ── 只读查询 ──

    /**
     * @brief 通过 shapeId 获取 ShapeEntry（只读）
     * @return 指针，nullptr 如果不存在
     */
    [[nodiscard]] const ShapeEntry* find(uint32_t shape_id) const;

    /**
     * @brief 获取所有有效 shape 的 ID 列表
     */
    [[nodiscard]] std::vector<uint32_t> allShapeIds() const;

    /**
     * @brief 当前有效 shape 数量
     */
    [[nodiscard]] std::size_t size() const;

    // ── 信号 ──

    /**
     * @brief shape 被添加（在 add() 完成后发出）
     *
     * 参数：shapeId, ShapeEntry 的只读引用
     * 注意：此时 ShapeEntry 尚未离散化（visualData 为空）
     */
    Kangaroo::Util::Signal<uint32_t, const ShapeEntry&> shapeAdded;

    /**
     * @brief shape 被删除（在 remove() 完成后发出）
     *
     * 参数：被删除的 shapeId
     */
    Kangaroo::Util::Signal<uint32_t> shapeRemoved;

    /**
     * @brief shape 被更新（在 tessellate() 或 update() 完成后发出）
     *
     * 参数：shapeId, 更新后的 ShapeEntry 只读引用
     * 对于 tessellate：visualData 已填充
     * 对于 update：子形状索引已重建，visualData 已清除
     */
    Kangaroo::Util::Signal<uint32_t, const ShapeEntry&> shapeUpdated;

private:
    /**
     * @brief 为 shape 构建子形状索引
     */
    void buildSubShapeIndex(ShapeEntry& entry);

    mutable std::mutex m_mutex;
    std::vector<std::unique_ptr<ShapeEntry>> m_slots;  ///< shapeId = 数组下标
    std::vector<uint32_t> m_freeList;                  ///< 已删除的空槽复用
    uint32_t m_nextId{0};                              ///< 下一个分配的 ID
};

} // namespace OpenGeoLab::Geometry
```

---

## 4. Tessellator — 离散化引擎

### 4.1 职责

将 OCC BRep 几何转为可渲染的三角面片 + 边线 + 点位数据。

### 4.2 接口

```cpp
// geometry/include/opengeolab/geometry/tessellator.hpp

namespace OpenGeoLab::Geometry {

/**
 * @brief 离散化结果
 */
struct TessellationResult {
    Core::VisualData visualData;           ///< 面 + 边 + 点渲染数据
    std::vector<Core::EntityTag> triangleTags;  ///< 每三角形语义标注
    std::vector<Core::EntityTag> edgeTags;      ///< 每线段语义标注
    std::vector<Core::EntityTag> vertexTags;    ///< 每点语义标注
};

/**
 * @brief 对 OCC shape 执行离散化
 *
 * 流程：
 * 1. BRepMesh_IncrementalMesh 三角化
 * 2. 遍历 faceMap → 提取三角化 → 填入 SurfaceMesh + triangleTags
 * 3. 遍历 edgeMap → 曲线采样 → 填入 EdgeMesh + edgeTags
 * 4. 遍历 vertexMap → 提取坐标 → 填入 PointSet + vertexTags
 *
 * @param entry  具有已构建子形状索引的 ShapeEntry
 * @param linear_deflection  BRepMesh 线性偏差
 * @param angular_deflection BRepMesh 角度偏差
 * @return 离散化结果
 */
[[nodiscard]] TessellationResult tessellate(const ShapeEntry& entry,
                                            double linear_deflection,
                                            double angular_deflection);

} // namespace OpenGeoLab::Geometry
```

### 4.3 离散化流程

```
输入: ShapeEntry (含 shape + 子形状索引)
  │
  ├─ Step 1: BRepMesh_IncrementalMesh(shape, linearDeflection, angularDeflection)
  │           → OCC 内部三角化（修改 shape 上的 Triangulation）
  │
  ├─ Step 2: 遍历 faceMap (1..N)
  │   for i in 1..faceMap.Extent():
  │       face = faceMap.FindKey(i)
  │       triangulation = BRep_Tool::Triangulation(face, location)
  │       提取 positions[], normals[], indices[]
  │       → 一个 SurfaceMesh
  │       所有三角形标注 EntityTag{GeoFace, i}
  │
  ├─ Step 3: 遍历 edgeMap (1..M)
  │   for j in 1..edgeMap.Extent():
  │       edge = edgeMap.FindKey(j)
  │       adaptor = BRepAdaptor_Curve(edge)
  │       均匀采样曲线点 → EdgeMesh
  │       所有线段标注 EntityTag{GeoEdge, j}
  │
  └─ Step 4: 遍历 vertexMap (1..K)
      for k in 1..vertexMap.Extent():
          vertex = vertexMap.FindKey(k)
          point = BRep_Tool::Pnt(vertex)
          → PointSet
          标注 EntityTag{GeoVertex, k}
```

---

## 5. VisualData 扩展 — 新增 PointSet

在 render-scene 架构设计中已定义 `SurfaceMesh` 和 `EdgeMesh`，
本次需要在 `core/visual_data.hpp` 中新增 `PointSet` 以支持顶点渲染和拾取。

```cpp
// 新增到 core/include/opengeolab/core/visual_data.hpp

/**
 * @brief 点集数据（用于 CAD 顶点显示和拾取）
 */
struct PointSet {
    std::vector<float> positions;          ///< [x,y,z, ...]
    float pointSize{5.0f};                 ///< GL_POINTS 尺寸
    glm::vec4 color{1.0f, 0.0f, 0.0f, 1.0f}; ///< 默认红色
};

/**
 * @brief 一个场景对象的完整可视化数据
 */
struct VisualData {
    std::vector<SurfaceMesh> surfaces;     ///< 面数据（每个 OCC Face 一组）
    std::vector<EdgeMesh> edges;           ///< 边线数据
    std::vector<PointSet> points;          ///< 点数据（新增）
    RenderStyle style{RenderStyle::SolidWithEdges};
};
```

---

## 6. EntityTag 扩展 — 新增 GeoWire

在 render-scene 架构设计的 `EntityType` 枚举中新增 `GeoWire`。

```cpp
// 更新 core/include/opengeolab/core/entity_tag.hpp

enum class EntityType : uint8_t {
    GeoVertex  = 0,   ///< OCC 顶点
    GeoEdge    = 1,   ///< OCC 边
    GeoWire    = 2,   ///< OCC 线框 (新增)
    GeoFace    = 3,   ///< OCC 面
    GeoSolid   = 4,   ///< OCC 体

    MeshNode    = 10,  ///< FEM 节点
    MeshEdge    = 11,  ///< FEM 元素边
    MeshElement = 12,  ///< FEM 元素

    SceneNode   = 20   ///< 整个场景对象
};
```

---

## 7. 拾取编码优化建议

### 7.1 问题

原 render-scene 架构中使用单个 `pickId (uint32_t)` + `EntityRegistry` 哈希表映射，
在高频拾取（鼠标移动实时高亮）时 map.find() 可能成为热点。

### 7.2 建议：直接编码到拾取缓冲区

```
Pick FBO 使用 RG32UI 纹理（两个 32 位无符号整数通道）：
  R (32 bits): shapeId(24 bits) << 8 | entityType(8 bits)
  G (32 bits): localId(32 bits)

Pick shader:
  flat out uvec2 v_pickData;        // vertex shader 输出
  v_pickData = uvec2((u_shapeId << 8u) | uint(u_entityType), u_localId);

CPU 回读:
  glReadPixels(x, y, 1, 1, GL_RG_INTEGER, GL_UNSIGNED_INT, &data);
  shapeId    = data[0] >> 8;        // 24 bits → 最大 16,777,215 顶层 shape
  entityType = data[0] & 0xFF;      // 8 bits  → 最大 255 种类型（字节对齐）
  localId    = data[1];             // 32 bits → 最大 4,294,967,295 子形状
```

**优势：**
- 零 map 查找，O(1) 直接解码
- 不需要维护 EntityRegistry 全局映射表
- scene 对象增删不需要重新分配 pickId

**注意：** 此优化影响 render-scene 模块设计，本文仅作建议标注。
geometry 模块只需提供 `{shapeId, entityType, localId}` 三元组。

---

## 8. 信号协议

### 8.1 信号定义

geometry 模块通过 ShapeStore 的三个信号与外部通信：

| 信号 | 参数 | 触发时机 | 订阅者动作（scene） |
|------|------|----------|---------------------|
| `shapeAdded` | `(uint32_t shapeId, const ShapeEntry&)` | add() 完成 | 可等待 tessellate 后再 addNode |
| `shapeRemoved` | `(uint32_t shapeId)` | remove() 完成 | removeNode |
| `shapeUpdated` | `(uint32_t shapeId, const ShapeEntry&)` | tessellate() 或 update() 完成 | addNode 或 updateNode |

### 8.2 典型使用流程

```
1. 用户请求: {"module": "geometry", "action": "import_step", "param": {"path": "model.stp"}}

2. GeometryModule.import_step:
   a. STEPControl_Reader 读入文件 → TopoDS_Shape
   b. ShapeStore.add("Part_1", shape)
      → 构建子形状索引
      → emit shapeAdded(42, entry)       [scene 可选择等待离散化]
   c. ShapeStore.tessellate(42)
      → BRepMesh → 提取 VisualData + EntityTags
      → emit shapeUpdated(42, entry)     [scene 创建 SceneNode]
   d. 返回 JSON: {"ok": true, "shapeId": 42, "name": "Part_1", ...}

3. scene 收到 shapeUpdated:
   → 从 entry 获取 visualData, triangleTags, edgeTags, vertexTags
   → SceneGraph.addNode(...)
   → 渲染线程同步并绘制
```

### 8.3 scene 如何获取 ShapeStore 引用

scene 模块在启动时从 PluginComponentFactory 获取 GeometryModule 单例，
通过 GeometryModule 的公共方法获取 ShapeStore 引用，然后订阅信号。

```cpp
// 在 scene 模块初始化时（或 SceneModule 构造函数中）
auto& geoModule = factory.getInstance<Core::ModuleBase>("geometry");
auto& geoModuleConcrete = static_cast<Geometry::GeometryModule&>(geoModule);
auto& store = geoModuleConcrete.shapeStore();

m_addedConn = store.shapeAdded.connect([this](uint32_t id, const auto& entry) {
    // 当 visualData 可用时添加场景节点
    if (entry.visualData) {
        addSceneNodeFromEntry(id, entry);
    }
});

m_updatedConn = store.shapeUpdated.connect([this](uint32_t id, const auto& entry) {
    if (entry.visualData) {
        updateOrAddSceneNode(id, entry);
    }
});

m_removedConn = store.shapeRemoved.connect([this](uint32_t id) {
    removeSceneNode(id);
});
```

---

## 9. GeometryModule 接口

### 9.1 模块定义

```cpp
// geometry/include/opengeolab/geometry/geometry_module.hpp

class GeometryModule final : public Core::ModuleBase {
public:
    explicit GeometryModule(Kangaroo::Util::PluginComponentFactory& factory);
    ~GeometryModule() override;

    /// 获取 ShapeStore 引用（供 scene 模块订阅信号）
    [[nodiscard]] ShapeStore& shapeStore();
    [[nodiscard]] const ShapeStore& shapeStore() const;

    static constexpr std::string_view MODULE_NAME{"geometry"};

private:
    ShapeStore m_shapeStore;
};
```

### 9.1.1 Action 获取 ShapeStore 的注入方式

所有 geometry action 需要访问 `ShapeStore` 来注册/查询/删除 shape。
注入方式：**通过 `registerAction` 模板传递构造参数**。

```cpp
// GeometryModule 构造函数中
registerAction<CreateBoxAction>(std::ref(m_shapeStore));
registerAction<ImportStepAction>(std::ref(m_shapeStore));
// ...

// 需要扩展 ModuleBase::registerAction 模板以支持额外构造参数：
template <class ActionT, class... Args>
void ModuleBase::registerAction(Args&&... args) {
    std::string key = m_moduleName + "." + std::string(ActionT::ACTION_NAME);
    m_factory.bindSingleton<IAction, ActionT>(key, std::forward<Args>(args)...);
}
```

每个 Action 的构造函数签名示例：
```cpp
class CreateBoxAction final : public Core::IAction {
public:
    explicit CreateBoxAction(ShapeStore& store);
    // ...
private:
    ShapeStore& m_store;
};
```

### 9.2 Actions 列表

| Action | 请求 param | 返回 | 说明 |
|--------|-----------|------|------|
| `import_brep` | `{"path": "...", "name": "...", ...}` | `{"ok": true, "shapeId": N, ...}` | 读入 .brep 文件（+ 可选 tessellate 参数） |
| `import_step` | `{"path": "...", "name": "...", ...}` | `{"ok": true, "shapeId": N, ...}` | 读入 .stp/.step 文件（+ 可选 tessellate 参数） |
| `create_box` | `{"name": "...", "width": W, "height": H, "depth": D, "origin": [x,y,z], ...}` | `{"ok": true, "shapeId": N, ...}` | 创建 Box（+ 可选 tessellate 参数） |
| `create_cylinder` | `{"name": "...", "radius": R, "height": H, "center": [x,y,z], ...}` | `{"ok": true, "shapeId": N, ...}` | 创建 Cylinder（+ 可选 tessellate 参数） |
| `create_sphere` | `{"name": "...", "radius": R, "center": [x,y,z], ...}` | `{"ok": true, "shapeId": N, ...}` | 创建 Sphere（+ 可选 tessellate 参数） |
| `create_torus` | `{"name": "...", "majorRadius": R, "minorRadius": r, "center": [x,y,z], ...}` | `{"ok": true, "shapeId": N, ...}` | 创建 Torus（+ 可选 tessellate 参数） |
| `tessellate` | `{"shapeId": N, "linearDeflection": 0.1, "angularDeflection": 0.5}` | `{"ok": true, ...}` | 对指定 shape 执行/更新离散化 |
| `query_shape` | `{"shapeId": N}` | 拓扑信息 JSON | 查询 shape 信息 |
| `list_shapes` | `{}` | `{"shapes": [...]}` | 列出所有 shape |
| `delete_shape` | `{"shapeId": N}` | `{"ok": true}` | 删除 shape |

### 9.3 Action 详细协议

#### import_step

```json
// 请求
{
  "module": "geometry",
  "action": "import_step",
  "param": {
    "path": "/data/model.stp",
    "name": "Part_1",
    "tessellate": true,
    "linearDeflection": 0.1,
    "angularDeflection": 0.5
  }
}

// 成功返回
{
  "ok": true,
  "action": "import_step",
  "shapeId": 42,
  "name": "Part_1",
  "topology": {
    "solids": 1,
    "faces": 6,
    "edges": 12,
    "vertices": 8,
    "wires": 6
  }
}
```

#### import_brep

与 import_step 格式相同，内部使用 `BRepTools::Read()` 代替 `STEPControl_Reader`。

#### create_box

```json
// 请求
{
  "module": "geometry",
  "action": "create_box",
  "param": {
    "name": "Box_1",
    "width": 10.0,
    "height": 5.0,
    "depth": 3.0,
    "origin": [0.0, 0.0, 0.0],
    "tessellate": true
  }
}

// 成功返回
{
  "ok": true,
  "action": "create_box",
  "shapeId": 43,
  "name": "Box_1",
  "topology": {
    "solids": 1,
    "faces": 6,
    "edges": 12,
    "vertices": 8,
    "wires": 6
  }
}
```

#### query_shape

```json
// 请求
{
  "module": "geometry",
  "action": "query_shape",
  "param": {
    "shapeId": 42
  }
}

// 返回
{
  "ok": true,
  "action": "query_shape",
  "shapeId": 42,
  "name": "Part_1",
  "topology": {
    "solids": 1,
    "faces": 6,
    "edges": 12,
    "vertices": 8,
    "wires": 6
  },
  "boundingBox": {
    "min": [0.0, 0.0, 0.0],
    "max": [10.0, 5.0, 3.0]
  },
  "hasTessellation": true
}
```

#### list_shapes

```json
// 请求
{
  "module": "geometry",
  "action": "list_shapes",
  "param": {}
}

// 返回
{
  "ok": true,
  "action": "list_shapes",
  "count": 2,
  "shapes": [
    {"shapeId": 42, "name": "Part_1", "faces": 6, "hasTessellation": true},
    {"shapeId": 43, "name": "Box_1", "faces": 6, "hasTessellation": true}
  ]
}
```

#### delete_shape

```json
// 请求
{
  "module": "geometry",
  "action": "delete_shape",
  "param": {
    "shapeId": 42
  }
}

// 返回
{
  "ok": true,
  "action": "delete_shape",
  "shapeId": 42
}
```

---

## 10. 文件结构

```
libs/geometry/
├── include/opengeolab/geometry/
│   ├── geometry_module.hpp              ← ModuleBase 实现
│   ├── shape_store.hpp                  ← 几何存储核心
│   ├── shape_entry.hpp                  ← ShapeEntry 数据结构
│   ├── tessellator.hpp                  ← 离散化引擎
│   ├── import_step_action.hpp           ← import_step action
│   ├── import_brep_action.hpp           ← import_brep action
│   ├── create_box_action.hpp            ← 替换现有 mock
│   ├── create_cylinder_action.hpp
│   ├── create_sphere_action.hpp
│   ├── create_torus_action.hpp
│   ├── tessellate_action.hpp
│   ├── query_shape_action.hpp
│   ├── list_shapes_action.hpp
│   └── delete_shape_action.hpp
├── src/
│   ├── geometry_module.cpp
│   ├── shape_store.cpp
│   ├── tessellator.cpp
│   ├── import_step_action.cpp
│   ├── import_brep_action.cpp
│   ├── create_box_action.cpp
│   ├── create_cylinder_action.cpp
│   ├── create_sphere_action.cpp
│   ├── create_torus_action.cpp
│   ├── tessellate_action.cpp
│   ├── query_shape_action.cpp
│   ├── list_shapes_action.cpp
│   └── delete_shape_action.cpp
├── test/
│   ├── shape_store_test.cpp
│   ├── tessellator_test.cpp
│   ├── import_actions_test.cpp
│   ├── create_actions_test.cpp
│   └── geometry_module_test.cpp         ← 替换现有测试
└── CMakeLists.txt
```

---

## 11. CMake 变更

### 11.1 geometry 模块

```cmake
# geometry/CMakeLists.txt

# OpenCASCADE 依赖
find_package(OpenCASCADE REQUIRED)

set(geometry_public_headers
    include/opengeolab/geometry/geometry_module.hpp
    include/opengeolab/geometry/shape_store.hpp
    include/opengeolab/geometry/shape_entry.hpp
    include/opengeolab/geometry/tessellator.hpp
    include/opengeolab/geometry/import_step_action.hpp
    include/opengeolab/geometry/import_brep_action.hpp
    include/opengeolab/geometry/create_box_action.hpp
    include/opengeolab/geometry/create_cylinder_action.hpp
    include/opengeolab/geometry/create_sphere_action.hpp
    include/opengeolab/geometry/create_torus_action.hpp
    include/opengeolab/geometry/tessellate_action.hpp
    include/opengeolab/geometry/query_shape_action.hpp
    include/opengeolab/geometry/list_shapes_action.hpp
    include/opengeolab/geometry/delete_shape_action.hpp)

set(geometry_sources
    src/geometry_module.cpp
    src/shape_store.cpp
    src/tessellator.cpp
    src/import_step_action.cpp
    src/import_brep_action.cpp
    src/create_box_action.cpp
    src/create_cylinder_action.cpp
    src/create_sphere_action.cpp
    src/create_torus_action.cpp
    src/tessellate_action.cpp
    src/query_shape_action.cpp
    src/list_shapes_action.cpp
    src/delete_shape_action.cpp)

opengeolab_add_module(
    opengeolab_geometry
    ALIAS_NAME Geometry
    SOURCES ${geometry_sources}
    PUBLIC_HEADERS ${geometry_public_headers}
    PUBLIC_LINKS
        OpenGeoLab::Core
        # OCC 按需链接的组件
        TKernel TKMath TKG3d TKGeomBase TKBRep TKTopAlgo
        TKPrim TKMesh TKSTEP TKSTEPBase TKXSBase TKShHealing)
```

### 11.2 依赖图更新

```
command → core, io, geometry (已有)
scene   → core, geometry (新增 geometry 依赖，用于订阅信号)
```

---

## 12. 拾取集成 — geometry 模块的职责边界

### 12.1 geometry 模块做什么

1. **提供子形状索引**：通过 `ShapeEntry` 的 `vertexMap/edgeMap/wireMap/faceMap/solidMap`
2. **提供 EntityTag 标注**：tessellate 时为每个渲染图元标注 `{entityType, localId}`
3. **提供回查 API**：给定 `(shapeId, entityType, localId)` → 返回 OCC `TopoDS_Shape`

### 12.2 geometry 模块不做什么

1. 不管理 pickId（那是 render/scene 的颜色编码）
2. 不处理鼠标事件（那是 render 层的 ViewportItem）
3. 不管理选择状态（那是 scene 的 SelectionManager）

### 12.3 拾取回查 API

```cpp
// ShapeStore 新增方法

/**
 * @brief 通过拾取结果获取对应的 OCC 子形状
 *
 * @param shape_id  ShapeStore 中的几何对象 ID
 * @param type      实体类型
 * @param local_id  子形状局部索引（1-based）
 * @return OCC 子形状，如果参数无效返回空 shape
 */
[[nodiscard]] TopoDS_Shape getSubShape(uint32_t shape_id,
                                       Core::EntityType type,
                                       uint32_t local_id) const;
```

### 12.4 拾取全链路（跨模块）

```
用户在视口点击
  → render: PickRenderer 读 FBO 像素 → 解码 {shapeId, localId, entityType}
  → scene: SelectionManager 更新选择状态
  → 用户请求"对选中面网格剖分"
  → mesh: 从选择结果获取 (shapeId=42, GeoFace, localId=3)
  → geometry: ShapeStore.getSubShape(42, GeoFace, 3) → TopoDS_Face
  → mesh: 将 TopoDS_Face 传给 GMSH 进行网格剖分
```

---

## 13. 测试策略

### 13.1 单元测试

| 测试文件 | 覆盖范围 |
|----------|----------|
| `shape_store_test.cpp` | add/remove/update/find/allShapeIds + 信号触发验证 |
| `tessellator_test.cpp` | Box/Sphere 离散化 → 验证 SurfaceMesh/EdgeMesh/PointSet 非空 + EntityTag 正确性 |
| `import_actions_test.cpp` | import_brep/import_step（需要测试用 .brep/.stp 文件） |
| `create_actions_test.cpp` | create_box/cylinder/sphere/torus → 验证 shapeId + 拓扑计数 |
| `geometry_module_test.cpp` | 模块级集成：请求分发 + describe() |

### 13.2 测试数据

在 `test/data/` 目录下放置测试用几何文件：
- `box.brep` — 简单 Box BRep 文件
- `simple.stp` — 简单 STEP 文件

### 13.3 构建与运行

```bash
# 构建
cmake --build build --target opengeolab_geometry --config Debug --parallel 4

# 运行测试
ctest --test-dir build -C Debug -R geometry --output-on-failure
```

---

## 14. 与现有代码的兼容性

### 14.1 现有 CreateBoxAction 替换

当前 `create_box_action.cpp` 是一个模拟延迟的 mock 实现。
阶段 1 将替换为真正的 OCC Box 创建 + ShapeStore 注册 + 离散化。

**变更影响：**
- JSON 请求格式：新增 `origin` 参数，其余保持兼容
- JSON 返回格式：新增 `shapeId`、`topology` 字段，`data` 字段移除
- 测试用例：需要更新以适应新的返回格式和无延迟行为

### 14.2 现有 IO 模块 ReadBrepAction

当前 `read_brep_action.cpp` 是 TODO stub（直接 throw）。
阶段 1 中 BRep 读取逻辑迁移到 geometry 模块的 `import_brep` action。
IO 模块的 `read_brep` action 可保留为兼容入口（内部透传到 geometry），
或标记为 deprecated。

---

## 15. 未来扩展点（阶段 2/3 接口预留）

### 15.1 阶段 2：几何操作

ShapeStore 的 `update()` 方法和 `shapeUpdated` 信号已预留布尔运算等修改操作。
新增 action 如 `boolean_cut`、`fillet`、`chamfer` 只需调用 OCC 算法后
`ShapeStore.update(shapeId, newShape)` 即可触发场景图更新。

### 15.2 阶段 3：GMSH 联动

mesh 模块通过 `ShapeStore.getSubShape(shapeId, entityType, localId)` 获取
OCC 子形状传给 GMSH。geometry 模块无需额外修改。
