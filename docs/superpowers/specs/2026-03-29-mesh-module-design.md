# Mesh 网格模块设计规格

> **日期**：2026-03-29
> **状态**：Draft
> **模块**：`OpenGeoLab::Mesh`
> **依赖**：`OpenGeoLab::Core`、Gmsh 4.15.0、运行时弱引用 `OpenGeoLab::Geometry`

---

## 1. 目标与范围

### 1.1 目标

为 OpenGeoLab 新增独立的 Mesh 网格模块，使用 Gmsh 库实现：

- **2D 面网格剖分**：对 OCC shape 的面（face/shell）生成三角形/四边形网格
- **3D 体网格剖分**：对 OCC shape 的体（solid）生成四面体/六面体网格
- **网格数据管理**：高效存储和检索百万级 node / edge / element 数据
- **渲染数据生成**：将网格转换为 `Core::VisualData` 供 GPU 渲染

### 1.2 数据结构预留（本次不实现逻辑，仅预留字段）

- **场景拾取支持**：数据模型中保留 `EntityTag` 字段和 `GeoEntityRef` 几何关联信息，为后续实现 node / edge / element 拾取以及 by face / by solid 批量选择做准备

### 1.3 不在范围内

- 场景拾取逻辑实现（拾取交互、高亮、选择模式切换）— 数据结构已预留，留待后续迭代
- 网格编辑（节点移动、单元删除）— 留待后续迭代
- 网格文件导入/导出（.msh / .vtk）— 留待后续迭代
- 自适应网格细化 — 留待后续迭代
- 网格质量统计与可视化 — 留待后续迭代

---

## 2. 架构决策

### 2.1 Mesh 独立于 Geometry（方案 B）

MeshModule 拥有独立的 `MeshStore`，与 `ShapeStore` 解耦：

- MeshEntry 通过 `std::optional<uint32_t> sourceShapeId` 弱引用源 shape
- 删除 shape 不自动删除 mesh（由调用者决定策略）
- 未来可支持从外部文件导入纯网格（无关联 shape）
- 同一 shape 可生成多套网格

**模块间通信**：MeshModule 在 action 执行时通过 `PluginComponentFactory` 获取 GeometryModule 实例，再访问 `ShapeStore` 获取 OCC shape。

### 2.2 连续 ID 直接数组索引（方案 1）

- Node ID = 数组下标 + 1（1-based，与 Gmsh 一致）
- Element ID 连续分配，通过前缀和数组 O(1) 定位到 ElementBlock + 局部索引
- 对外部导入的稀疏 ID 做重编号

### 2.3 Gmsh OCC 内核直接导入

- 使用 `gmsh::model::occ::importShapesNativePointer()` 直接导入 `TopoDS_Shape`
- 无需中间文件序列化，性能最优
- Gmsh 生命周期通过 RAII `GmshSession` 管理

---

## 3. 模块架构

```
┌─ MeshModule (OpenGeoLab::Mesh) ──────────────────────────────────────┐
│                                                                       │
│  MeshModule : Core::ModuleBase                                        │
│  ├─ MeshStore              数据存储与信号                              │
│  ├─ GmshBridge             Gmsh C++ API 封装                          │
│  ├─ MeshVisualBuilder      网格 → VisualData 转换                     │
│  └─ Actions:                                                          │
│     ├─ GenerateSurfaceMeshAction   2D 面剖分                          │
│     ├─ GenerateVolumeMeshAction    3D 体剖分                          │
│     ├─ DeleteMeshAction            删除网格                            │
│     ├─ QueryMeshAction             查询网格信息                        │
│     └─ ListMeshesAction            列出所有网格                        │
│                                                                       │
│  依赖关系:                                                             │
│  ├─ 编译期: OpenGeoLab::Core, Gmsh                                   │
│  └─ 运行时: OpenGeoLab::Geometry (通过 factory 获取)                  │
└───────────────────────────────────────────────────────────────────────┘
```

### 3.1 目录结构

```
src/libs/mesh/
├── CMakeLists.txt
├── include/opengeolab/mesh/
│   ├── mesh_module.hpp
│   ├── mesh_store.hpp
│   ├── mesh_entry.hpp
│   ├── mesh_types.hpp              # ElementType, MeshNodeArray, ElementBlock
│   ├── mesh_params.hpp             # SurfaceMeshParams, VolumeMeshParams
│   ├── gmsh_bridge.hpp
│   ├── mesh_visual_builder.hpp
│   ├── generate_surface_mesh_action.hpp
│   ├── generate_volume_mesh_action.hpp
│   ├── delete_mesh_action.hpp
│   ├── query_mesh_action.hpp
│   └── list_meshes_action.hpp
├── src/
│   ├── mesh_module.cpp
│   ├── mesh_store.cpp
│   ├── mesh_entry.cpp
│   ├── gmsh_bridge.cpp
│   ├── mesh_visual_builder.cpp
│   ├── generate_surface_mesh_action.cpp
│   ├── generate_volume_mesh_action.cpp
│   ├── delete_mesh_action.cpp
│   ├── query_mesh_action.cpp
│   └── list_meshes_action.cpp
└── test/
    ├── mesh_store_test.cpp
    ├── mesh_entry_test.cpp
    └── gmsh_bridge_test.cpp
```

---

## 4. 核心数据模型

### 4.1 网格类型定义 (`mesh_types.hpp`)

```cpp
namespace OpenGeoLab::Mesh {

/// 网格单元类型，值与 Gmsh element type 对应
enum class ElementType : uint8_t {
    Line2 = 1,        ///< 2 节点线段
    Triangle3 = 2,    ///< 3 节点三角形
    Quad4 = 3,        ///< 4 节点四边形
    Tetra4 = 4,       ///< 4 节点四面体
    Hexa8 = 5,        ///< 8 节点六面体
    Prism6 = 6,       ///< 6 节点三棱柱
    Pyramid5 = 7,     ///< 5 节点四棱锥
    Line3 = 8,        ///< 3 节点二次线段
    Triangle6 = 9,    ///< 6 节点二次三角形
    Quad9 = 10,       ///< 9 节点二次四边形
    Tetra10 = 11,     ///< 10 节点二次四面体
    Hexa27 = 12,      ///< 27 节点二次六面体
    Prism18 = 13,     ///< 18 节点二次三棱柱
    Pyramid14 = 14,   ///< 14 节点二次四棱锥
};

/// 返回单元类型的节点数
constexpr uint32_t nodesPerElement(ElementType type);

/// 返回单元类型的拓扑维度 (1=线, 2=面, 3=体)
constexpr int elementDimension(ElementType type);

/// 节点紧凑数组 (double 精度)
struct MeshNodeArray {
    std::vector<double> coords;   ///< [x0,y0,z0, x1,y1,z1, ...] 3*N doubles
    size_t count() const { return coords.size() / 3; }
    /// 获取节点坐标 (nodeId 1-based)
    std::array<double, 3> position(uint32_t nodeId) const;
};

/// 来源几何实体关联
struct GeoEntityRef {
    int dimension;          ///< 几何维度 (0=vertex, 1=curve, 2=surface, 3=volume)
    int gmshTag;            ///< Gmsh geometric entity tag
    uint32_t sourceLocalId; ///< 对应 ShapeEntry 中 faceMap/solidMap 的 1-based 索引
};

/// 同类型单元块
struct ElementBlock {
    ElementType type;                      ///< 单元类型
    std::vector<uint32_t> connectivity;    ///< flat [n0,n1,n2, n3,n4,n5, ...] node IDs (1-based)
    GeoEntityRef geoEntity;                ///< 来源几何实体

    uint32_t nodesPerElem() const { return nodesPerElement(type); }
    size_t elementCount() const { return connectivity.size() / nodesPerElem(); }
};

} // namespace OpenGeoLab::Mesh
```

### 4.2 网格数据条目 (`mesh_entry.hpp`)

```cpp
namespace OpenGeoLab::Mesh {

/// Element ID 快速定位器 (前缀和 O(1) 查找)
class ElementLocator {
public:
    /// 从所有 element blocks 构建前缀和
    void build(const std::vector<ElementBlock>& lineBlocks,
               const std::vector<ElementBlock>& surfaceBlocks,
               const std::vector<ElementBlock>& volumeBlocks);

    struct Location {
        enum class Group { Line, Surface, Volume };
        Group group;
        size_t blockIndex;    ///< block 在所属 group 中的索引
        size_t localIndex;    ///< element 在 block 内的局部索引
    };

    /// O(1) 查找 (实际 O(log B)，B=block 数，通常很小 < 100)
    Location locate(uint32_t elementId) const;

    /// 总 element 数量
    uint32_t totalCount() const;

private:
    std::vector<uint32_t> m_prefixSums;  ///< 每个 block 的起始 element ID
    std::vector<Location::Group> m_groups;
    std::vector<size_t> m_blockIndices;
};

/// 完整网格数据条目
struct MeshEntry {
    uint32_t id{0};                            ///< Mesh ID (MeshStore 分配, 1-based)
    std::string name;                          ///< 用户可见名称
    std::optional<uint32_t> sourceShapeId;     ///< 来源 shape ID (可选)

    MeshNodeArray nodes;                       ///< 所有节点

    std::vector<ElementBlock> lineBlocks;      ///< 1D 线段单元
    std::vector<ElementBlock> surfaceBlocks;   ///< 2D 面单元 (三角/四边)
    std::vector<ElementBlock> volumeBlocks;    ///< 3D 体单元 (四面体/六面体等)

    ElementLocator elementLocator;             ///< 全局 element ID → block 定位

    // 渲染与拾取缓存
    std::shared_ptr<Core::VisualData> visualData;
    std::vector<Core::EntityTag> nodeTags;     ///< MeshNode tags
    std::vector<Core::EntityTag> edgeTags;     ///< MeshEdge tags
    std::vector<Core::EntityTag> elementTags;  ///< MeshElement tags

    /// 总节点数
    size_t nodeCount() const { return nodes.count(); }
    /// 总单元数
    uint32_t elementCount() const { return elementLocator.totalCount(); }
};

} // namespace OpenGeoLab::Mesh
```

### 4.3 网格数据存储 (`mesh_store.hpp`)

```cpp
namespace OpenGeoLab::Mesh {

class MeshStore {
public:
    /// 添加网格，返回分配的 mesh ID
    uint32_t add(MeshEntry entry);

    /// 删除网格
    void remove(uint32_t meshId);

    /// 查找网格 (返回 nullptr 如果不存在)
    const MeshEntry* find(uint32_t meshId) const;

    /// 所有 mesh ID 列表
    std::vector<uint32_t> allMeshIds() const;

    /// 查找关联指定 shape 的所有 mesh
    std::vector<uint32_t> findByShapeId(uint32_t shapeId) const;

    /// 生成/更新渲染数据
    void buildVisualData(uint32_t meshId);

    // 信号
    Kangaroo::Util::Signal<uint32_t, const MeshEntry&> meshAdded;
    Kangaroo::Util::Signal<uint32_t> meshRemoved;
    Kangaroo::Util::Signal<uint32_t, const MeshEntry&> meshUpdated;

private:
    mutable std::mutex m_mutex;
    std::vector<std::unique_ptr<MeshEntry>> m_entries;  ///< index = meshId - 1
    uint32_t m_nextId{1};
};

} // namespace OpenGeoLab::Mesh
```

---

## 5. Gmsh 桥接层

### 5.1 参数 (`mesh_params.hpp`)

```cpp
namespace OpenGeoLab::Mesh {

/// 2D 面网格参数
struct SurfaceMeshParams {
    double minSize{0.1};       ///< 最小单元尺寸
    double maxSize{10.0};      ///< 最大单元尺寸
    int algorithm{6};          ///< 算法: 1=MeshAdapt, 5=Delaunay, 6=Frontal-Delaunay, 7=BAMG
    bool quadDominant{false};  ///< true → recombine 为四边形
    int order{1};              ///< 单元阶次: 1=线性, 2=二次
    bool optimize{true};       ///< 是否优化

    static SurfaceMeshParams fromJson(const nlohmann::json& j);
    nlohmann::json toJson() const;
};

/// 3D 体网格参数
struct VolumeMeshParams {
    double minSize{0.1};
    double maxSize{10.0};
    int algorithm{1};           ///< 算法: 1=Delaunay, 4=Frontal, 10=HXT
    bool hexDominant{false};    ///< true → recombine 为六面体
    int order{1};
    bool optimize{true};
    int optimizeAlgorithm{0};   ///< 0=Gmsh default, 1=Netgen, 2=HighOrder

    static VolumeMeshParams fromJson(const nlohmann::json& j);
    nlohmann::json toJson() const;
};

} // namespace OpenGeoLab::Mesh
```

### 5.2 Gmsh 封装 (`gmsh_bridge.hpp`)

```cpp
namespace OpenGeoLab::Mesh {

/// RAII Gmsh 生命周期管理
class GmshSession {
public:
    GmshSession();   ///< gmsh::initialize()
    ~GmshSession();  ///< gmsh::finalize()
    GmshSession(const GmshSession&) = delete;
    GmshSession& operator=(const GmshSession&) = delete;
};

/// Gmsh 桥接 — 将 OCC shape 转为网格
namespace GmshBridge {

/// 对 shape 的面进行 2D 网格剖分
/// @param shape OCC shape (至少包含 face)
/// @param params 网格参数
/// @param progress 进度回调
/// @return 生成的 MeshEntry (未分配 ID，由 MeshStore::add 分配)
MeshEntry generateSurfaceMesh(const TopoDS_Shape& shape,
                               const SurfaceMeshParams& params,
                               const Core::ProgressCallback& progress);

/// 对 shape 的体进行 3D 网格剖分
/// @param shape OCC shape (至少包含 solid)
/// @param params 网格参数
/// @param progress 进度回调
/// @return 生成的 MeshEntry
MeshEntry generateVolumeMesh(const TopoDS_Shape& shape,
                              const VolumeMeshParams& params,
                              const Core::ProgressCallback& progress);

} // namespace GmshBridge
} // namespace OpenGeoLab::Mesh
```

### 5.3 Gmsh 工作流（伪代码）

```
generateSurfaceMesh(shape, params, progress):
    GmshSession session;
    gmsh::model::add("mesh_model");

    // 1. 导入 OCC shape
    std::vector<std::pair<int,int>> outDimTags;
    gmsh::model::occ::importShapesNativePointer(&shape, outDimTags);
    gmsh::model::occ::synchronize();

    // 2. 设置网格参数
    gmsh::option::setNumber("Mesh.MeshSizeMin", params.minSize);
    gmsh::option::setNumber("Mesh.MeshSizeMax", params.maxSize);
    gmsh::option::setNumber("Mesh.Algorithm", params.algorithm);
    if (params.quadDominant)
        gmsh::option::setNumber("Mesh.RecombineAll", 1);
    gmsh::option::setNumber("Mesh.ElementOrder", params.order);

    progress(0.1, "Parameters configured");  // ProgressCallback(double, string) → bool

    // 3. 生成 2D 网格
    gmsh::model::mesh::generate(2);
    if (!progress(0.7, "Mesh generated"))  // 返回 false → 用户请求取消
        throw std::runtime_error("Mesh generation cancelled");

    // 4. 优化
    if (params.optimize)
        gmsh::model::mesh::optimize("", true);
    progress(0.8, "Optimization complete");

    // 5. 提取数据
    MeshEntry entry;
    extractNodes(entry);     // gmsh::model::mesh::getNodes()
    extractElements(entry);  // gmsh::model::mesh::getElements()
    entry.elementLocator.build(entry.lineBlocks, entry.surfaceBlocks, entry.volumeBlocks);
    progress(1.0, "Data extraction complete");

    return entry;
```

---

## 6. 渲染数据生成

### 6.1 MeshVisualBuilder (`mesh_visual_builder.hpp`)

```cpp
namespace OpenGeoLab::Mesh::MeshVisualBuilder {

/// 从 MeshEntry 构建渲染数据
Core::VisualData buildVisualData(const MeshEntry& entry);

/// 构建拾取用 EntityTag
struct MeshTags {
    std::vector<Core::EntityTag> nodeTags;     ///< per node
    std::vector<Core::EntityTag> edgeTags;     ///< per wireframe line segment
    std::vector<Core::EntityTag> elementTags;  ///< per rendered triangle
};
MeshTags buildEntityTags(const MeshEntry& entry);

} // namespace OpenGeoLab::Mesh::MeshVisualBuilder
```

### 6.2 渲染映射规则

| 网格数据 | → VisualData 组件 | 说明 |
|---------|------------------|------|
| 2D 面单元 (tri/quad) | `SurfaceMesh` | 直接渲染，计算面法线 |
| 3D 体网格外表面 | `SurfaceMesh` | 提取边界面（仅外表面） |
| 单元边线 | `EdgeMesh` | wireframe 显示 |
| 网格节点 | `PointSet` | 节点点云 |

### 6.3 3D 体网格外表面提取

对 3D 体网格，只渲染外边界面，避免渲染百万内部单元：

```
1. 遍历所有体单元，提取每个面 (tet → 4 tri, hex → 6 quad)
2. 对每个面，按节点 ID 排序生成 canonical key
3. 出现 1 次的面 = 边界面，出现 2 次的 = 内部面
4. 只保留边界面生成 SurfaceMesh
5. 边界面的线框 → EdgeMesh
```

---

## 7. 拾取与选择（数据结构预留，本次不实现交互逻辑）

> **说明**：以下数据结构和字段在本次实现中预留，但拾取交互逻辑（模式切换、高亮、选择操作）不在本次范围内。

### 7.1 预留数据结构

**EntityTag 字段**：`MeshEntry` 中保留 `nodeTags` / `edgeTags` / `elementTags` 向量，由 `MeshVisualBuilder` 在生成渲染数据时一并构建，供后续拾取使用。

**GeoEntityRef**：每个 `ElementBlock` 关联 `GeoEntityRef`，记录来源几何面/体索引，供后续 "sel by face / by solid" 使用。

**拾取地址约定**：统一使用 `(meshId, EntityTag)` 格式：
- `EntityTag{MeshNode, nodeId}` — 节点
- `EntityTag{MeshEdge, edgeElementId}` — 边
- `EntityTag{MeshElement, elementId}` — 面/体单元

### 7.2 后续迭代计划（仅记录，不实现）

拾取模式枚举和交互流程在后续迭代中实现：

```cpp
// 未来实现
enum class MeshPickMode : uint8_t {
    Node,       ///< 拾取单个网格节点
    Edge,       ///< 拾取单条网格边
    Element,    ///< 拾取单个网格单元
    ByFace,     ///< 选中同一几何面上的所有单元
    BySolid     ///< 选中同一几何体上的所有单元
};
```

**Sel by Face 流程**（后续实现）：
```
1. 用户拾取一个 MeshElement → 获得 elementId
2. elementLocator.locate(elementId) → 找到所属 ElementBlock
3. 读取 block.geoEntity → {dimension=2, sourceLocalId=faceIdx}
4. 遍历所有 surfaceBlocks，收集 geoEntity.sourceLocalId == faceIdx 的 blocks
5. 返回这些 blocks 中的全部 element ID 范围 → 高亮
```

### 7.3 Gmsh entity tag → ShapeEntry localId 映射

Gmsh 在 `occ::importShapesNativePointer` 后会给每个几何实体分配 tag。
通过 `gmsh::model::getEntities(dim)` 获取实体列表，其顺序与 OCC 的
`TopTools_IndexedMapOfShape` 索引对应（Gmsh OCC 内核保持顺序）。
在提取单元时，记录每个 element 所属的 `(dim, gmshTag)` → 转换为 `sourceLocalId`。

---

## 8. Geometry 模块接口使用

MeshModule 需要从 GeometryModule 获取 OCC shape。`ShapeStore::find(shapeId)` 已返回
`const ShapeEntry*`，可直接访问 `.shape` 和 `.name`，无需新增额外接口。

MeshModule 的 action 在执行时通过 factory 获取 GeometryModule：
```cpp
auto geoModule = factory.getSharedInstance<Core::ModuleBase>("geometry");
auto& geoMod = static_cast<Geometry::GeometryModule&>(*geoModule);
const auto* entry = geoMod.shapeStore().find(shapeId);
// entry->shape  → TopoDS_Shape
// entry->name   → std::string
```

---

## 9. CMake 集成

### 9.1 Gmsh 路径配置 (CMakeUserPresets.json)

```json
{
    "name": "local-debug",
    "cacheVariables": {
        "OpenCASCADE_DIR": "D:/WorkSpace/OpenSource/OCCT7.9.2/occt-debug-install/cmake",
        "gmsh_DIR": "D:/WorkSpace/OpenSource/GMesh/gmsh_4_15_0-debug/lib/cmake/gmsh"
    }
},
{
    "name": "local-relwithdebinfo",
    "cacheVariables": {
        "OpenCASCADE_DIR": "D:/WorkSpace/OpenSource/OCCT7.9.2/occt-install/cmake",
        "gmsh_DIR": "D:/WorkSpace/OpenSource/GMesh/gmsh_4_15_0-relwithdebinfo/lib/cmake/gmsh"
    }
}
```

### 9.2 Mesh 模块 CMakeLists.txt

```cmake
find_package(gmsh REQUIRED)

opengeolab_add_module(
    opengeolab_mesh
    ALIAS_NAME Mesh
    SOURCES ${mesh_sources}
    PUBLIC_HEADERS ${mesh_public_headers}
    PUBLIC_LINKS
        OpenGeoLab::Core
        gmsh                   # Gmsh C++ API
    PRIVATE_LINKS
        OpenGeoLab::Geometry   # 仅 action .cpp 中运行时获取 shape 数据
)
```

### 9.3 顶层 CMakeLists.txt

在现有 `add_subdirectory` 序列中添加：

```cmake
add_subdirectory(src/libs/core)
add_subdirectory(src/libs/io)
add_subdirectory(src/libs/geometry)
add_subdirectory(src/libs/mesh)       # ← 新增，在 geometry 之后
add_subdirectory(src/libs/command)
add_subdirectory(src/libs/python)
add_subdirectory(src/app)
```

### 9.4 Module Registry

在 `module_registry.cpp` 中注册 MeshModule：

```cpp
if (!is_registered(Mesh::MeshModule::MODULE_NAME)) {
    factory.bindSingleton<Core::ModuleBase, Mesh::MeshModule>(
        Mesh::MeshModule::MODULE_NAME, std::ref(factory));
}
```

---

## 10. UI 界面设计

### 10.1 Ribbon Tab 调整

Mesh Tab 的 ribbon 从 `generateMesh / smoothMesh` 改为：

| Group | Action Key | Title | Icon | 说明 |
|-------|-----------|-------|------|------|
| Generate | `meshSurface` | 2D | `meshSurface` | 2D 面网格剖分 |
| Generate | `meshVolume` | 3D | `meshVolume` | 3D 体网格剖分 |
| Inspect | `queryMesh` | Query | `query` | 查询网格信息 |

> **注意**：现有 `generateMesh` / `smoothMesh` 按钮移除。`smoothMesh` 留待后续迭代。

### 10.2 MeshSurfacePage.qml (2D 面剖分)

继承 `FunctionPageBase`，字段：

| 字段 | 组件 | 说明 |
|------|------|------|
| Name | ParamField | 网格名称（自动生成） |
| Target Shape | ShapeSelector (新组件) | 下拉选择 shape |
| Min Size | DimensionInput | 最小单元尺寸 |
| Max Size | DimensionInput | 最大单元尺寸 |
| Algorithm | ComboBox | MeshAdapt / Delaunay / Frontal-Delaunay / BAMG |
| Element Type | ComboBox | Triangle / Quad / Mixed |
| Order | ComboBox | 1st / 2nd |
| Optimize | CheckBox | 是否优化 |

**getParameters() 输出**：
```json
{
    "module": "mesh",
    "action": "generate_surface_mesh",
    "param": {
        "shapeId": 1,
        "name": "SurfaceMesh_1",
        "minSize": 0.1,
        "maxSize": 10.0,
        "algorithm": 6,
        "quadDominant": false,
        "order": 1,
        "optimize": true
    }
}
```

### 10.3 MeshVolumePage.qml (3D 体剖分)

与 MeshSurfacePage 类似，额外字段：

| 字段 | 组件 | 说明 |
|------|------|------|
| Optimize Algorithm | ComboBox | Default / Netgen / HighOrder |

**getParameters() 输出**：
```json
{
    "module": "mesh",
    "action": "generate_volume_mesh",
    "param": {
        "shapeId": 1,
        "name": "VolumeMesh_1",
        "minSize": 0.1,
        "maxSize": 10.0,
        "algorithm": 1,
        "hexDominant": false,
        "order": 1,
        "optimize": true,
        "optimizeAlgorithm": 0
    }
}
```

### 10.4 新增组件

- **ShapeSelector.qml** — 下拉列表组件，显示 ShapeStore 中的所有 shape，选中后传出 shapeId
- **meshSurface.svg** — 2D 面网格图标（网格线覆盖平面的图标）
- **meshVolume.svg** — 3D 体网格图标（网格线覆盖立方体的图标）

### 10.5 MainPages.qml 注册

```javascript
"meshSurface": { path: "components/pages/MeshSurfacePage.qml" },
"meshVolume":  { path: "components/pages/MeshVolumePage.qml" }
```

---

## 11. JSON 协议

### 11.1 generate_surface_mesh

**Request**：
```json
{
    "module": "mesh",
    "action": "generate_surface_mesh",
    "param": {
        "shapeId": 1,
        "name": "SurfaceMesh_1",
        "minSize": 0.1,
        "maxSize": 10.0,
        "algorithm": 6,
        "quadDominant": false,
        "order": 1,
        "optimize": true
    }
}
```

**Response (success)**：
```json
{
    "ok": true,
    "meshId": 1,
    "name": "SurfaceMesh_1",
    "nodeCount": 12345,
    "elementCount": 24680,
    "elementTypes": ["Triangle3"]
}
```

### 11.2 generate_volume_mesh

类似 surface，额外 `optimizeAlgorithm` 参数。

### 11.3 delete_mesh

**Request**：
```json
{ "module": "mesh", "action": "delete_mesh", "param": { "meshId": 1 } }
```

**Response**：
```json
{
    "ok": true,
    "action": "delete_mesh",
    "meshId": 1
}
```

### 11.4 query_mesh

```json
{ "module": "mesh", "action": "query_mesh", "param": { "meshId": 1 } }
```

**Response**：
```json
{
    "ok": true,
    "meshId": 1,
    "name": "SurfaceMesh_1",
    "sourceShapeId": 1,
    "nodeCount": 12345,
    "elementSummary": {
        "Triangle3": 24000,
        "Quad4": 680
    },
    "boundingBox": { "min": [0,0,0], "max": [10,10,10] }
}
```

### 11.5 list_meshes

**Request**：
```json
{ "module": "mesh", "action": "list_meshes", "param": {} }
```

**Response**：
```json
{
    "ok": true,
    "action": "list_meshes",
    "count": 2,
    "meshes": [
        { "meshId": 1, "name": "SurfaceMesh_1", "sourceShapeId": 1, "nodeCount": 12345, "elementCount": 24680 },
        { "meshId": 2, "name": "VolumeMesh_1", "sourceShapeId": 1, "nodeCount": 50000, "elementCount": 200000 }
    ]
}
```

---

## 12. 测试策略

| 测试 | 范围 | 关键验证点 |
|------|------|-----------|
| `mesh_entry_test` | MeshNodeArray, ElementBlock, ElementLocator | O(1) 查找正确性、边界情况 |
| `mesh_store_test` | MeshStore add/remove/find/findByShapeId | 线程安全、信号触发 |
| `gmsh_bridge_test` | GmshBridge::generateSurfaceMesh/VolumeMesh | 对简单 box shape 生成网格并验证 node/element 数量 |
| `mesh_visual_builder_test` | buildVisualData | VisualData 非空、索引有效 |

---

## 13. 风险与约束

1. **Gmsh 线程安全**：Gmsh 不是线程安全的。并发生成网格时需要串行化 Gmsh 调用（加互斥锁）。
2. **Gmsh OCC 版本兼容**：Gmsh 4.15 内置 OCC 内核，需确认与项目使用的 OCCT 7.9.2 兼容。如果有冲突，需要改用中间文件（BRep export → Gmsh import）作为降级方案。
3. **Gmsh entity tag 映射**：§7.4 假设 Gmsh OCC 内核保持几何实体顺序与 OCC `TopTools_IndexedMapOfShape` 一致。此为关键前提，实现时需显式验证（对比 Gmsh entity 与 OCC face/solid 的几何中心坐标）。如不一致，需改用几何中心匹配建立映射。
4. **大数据渲染性能**：百万级单元生成 VisualData 时需注意内存分配策略，建议预分配 vector capacity。
5. **Gmsh DLL 部署**：Windows 上需要将 gmsh.dll 部署到运行目录或 PATH 中。

---

## 14. 依赖总结

| 依赖 | 类型 | 版本 | 用途 |
|------|------|------|------|
| OpenGeoLab::Core | 编译期 | - | ModuleBase, IAction, VisualData, EntityTag |
| OpenGeoLab::Geometry | 编译期+运行时 | - | ShapeStore, ShapeEntry (获取 OCC shape) |
| Gmsh | 编译期+运行时 | 4.15.0 | C++ API, OCC kernel |
| nlohmann_json | 编译期 | 3.12.0 | JSON 序列化 (已由 Core 传递) |
| Kangaroo::Util | 编译期 | 2.3.1 | Signal, Factory (已由 Core 传递) |
