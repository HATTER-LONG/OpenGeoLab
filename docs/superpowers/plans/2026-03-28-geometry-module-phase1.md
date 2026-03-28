# Geometry Module Phase 1 实现计划

> **执行要求：** 按任务逐项执行，优先使用 superpowers:subagent-driven-development；如果不使用子代理流程，则使用 superpowers:executing-plans。步骤使用复选框语法记录进度。

**目标：** 基于 OCC 实现 geometry 模块的几何存储、文件导入、参数化创建、离散化和拾取标注能力。

**架构：** ShapeStore 集中持有 OCC 几何并通过 Kangaroo::Util::Signal 广播变更；所有 Action 通过 registerAction 注入 ShapeStore 引用；Tessellator 将 BRep 几何转为 VisualData + EntityTag。

**技术栈：** C++20, CMake/Ninja, OpenCASCADE 7.9.2, Kangaroo PluginComponentFactory + Signal, doctest, nlohmann::json, spdlog/fmt

**规格文档：** `docs/superpowers/specs/2026-03-28-geometry-module-design.md`

---

## 文件概览

### 新增文件

| 文件 | 职责 |
|------|------|
| `src/libs/geometry/include/opengeolab/geometry/shape_entry.hpp` | ShapeEntry 数据结构 |
| `src/libs/geometry/include/opengeolab/geometry/shape_store.hpp` | ShapeStore 几何存储 |
| `src/libs/geometry/include/opengeolab/geometry/tessellator.hpp` | 离散化引擎 |
| `src/libs/geometry/include/opengeolab/geometry/import_brep_action.hpp` | import_brep action |
| `src/libs/geometry/include/opengeolab/geometry/import_step_action.hpp` | import_step action |
| `src/libs/geometry/include/opengeolab/geometry/create_cylinder_action.hpp` | create_cylinder action |
| `src/libs/geometry/include/opengeolab/geometry/create_sphere_action.hpp` | create_sphere action |
| `src/libs/geometry/include/opengeolab/geometry/create_torus_action.hpp` | create_torus action |
| `src/libs/geometry/include/opengeolab/geometry/tessellate_action.hpp` | tessellate action |
| `src/libs/geometry/include/opengeolab/geometry/query_shape_action.hpp` | query_shape action |
| `src/libs/geometry/include/opengeolab/geometry/list_shapes_action.hpp` | list_shapes action |
| `src/libs/geometry/include/opengeolab/geometry/delete_shape_action.hpp` | delete_shape action |
| `src/libs/geometry/src/shape_store.cpp` | ShapeStore 实现 |
| `src/libs/geometry/src/tessellator.cpp` | Tessellator 实现 |
| `src/libs/geometry/src/import_brep_action.cpp` | import_brep 实现 |
| `src/libs/geometry/src/import_step_action.cpp` | import_step 实现 |
| `src/libs/geometry/src/create_cylinder_action.cpp` | create_cylinder 实现 |
| `src/libs/geometry/src/create_sphere_action.cpp` | create_sphere 实现 |
| `src/libs/geometry/src/create_torus_action.cpp` | create_torus 实现 |
| `src/libs/geometry/src/tessellate_action.cpp` | tessellate 实现 |
| `src/libs/geometry/src/query_shape_action.cpp` | query_shape 实现 |
| `src/libs/geometry/src/list_shapes_action.cpp` | list_shapes 实现 |
| `src/libs/geometry/src/delete_shape_action.cpp` | delete_shape 实现 |
| `src/libs/geometry/test/shape_store_test.cpp` | ShapeStore 单元测试 |
| `src/libs/geometry/test/tessellator_test.cpp` | Tessellator 单元测试 |
| `src/libs/geometry/test/create_actions_test.cpp` | 创建 action 测试 |
| `src/libs/geometry/test/import_actions_test.cpp` | 导入 action 测试 |
| `src/libs/geometry/test/data/box.brep` | 测试用 BRep 文件 |
| `src/libs/geometry/test/data/simple.stp` | 测试用 STEP 文件 |

### 修改文件

| 文件 | 变更 |
|------|------|
| `src/libs/core/include/opengeolab/core/module.hpp` | registerAction 模板支持变参转发 |
| `src/libs/geometry/include/opengeolab/geometry/geometry_module.hpp` | 持有 ShapeStore，暴露 shapeStore() |
| `src/libs/geometry/include/opengeolab/geometry/create_box_action.hpp` | 构造函数接受 ShapeStore& |
| `src/libs/geometry/src/geometry_module.cpp` | 注册所有 action 并传入 ShapeStore |
| `src/libs/geometry/src/create_box_action.cpp` | 替换 mock 为真正的 OCC 实现 |
| `src/libs/geometry/CMakeLists.txt` | 添加新文件、链接 OCC 库 |
| `src/libs/geometry/test/geometry_module_test.cpp` | 更新为新的返回格式和行为 |

---

## 任务列表

### 任务 0：基线构建验证

**目的：** 确认当前代码能编译通过、测试全绿，作为后续变更的基线。

- [ ] 步骤 1：运行 `cmake --build build --config Debug --parallel 4`
- [ ] 步骤 2：运行 `ctest --test-dir build -C Debug --output-on-failure`
- [ ] 步骤 3：确认无编译错误、所有测试通过

**验证命令：**
```bash
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

**预期结果：** 全部编译通过，全部测试通过。

---

### 任务 1：扩展 registerAction 模板支持变参转发

**目的：** 让 Action 构造函数可以接收额外参数（如 ShapeStore&），是后续所有 Action 注入的基础。

**文件：**
- 修改：`src/libs/core/include/opengeolab/core/module.hpp`
- 测试：使用任务 0 的基线测试回归

**背景：** Kangaroo 的 `bindSingleton` 已支持 `Args&&...` 变参转发（见 `plugin_component_factory.hpp` 第 336-340 行）。当前 `registerAction<ActionT>()` 模板无参，需要扩展。

- [ ] 步骤 1：修改 `module.hpp` 中的 `registerAction` 模板，将：

```cpp
template <class ActionT> void ModuleBase::registerAction() {
    std::string key = m_moduleName + "." + std::string(ActionT::ACTION_NAME);
    m_factory.bindSingleton<IAction, ActionT>(key);
}
```

改为：

```cpp
template <class ActionT, class... Args> void ModuleBase::registerAction(Args&&... args) {
    std::string key = m_moduleName + "." + std::string(ActionT::ACTION_NAME);
    m_factory.bindSingleton<IAction, ActionT>(key, std::forward<Args>(args)...);
}
```

同时更新类内声明（protected 区域）为：

```cpp
template <class ActionT, class... Args> void registerAction(Args&&... args);
```

- [ ] 步骤 2：构建并运行全量测试确认回归无破坏

**验证命令：**
```bash
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

**预期结果：** 编译通过，现有测试全绿（现有 action 仍无参构造，兼容变参模板的零参特化）。

---

### 任务 2：CMake — geometry 链接 OCC 库

**目的：** 更新 geometry 的 CMakeLists.txt 以链接 OpenCASCADE 组件。此任务不添加新的源文件，只修改构建配置。

**文件：**
- 修改：`src/libs/geometry/CMakeLists.txt`

**背景：** 顶层 CMakeLists.txt 第 178 行已有 `find_package(OpenCASCADE REQUIRED)`。geometry 模块需链接具体的 OCC 库。

- [ ] 步骤 1：在 `src/libs/geometry/CMakeLists.txt` 的 `opengeolab_add_module` 调用中添加 OCC 链接。当前的 `PUBLIC_LINKS` 只有 `OpenGeoLab::Core`，需要增加 OCC 目标：

```cmake
opengeolab_add_module(
    opengeolab_geometry
    ALIAS_NAME
    Geometry
    SOURCES
    ${geometry_sources}
    PUBLIC_HEADERS
    ${geometry_public_headers}
    PUBLIC_LINKS
    OpenGeoLab::Core
    TKernel
    TKMath
    TKG3d
    TKGeomBase
    TKBRep
    TKTopAlgo
    TKPrim
    TKMesh
    TKDESTEP
    TKDE
    TKXSBase
    TKShHealing)
```

注意：OCC 放 PUBLIC_LINKS，因为 `shape_entry.hpp` 是公共头文件且包含 `<TopoDS_Shape.hxx>` 等 OCC 头文件。下游 target 如果 include 了 shape_entry.hpp（例如通过 ShapeStore 信号的 `const ShapeEntry&` 参数），需要 OCC include 路径可见。这与规格 §11.1 一致。OCC 7.9.2 使用 `TKDESTEP` + `TKDE` 替代旧版的 `TKSTEP` + `TKSTEPBase`。

- [ ] 步骤 2：构建确认 CMake 配置通过、链接无误

**验证命令：**
```bash
cmake --build build --target opengeolab_geometry --config Debug --parallel 4
```

**预期结果：** 编译通过（源文件不变，仅链接关系变更）。

---

### 任务 3：core 扩展 — VisualData + EntityTag

**目的：** 在 core 模块中创建或更新 VisualData 和 EntityTag 类型，供 geometry、scene、render 共享。

**文件：**
- 新增（如果不存在）：`src/libs/core/include/opengeolab/core/visual_data.hpp`
- 新增（如果不存在）：`src/libs/core/include/opengeolab/core/entity_tag.hpp`
- 修改：`src/libs/core/CMakeLists.txt`（添加到 `core_public_headers`）

**前置条件：** 检查 core 模块中是否已有这些文件。如果已有，则按规格 §5 和 §6 扩展。

- [ ] 步骤 1：检查文件是否存在。如不存在，创建 `entity_tag.hpp`：

```cpp
#pragma once
#include <cstdint>

namespace OpenGeoLab::Core {

enum class EntityType : uint8_t {
    GeoVertex  = 0,
    GeoEdge    = 1,
    GeoWire    = 2,
    GeoFace    = 3,
    GeoSolid   = 4,

    MeshNode    = 10,
    MeshEdge    = 11,
    MeshElement = 12,

    SceneNode   = 20
};

struct EntityTag {
    EntityType type;
    uint32_t localId;
};

} // namespace OpenGeoLab::Core
```

- [ ] 步骤 2：创建 `visual_data.hpp`（需要 GLM 依赖，core 已链接 GLM 否？如果 core 不链接 GLM，此文件可能需要用 float[4] 代替 glm::vec4，或在 core 的 CMakeLists.txt 中添加 GLM 链接）：

```cpp
#pragma once
#include <cstdint>
#include <vector>

namespace OpenGeoLab::Core {

struct SurfaceMesh {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<uint32_t> indices;
    std::vector<float> colors;
    float defaultColor[4]{0.7f, 0.7f, 0.7f, 1.0f};
};

struct EdgeMesh {
    std::vector<float> positions;
    std::vector<uint32_t> indices;
    float color[4]{0.0f, 0.0f, 0.0f, 1.0f};
};

struct PointSet {
    std::vector<float> positions;
    float pointSize{5.0f};
    float color[4]{1.0f, 0.0f, 0.0f, 1.0f};
};

enum class RenderStyle : uint8_t {
    Solid,
    Wireframe,
    SolidWithEdges,
    Transparent
};

struct VisualData {
    std::vector<SurfaceMesh> surfaces;
    std::vector<EdgeMesh> edges;
    std::vector<PointSet> points;
    RenderStyle style{RenderStyle::SolidWithEdges};
};

} // namespace OpenGeoLab::Core
```

注意：颜色使用 `float[4]` 而非 `glm::vec4` 以避免 core 链接 GLM。render 模块可以在使用时转换。如果 core 已链接 GLM，则改用 `glm::vec4`。

- [ ] 步骤 3：更新 core 的 CMakeLists.txt 添加新头文件到 `core_public_headers`
- [ ] 步骤 4：构建并运行 core 测试

**验证命令：**
```bash
cmake --build build --target opengeolab_core --config Debug --parallel 4
ctest --test-dir build -C Debug -R core --output-on-failure
```

**预期结果：** 编译通过，core 测试全绿。

---

### 任务 4：ShapeEntry 数据结构

**目的：** 定义每个顶层几何对象的完整索引与缓存结构。

**前置条件：** 任务 3 已完成（core/entity_tag.hpp 和 core/visual_data.hpp 已存在）。

**文件：**
- 新增：`src/libs/geometry/include/opengeolab/geometry/shape_entry.hpp`
- 修改：`src/libs/geometry/CMakeLists.txt`（添加到 `geometry_public_headers`）

- [ ] 步骤 1：创建 `shape_entry.hpp`，内容参照规格 §3.1：

```cpp
#pragma once

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/core/visual_data.hpp>

#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS_Shape.hxx>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace OpenGeoLab::Geometry {

struct ShapeEntry {
    uint32_t id{0};
    std::string name;
    TopoDS_Shape shape;

    TopTools_IndexedMapOfShape vertexMap;
    TopTools_IndexedMapOfShape edgeMap;
    TopTools_IndexedMapOfShape wireMap;
    TopTools_IndexedMapOfShape faceMap;
    TopTools_IndexedMapOfShape solidMap;

    std::shared_ptr<Core::VisualData> visualData;
    std::vector<Core::EntityTag> triangleTags;
    std::vector<Core::EntityTag> edgeTags;
    std::vector<Core::EntityTag> vertexTags;
};

} // namespace OpenGeoLab::Geometry
```

- [ ] 步骤 2：将 `shape_entry.hpp` 添加到 `CMakeLists.txt` 的 `geometry_public_headers` 列表
- [ ] 步骤 3：构建确认头文件无语法错误

**验证命令：**
```bash
cmake --build build --target opengeolab_geometry --config Debug --parallel 4
```

**预期结果：** 编译通过。

---

### 任务 5：ShapeStore 核心实现

**目的：** 实现几何存储的增删改查和信号广播。这是整个 geometry 模块的基础组件。

**文件：**
- 新增：`src/libs/geometry/include/opengeolab/geometry/shape_store.hpp`
- 新增：`src/libs/geometry/src/shape_store.cpp`
- 新增：`src/libs/geometry/test/shape_store_test.cpp`
- 修改：`src/libs/geometry/CMakeLists.txt`

- [ ] 步骤 1：创建 `shape_store.hpp` 参照规格 §3.2。关键接口：

```cpp
class ShapeStore {
public:
    ShapeStore();
    ~ShapeStore();

    uint32_t add(const std::string& name, const TopoDS_Shape& shape);
    void remove(uint32_t shape_id);
    void update(uint32_t shape_id, const TopoDS_Shape& new_shape);
    void tessellate(uint32_t shape_id,
                    double linear_deflection = 0.1,
                    double angular_deflection = 0.5);

    [[nodiscard]] const ShapeEntry* find(uint32_t shape_id) const;
    [[nodiscard]] std::vector<uint32_t> allShapeIds() const;
    [[nodiscard]] std::size_t size() const;

    [[nodiscard]] TopoDS_Shape getSubShape(uint32_t shape_id,
                                           Core::EntityType type,
                                           uint32_t local_id) const;

    Kangaroo::Util::Signal<uint32_t, const ShapeEntry&> shapeAdded;
    Kangaroo::Util::Signal<uint32_t> shapeRemoved;
    Kangaroo::Util::Signal<uint32_t, const ShapeEntry&> shapeUpdated;

private:
    void buildSubShapeIndex(ShapeEntry& entry);

    mutable std::mutex m_mutex;
    std::vector<std::unique_ptr<ShapeEntry>> m_slots;
    std::vector<uint32_t> m_freeList;
    uint32_t m_nextId{0};
};
```

- [ ] 步骤 2：创建 `shape_store.cpp`，实现 add/remove/find/allShapeIds/size/getSubShape/buildSubShapeIndex。tessellate 暂时 throw（任务 6 实现）。

`buildSubShapeIndex` 实现要点：
```cpp
void ShapeStore::buildSubShapeIndex(ShapeEntry& entry) {
    TopExp::MapShapes(entry.shape, TopAbs_VERTEX, entry.vertexMap);
    TopExp::MapShapes(entry.shape, TopAbs_EDGE, entry.edgeMap);
    TopExp::MapShapes(entry.shape, TopAbs_WIRE, entry.wireMap);
    TopExp::MapShapes(entry.shape, TopAbs_FACE, entry.faceMap);
    TopExp::MapShapes(entry.shape, TopAbs_SOLID, entry.solidMap);
}
```

`add` 实现要点：分配 ID → 创建 ShapeEntry → buildSubShapeIndex → 存入 m_slots → 解锁后 emit shapeAdded。

`getSubShape` 实现要点：
```cpp
switch (type) {
    case Core::EntityType::GeoVertex: return entry->vertexMap.FindKey(local_id);
    case Core::EntityType::GeoEdge:   return entry->edgeMap.FindKey(local_id);
    case Core::EntityType::GeoWire:   return entry->wireMap.FindKey(local_id);
    case Core::EntityType::GeoFace:   return entry->faceMap.FindKey(local_id);
    case Core::EntityType::GeoSolid:  return entry->solidMap.FindKey(local_id);
    default: return {};
}
```

- [ ] 步骤 3：创建 `shape_store_test.cpp`，测试用例：
  - `add` 返回递增 ID 且 `find` 可查
  - `remove` 后 `find` 返回 nullptr
  - `remove` 后 ID 被 freeList 复用
  - `allShapeIds` 返回正确列表
  - `size` 返回正确数量
  - `add` 自动构建子形状索引（用 BRepPrimAPI_MakeBox 创建测试 shape，验证 faceMap.Extent() == 6）
  - `getSubShape` 对 GeoFace 返回非空 shape
  - 信号触发测试：add 触发 shapeAdded、remove 触发 shapeRemoved

- [ ] 步骤 4：更新 CMakeLists.txt 添加源文件和测试

```cmake
# 在 geometry_public_headers 中添加
include/opengeolab/geometry/shape_store.hpp

# 在 geometry_sources 中添加
src/shape_store.cpp

# 添加新测试 target
opengeolab_add_doctest_test(
    opengeolab_shape_store_test SOURCES test/shape_store_test.cpp
    LINKS OpenGeoLab::Geometry)
```

- [ ] 步骤 5：构建并运行测试

**验证命令：**
```bash
cmake --build build --target opengeolab_geometry --config Debug --parallel 4
cmake --build build --target opengeolab_shape_store_test --config Debug --parallel 4
ctest --test-dir build -C Debug -R shape_store --output-on-failure
```

**预期结果：** 编译通过，shape_store 测试全绿。

---

### 任务 6：Tessellator 实现

**目的：** 将 OCC BRep 几何转为 VisualData（面/边/点）+ EntityTag。

**文件：**
- 新增：`src/libs/geometry/include/opengeolab/geometry/tessellator.hpp`
- 新增：`src/libs/geometry/src/tessellator.cpp`
- 新增：`src/libs/geometry/test/tessellator_test.cpp`
- 修改：`src/libs/geometry/CMakeLists.txt`

- [ ] 步骤 1：创建 `tessellator.hpp`，参照规格 §4.2：

```cpp
namespace OpenGeoLab::Geometry {

struct TessellationResult {
    Core::VisualData visualData;
    std::vector<Core::EntityTag> triangleTags;
    std::vector<Core::EntityTag> edgeTags;
    std::vector<Core::EntityTag> vertexTags;
};

[[nodiscard]] TessellationResult tessellate(const ShapeEntry& entry,
                                            double linear_deflection,
                                            double angular_deflection);

} // namespace OpenGeoLab::Geometry
```

- [ ] 步骤 2：创建 `tessellator.cpp`，实现三阶段提取（参照规格 §4.3）：

**面提取关键代码：**
```cpp
BRepMesh_IncrementalMesh mesher(entry.shape, linear_deflection,
                                Standard_False, angular_deflection);
for (int i = 1; i <= entry.faceMap.Extent(); ++i) {
    auto face = TopoDS::Face(entry.faceMap.FindKey(i));
    TopLoc_Location loc;
    auto tri = BRep_Tool::Triangulation(face, loc);
    if (tri.IsNull()) continue;

    SurfaceMesh surface;
    // 提取 tri->NbNodes() 个顶点的坐标和法线
    // 提取 tri->NbTriangles() 个三角形索引
    // 处理 face orientation 翻转法线
    // 应用 loc 变换到坐标
    result.visualData.surfaces.push_back(std::move(surface));

    // 为每个三角形添加 EntityTag{GeoFace, i}
    for (int t = 0; t < tri->NbTriangles(); ++t) {
        result.triangleTags.push_back({Core::EntityType::GeoFace, static_cast<uint32_t>(i)});
    }
}
```

**边提取关键代码：**
```cpp
for (int j = 1; j <= entry.edgeMap.Extent(); ++j) {
    auto edge = TopoDS::Edge(entry.edgeMap.FindKey(j));
    BRepAdaptor_Curve curve(edge);
    double first = curve.FirstParameter();
    double last = curve.LastParameter();

    EdgeMesh edgeMesh;
    constexpr int num_samples = 50; // 或根据弧长自适应
    for (int s = 0; s <= num_samples; ++s) {
        double u = first + (last - first) * s / num_samples;
        gp_Pnt p = curve.Value(u);
        edgeMesh.positions.push_back(static_cast<float>(p.X()));
        edgeMesh.positions.push_back(static_cast<float>(p.Y()));
        edgeMesh.positions.push_back(static_cast<float>(p.Z()));
    }
    // indices: 连续线段 [0,1], [1,2], ...
    result.visualData.edges.push_back(std::move(edgeMesh));
    // 每条线段标注 EntityTag{GeoEdge, j}
}
```

**点提取关键代码：**
```cpp
PointSet points;
for (int k = 1; k <= entry.vertexMap.Extent(); ++k) {
    auto vertex = TopoDS::Vertex(entry.vertexMap.FindKey(k));
    gp_Pnt p = BRep_Tool::Pnt(vertex);
    points.positions.push_back(static_cast<float>(p.X()));
    points.positions.push_back(static_cast<float>(p.Y()));
    points.positions.push_back(static_cast<float>(p.Z()));
    result.vertexTags.push_back({Core::EntityType::GeoVertex, static_cast<uint32_t>(k)});
}
result.visualData.points.push_back(std::move(points));
```

- [ ] 步骤 3：创建 `tessellator_test.cpp`，测试用例：
  - 对 BRepPrimAPI_MakeBox 结果离散化 → surfaces 非空、edges 非空、points 非空
  - triangleTags 每个都是 GeoFace，localId 范围 1..6
  - edgeTags 每个都是 GeoEdge，localId 范围 1..12
  - vertexTags 每个都是 GeoVertex，localId 范围 1..8
  - 对 BRepPrimAPI_MakeSphere 结果离散化 → 验证基本正确性

- [ ] 步骤 4：更新 CMakeLists.txt 添加源文件和测试
- [ ] 步骤 5：构建并运行测试

**验证命令：**
```bash
cmake --build build --target opengeolab_geometry --config Debug --parallel 4
cmake --build build --target opengeolab_tessellator_test --config Debug --parallel 4
ctest --test-dir build -C Debug -R tessellator --output-on-failure
```

**预期结果：** 编译通过，tessellator 测试全绿。

---

### 任务 7：ShapeStore.tessellate 集成

**目的：** 将 Tessellator 集成到 ShapeStore.tessellate() 方法中。

**文件：**
- 修改：`src/libs/geometry/src/shape_store.cpp`
- 修改：`src/libs/geometry/test/shape_store_test.cpp`

- [ ] 步骤 1：在 `shape_store.cpp` 的 `tessellate()` 方法中调用 `Geometry::tessellate(entry, ...)` 并将结果写入 entry
- [ ] 步骤 2：tessellate 完成后 emit `shapeUpdated` 信号
- [ ] 步骤 3：在 `shape_store_test.cpp` 中添加测试：
  - tessellate 后 entry 的 visualData 非空
  - tessellate 后 shapeUpdated 信号被触发
- [ ] 步骤 4：构建并运行测试

**验证命令：**
```bash
cmake --build build --target opengeolab_shape_store_test --config Debug --parallel 4
ctest --test-dir build -C Debug -R shape_store --output-on-failure
```

**预期结果：** 所有 shape_store 测试通过。

---

### 任务 8：GeometryModule 重构 + CreateBoxAction 实现

**目的：** GeometryModule 持有 ShapeStore，暴露 shapeStore() 访问器；CreateBoxAction 替换现有 mock 为真正的 OCC Box 创建。

**文件：**
- 修改：`src/libs/geometry/include/opengeolab/geometry/geometry_module.hpp`
- 修改：`src/libs/geometry/src/geometry_module.cpp`
- 修改：`src/libs/geometry/include/opengeolab/geometry/create_box_action.hpp`
- 修改：`src/libs/geometry/src/create_box_action.cpp`
- 修改：`src/libs/geometry/test/geometry_module_test.cpp`

- [ ] 步骤 1：修改 `geometry_module.hpp`：
  - 添加 `#include <opengeolab/geometry/shape_store.hpp>`
  - 添加 `ShapeStore m_shapeStore;` 私有成员
  - 添加公共方法 `ShapeStore& shapeStore();` 和 `const ShapeStore& shapeStore() const;`

- [ ] 步骤 2：修改 `geometry_module.cpp`：
  - 构造函数中用 `registerAction<CreateBoxAction>(std::ref(m_shapeStore))` 注册
  - 实现 `shapeStore()` 访问器

- [ ] 步骤 3：修改 `create_box_action.hpp`：
  - 构造函数改为 `explicit CreateBoxAction(ShapeStore& store);`
  - 添加私有成员 `ShapeStore& m_store;`
  - 移除 mock 相关的 describe 和 execute

- [ ] 步骤 4：修改 `create_box_action.cpp`，替换 mock 为真正实现：

```cpp
nlohmann::json CreateBoxAction::execute(const nlohmann::json& param,
                                        const Core::ProgressCallback& progress) {
    const double width  = param.value("width", 1.0);
    const double height = param.value("height", 1.0);
    const double depth  = param.value("depth", 1.0);
    const auto name = param.value("name", std::string("Box"));

    std::array<double, 3> origin{0.0, 0.0, 0.0};
    if (param.contains("origin") && param["origin"].is_array()) {
        origin = param["origin"].get<std::array<double, 3>>();
    }

    progress(0.0, "Creating box...");

    gp_Pnt corner(origin[0], origin[1], origin[2]);
    BRepPrimAPI_MakeBox maker(corner, width, height, depth);
    maker.Build();
    if (!maker.IsDone()) {
        return {{"ok", false}, {"summary", "Box creation failed"}};
    }

    progress(0.3, "Registering shape...");
    auto shape_id = m_store.add(name, maker.Shape());

    bool do_tessellate = param.value("tessellate", true);
    if (do_tessellate) {
        progress(0.5, "Tessellating...");
        double lin = param.value("linearDeflection", 0.1);
        double ang = param.value("angularDeflection", 0.5);
        m_store.tessellate(shape_id, lin, ang);
    }

    progress(1.0, "Done");
    const auto* entry = m_store.find(shape_id);
    return {
        {"ok", true},
        {"action", "create_box"},
        {"shapeId", shape_id},
        {"name", name},
        {"topology", {
            {"solids", entry->solidMap.Extent()},
            {"faces", entry->faceMap.Extent()},
            {"edges", entry->edgeMap.Extent()},
            {"vertices", entry->vertexMap.Extent()},
            {"wires", entry->wireMap.Extent()}
        }}
    };
}
```

- [ ] 步骤 5：更新 `geometry_module_test.cpp` 适配新的返回格式：
  - describe 测试：actions 数量更新
  - dispatch 测试：返回值检查 `shapeId`、`topology` 字段
  - 移除 progress 延迟相关的断言（不再有 sleep）

- [ ] 步骤 6：构建并运行测试

**验证命令：**
```bash
cmake --build build --target opengeolab_geometry --config Debug --parallel 4
cmake --build build --target opengeolab_geometry_test --config Debug --parallel 4
ctest --test-dir build -C Debug -R geometry --output-on-failure
```

**预期结果：** 编译通过，geometry 模块测试全绿。

---

### 任务 9：CreateCylinder / CreateSphere / CreateTorus Actions

**目的：** 实现剩余三个参数化创建 action，模式与 CreateBoxAction 一致。

**文件：**
- 新增：`create_cylinder_action.hpp/cpp`、`create_sphere_action.hpp/cpp`、`create_torus_action.hpp/cpp`
- 新增：`src/libs/geometry/test/create_actions_test.cpp`
- 修改：`src/libs/geometry/src/geometry_module.cpp`（注册新 action）
- 修改：`src/libs/geometry/CMakeLists.txt`

每个 Action 模式相同：
1. 构造函数接受 `ShapeStore&`
2. `execute` 中使用对应的 OCC API：
   - Cylinder: `BRepPrimAPI_MakeCylinder(gp_Ax2(center, gp_Dir(0,0,1)), radius, height)`
   - Sphere: `BRepPrimAPI_MakeSphere(center, radius)`
   - Torus: `BRepPrimAPI_MakeTorus(gp_Ax2(center, gp_Dir(0,0,1)), majorRadius, minorRadius)`
3. 注册到 ShapeStore，可选离散化，返回拓扑信息

- [ ] 步骤 1：创建三个 action 的头文件和实现
- [ ] 步骤 2：在 `geometry_module.cpp` 中注册三个新 action
- [ ] 步骤 3：创建 `create_actions_test.cpp` 测试：
  - 每种形状创建后 shapeId 有效
  - 拓扑计数正确（Cylinder: 3 faces, Sphere: 1 face, Torus: 1 face）
  - tessellate 后 visualData 非空
- [ ] 步骤 4：更新 CMakeLists.txt
- [ ] 步骤 5：构建并运行测试

**验证命令：**
```bash
cmake --build build --target opengeolab_geometry --config Debug --parallel 4
ctest --test-dir build -C Debug -R create_actions --output-on-failure
```

**预期结果：** 编译通过，create_actions 测试全绿。

---

### 任务 10：ImportBrepAction + ImportStepAction

**目的：** 实现 BRep 和 STEP 文件导入。

**文件：**
- 新增：`import_brep_action.hpp/cpp`、`import_step_action.hpp/cpp`
- 新增：`src/libs/geometry/test/import_actions_test.cpp`
- 新增：`src/libs/geometry/test/data/box.brep`（由测试代码生成或预置）
- 新增：`src/libs/geometry/test/data/simple.stp`（由测试代码生成或预置）
- 修改：`src/libs/geometry/src/geometry_module.cpp`
- 修改：`src/libs/geometry/CMakeLists.txt`

- [ ] 步骤 1：创建 `import_brep_action.hpp/cpp`：

```cpp
// execute 关键逻辑
BRep_Builder builder;
TopoDS_Shape shape;
BRepTools::Read(shape, path.c_str(), builder);
if (shape.IsNull()) {
    return {{"ok", false}, {"summary", "Failed to read BRep file"}};
}
auto shape_id = m_store.add(name, shape);
// 可选 tessellate ...
```

- [ ] 步骤 2：创建 `import_step_action.hpp/cpp`：

```cpp
// execute 关键逻辑
STEPControl_Reader reader;
auto status = reader.ReadFile(path.c_str());
if (status != IFSelect_RetDone) {
    return {{"ok", false}, {"summary", "Failed to read STEP file"}};
}
reader.TransferRoots();
TopoDS_Shape shape = reader.OneShape();
auto shape_id = m_store.add(name, shape);
// 可选 tessellate ...
```

- [ ] 步骤 3：生成测试数据文件。在测试的 setup 中用 OCC API 写出测试文件：
  - `box.brep`：`BRepPrimAPI_MakeBox(1,1,1)` → `BRepTools::Write(shape, path)`
  - `simple.stp`：`BRepPrimAPI_MakeBox(2,2,2)` → `STEPControl_Writer` 导出

或者在 test fixture 中动态生成到临时目录。

- [ ] 步骤 4：创建 `import_actions_test.cpp`：
  - import_brep 成功读入 → shapeId 有效 → 拓扑正确
  - import_step 成功读入 → shapeId 有效 → 拓扑正确
  - import_brep 对不存在的路径返回错误
  - import_step 对不存在的路径返回错误
- [ ] 步骤 5：注册 action 到 geometry_module.cpp
- [ ] 步骤 6：更新 CMakeLists.txt
- [ ] 步骤 7：构建并运行测试

**验证命令：**
```bash
cmake --build build --target opengeolab_geometry --config Debug --parallel 4
ctest --test-dir build -C Debug -R import_actions --output-on-failure
```

**预期结果：** 编译通过，import_actions 测试全绿。

---

### 任务 11：TessellateAction + QueryShapeAction + ListShapesAction + DeleteShapeAction

**目的：** 实现管理类 action，完成 geometry 模块的全部 action 覆盖。

**文件：**
- 新增：`tessellate_action.hpp/cpp`、`query_shape_action.hpp/cpp`、`list_shapes_action.hpp/cpp`、`delete_shape_action.hpp/cpp`
- 修改：`src/libs/geometry/src/geometry_module.cpp`
- 修改：`src/libs/geometry/CMakeLists.txt`
- 修改：`src/libs/geometry/test/geometry_module_test.cpp`（更新 action 数量断言）

每个 Action 的核心逻辑：

**TessellateAction：**
```cpp
auto shape_id = param.value("shapeId", uint32_t(0));
double lin = param.value("linearDeflection", 0.1);
double ang = param.value("angularDeflection", 0.5);
m_store.tessellate(shape_id, lin, ang);
return {{"ok", true}, {"action", "tessellate"}, {"shapeId", shape_id}};
```

**QueryShapeAction：**
```cpp
auto shape_id = param.value("shapeId", uint32_t(0));
const auto* entry = m_store.find(shape_id);
if (!entry) return error;
// 返回拓扑信息 + bounding box (Bnd_Box)
```

**ListShapesAction：**
```cpp
auto ids = m_store.allShapeIds();
nlohmann::json shapes = nlohmann::json::array();
for (auto id : ids) {
    const auto* entry = m_store.find(id);
    shapes.push_back({{"shapeId", id}, {"name", entry->name}, ...});
}
return {{"ok", true}, {"action", "list_shapes"}, {"count", ids.size()}, {"shapes", shapes}};
```

**DeleteShapeAction：**
```cpp
auto shape_id = param.value("shapeId", uint32_t(0));
m_store.remove(shape_id);
return {{"ok", true}, {"action", "delete_shape"}, {"shapeId", shape_id}};
```

- [ ] 步骤 1：创建 4 个 action 的头文件和实现
- [ ] 步骤 2：在 geometry_module.cpp 注册所有新 action
- [ ] 步骤 3：更新 geometry_module_test.cpp 中 describe 测试的 action 数量（从 1 更新为 10）
- [ ] 步骤 4：更新 CMakeLists.txt
- [ ] 步骤 5：构建并运行全量 geometry 测试

**验证命令：**
```bash
cmake --build build --target opengeolab_geometry --config Debug --parallel 4
ctest --test-dir build -C Debug -R geometry --output-on-failure
```

**预期结果：** 编译通过，所有 geometry 相关测试全绿。

---

### 任务 12：全量回归验证 + clang-format + clang-tidy

**目的：** 确保整个项目编译通过、所有测试绿色、代码格式合规。

- [ ] 步骤 1：全量构建
- [ ] 步骤 2：全量测试
- [ ] 步骤 3：对所有新增/修改文件运行 clang-format
- [ ] 步骤 4：对所有新增/修改文件运行 clang-tidy（如果项目已配置）
- [ ] 步骤 5：修复任何格式或检查问题

**验证命令：**
```bash
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

**预期结果：** 全部编译通过，全部测试通过，代码格式合规。

---

### 任务 13：Git 提交

**目的：** 提交所有变更。

- [ ] 步骤 1：`git add` 所有新增和修改的文件
- [ ] 步骤 2：向用户确认提交信息和范围
- [ ] 步骤 3：使用以下格式提交：

```
feat(geometry): implement OCC-based geometry core with ShapeStore and tessellation

Phase 1 of geometry module: ShapeStore for OCC shape management with signal-based
scene integration, BRepMesh tessellator producing VisualData + EntityTag, parametric
shape creation (box/cylinder/sphere/torus), BRep/STEP file import, and shape
management actions (query/list/delete/tessellate).

Key design decisions:
- ShapeStore holds TopoDS_Shape with TopTools_IndexedMapOfShape sub-shape indices
- Signal/observer pattern for geometry → scene communication
- Pick encoding: shapeId(24) | entityType(8) + localId(32) in RG32UI FBO
- All actions injected with ShapeStore& via extended registerAction variadic template

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
```

**预期结果：** 干净的 Git 提交。
