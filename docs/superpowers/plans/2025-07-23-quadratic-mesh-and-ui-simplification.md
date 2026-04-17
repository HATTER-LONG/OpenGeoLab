# Quadratic 2D Mesh & UI Simplification — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add second-order 2D element types (Tri6, Quad8, Quad9) to the mesh pipeline and simplify the Generate Mesh UI.

**Architecture:** Extend `MeshElementType` enum with 3 new variants, bump `K_MAX_ELEMENT_NODES` to 9, add straight-edge rendering (reuse corner-node triangulation), add gmsh type mapping, add percentage-based sizing via OCC `Bnd_Box`, rewrite MeshGeneratePage QML to flatten advanced options.

**Tech Stack:** C++20 / doctest / gmsh C API / OCC `BRepBndLib` / Qt6 QML

**Build commands:**
```
cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh opengeolab_mesh_test --parallel 4'
ctest --test-dir build -C RelWithDebInfo -R opengeolab_mesh_test --output-on-failure
```

**Spec:** `docs/superpowers/specs/2025-07-23-quadratic-mesh-and-ui-simplification-design.md`

---

### Task 1: MeshElementType data layer — enum, helpers, constants

**Files:**
- Modify: `src/libs/mesh/include/opengeolab/mesh/mesh_element_type.hpp`

- [ ] **Step 1: Add Tri6, Quad8, Quad9 to enum and update all switch functions**

In `src/libs/mesh/include/opengeolab/mesh/mesh_element_type.hpp`, apply these changes:

1. Add three new enum values after `Pyramid`:

```cpp
enum class MeshElementType : uint8_t {
    Triangle = 0, ///< 3-node triangle (2D)
    Quad = 1,     ///< 4-node quadrilateral (2D)
    Tetra = 2,    ///< 4-node tetrahedron (3D)
    Hexa = 3,     ///< 8-node hexahedron (3D)
    Prism = 4,    ///< 6-node prism/wedge (3D)
    Pyramid = 5,  ///< 5-node pyramid (3D)
    Tri6 = 6,     ///< 6-node quadratic triangle (2D)
    Quad8 = 7,    ///< 8-node serendipity quadrilateral (2D)
    Quad9 = 8,    ///< 9-node quadratic quadrilateral (2D)
};
```

2. Bump the constant:

```cpp
inline constexpr uint8_t K_MAX_ELEMENT_NODES = 9;
```

3. Add cases to `nodeCount()`:

```cpp
case MeshElementType::Tri6:
    return 6;
case MeshElementType::Quad8:
    return 8;
case MeshElementType::Quad9:
    return 9;
```

4. Add cases to `elementTypePrefix()`:

```cpp
case MeshElementType::Tri6:
    return "Tri6";
case MeshElementType::Quad8:
    return "Q8";
case MeshElementType::Quad9:
    return "Q9";
```

5. Update `elementDimension()` to explicitly list the new 2D types:

```cpp
[[nodiscard]] constexpr uint8_t elementDimension(MeshElementType type) noexcept {
    switch(type) {
    case MeshElementType::Triangle:
    case MeshElementType::Quad:
    case MeshElementType::Tri6:
    case MeshElementType::Quad8:
    case MeshElementType::Quad9:
        return 2;
    default:
        return 3;
    }
}
```

6. Add two new helper functions after `elementDimension()`:

```cpp
/// Number of corner (vertex) nodes — excludes mid-edge/mid-face nodes.
[[nodiscard]] constexpr uint8_t cornerCount(MeshElementType type) noexcept {
    switch(type) {
    case MeshElementType::Triangle:
    case MeshElementType::Tri6:
        return 3;
    case MeshElementType::Quad:
    case MeshElementType::Quad8:
    case MeshElementType::Quad9:
        return 4;
    case MeshElementType::Tetra:
        return 4;
    case MeshElementType::Hexa:
        return 8;
    case MeshElementType::Prism:
        return 6;
    case MeshElementType::Pyramid:
        return 5;
    }
    return 0;
}

/// Map a second-order type to its first-order equivalent for rendering.
[[nodiscard]] constexpr MeshElementType linearEquivalent(MeshElementType type) noexcept {
    switch(type) {
    case MeshElementType::Tri6:
        return MeshElementType::Triangle;
    case MeshElementType::Quad8:
    case MeshElementType::Quad9:
        return MeshElementType::Quad;
    default:
        return type;
    }
}
```

- [ ] **Step 2: Run clang-format**

```
clang-format -i src/libs/mesh/include/opengeolab/mesh/mesh_element_type.hpp
```

- [ ] **Step 3: Build to verify compilation**

```
cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh --parallel 4'
```

Expected: BUILD SUCCESS. There will be compiler warnings about unhandled enum values in switch statements in `mesh_render_builder.cpp` and `mesh_topology.cpp` — these are fixed in Tasks 2 and 3.

- [ ] **Step 4: Commit**

```
git add src/libs/mesh/include/opengeolab/mesh/mesh_element_type.hpp
git commit -m "feat(mesh): add Tri6, Quad8, Quad9 element types and helpers

Add three second-order 2D element types to MeshElementType enum.
Bump K_MAX_ELEMENT_NODES from 8 to 9 for Quad9.
Add cornerCount() and linearEquivalent() helpers for rendering dispatch.
Update nodeCount(), elementTypePrefix(), elementDimension() with new cases.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 2: Mesh render builder — second-order rendering support

**Files:**
- Modify: `src/libs/mesh/src/mesh_render_builder.cpp:51-134`
- Test: `src/libs/mesh/test/mesh_render_builder_test.cpp`

- [ ] **Step 1: Write failing tests for Tri6 and Quad9 rendering**

Append to `src/libs/mesh/test/mesh_render_builder_test.cpp`:

```cpp
namespace {

Mesh::MeshEntry makeTri6Entry() {
    Mesh::MeshEntry entry;
    entry.shapeId = 1;
    entry.nodes = {
        Mesh::MeshNode{{0.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{2.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{1.0F, 2.0F, 0.0F}},
        Mesh::MeshNode{{1.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{1.5F, 1.0F, 0.0F}},
        Mesh::MeshNode{{0.5F, 1.0F, 0.0F}},
    };
    Mesh::MeshElement tri6;
    tri6.type = Mesh::MeshElementType::Tri6;
    tri6.nodeLocalIds = {1, 2, 3, 4, 5, 6, 0, 0, 0};
    entry.elements = {tri6};
    return entry;
}

Mesh::MeshEntry makeQuad9Entry() {
    Mesh::MeshEntry entry;
    entry.shapeId = 1;
    entry.nodes = {
        Mesh::MeshNode{{0.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{2.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{2.0F, 2.0F, 0.0F}},
        Mesh::MeshNode{{0.0F, 2.0F, 0.0F}},
        Mesh::MeshNode{{1.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{2.0F, 1.0F, 0.0F}},
        Mesh::MeshNode{{1.0F, 2.0F, 0.0F}},
        Mesh::MeshNode{{0.0F, 1.0F, 0.0F}},
        Mesh::MeshNode{{1.0F, 1.0F, 0.0F}},
    };
    Mesh::MeshElement quad9;
    quad9.type = Mesh::MeshElementType::Quad9;
    quad9.nodeLocalIds = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    entry.elements = {quad9};
    return entry;
}

} // namespace

TEST_CASE("MeshRenderBuilder: Tri6 renders as single triangle with 6 point ranges") {
    const auto data = Mesh::MeshRenderBuilder::build(1, makeTri6Entry());
    CHECK(data.triangleRanges.size() == 1);
    CHECK(data.triangleRanges[0].indexCount == 3);
    CHECK(data.lineRanges.size() == 3);
    CHECK(data.pointRanges.size() == 6);
}

TEST_CASE("MeshRenderBuilder: Quad9 renders as 2 triangles with 4 edges and 9 nodes") {
    const auto data = Mesh::MeshRenderBuilder::build(1, makeQuad9Entry());
    CHECK(data.triangleRanges.size() == 1);
    CHECK(data.triangleRanges[0].indexCount == 6);
    CHECK(data.lineRanges.size() == 4);
    CHECK(data.pointRanges.size() == 9);
}
```

- [ ] **Step 2: Run tests to verify they fail**

```
cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_mesh_test --output-on-failure'
```

Expected: FAIL — `appendElementTriangles` and `appendElementEdges` have no cases for Tri6/Quad9, so no triangle/line ranges are produced.

- [ ] **Step 3: Add Tri6, Quad8, Quad9 cases to appendElementTriangles and appendElementEdges**

In `src/libs/mesh/src/mesh_render_builder.cpp`, add to `appendElementTriangles()` after the `Pyramid` case:

```cpp
case MeshElementType::Tri6:
    triangles.push_back({0, 1, 2});
    return;
case MeshElementType::Quad8:
case MeshElementType::Quad9:
    triangles.push_back({0, 1, 2});
    triangles.push_back({0, 2, 3});
    return;
```

And add to `appendElementEdges()` after the `Pyramid` case:

```cpp
case MeshElementType::Tri6:
    edges.insert(edges.end(), {{0, 1}, {1, 2}, {2, 0}});
    return;
case MeshElementType::Quad8:
case MeshElementType::Quad9:
    edges.insert(edges.end(), {{0, 1}, {1, 2}, {2, 3}, {3, 0}});
    return;
```

- [ ] **Step 4: Run tests to verify they pass**

```
cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_mesh_test --output-on-failure'
```

Expected: ALL TESTS PASS

- [ ] **Step 5: Run clang-format**

```
clang-format -i src/libs/mesh/src/mesh_render_builder.cpp src/libs/mesh/test/mesh_render_builder_test.cpp
```

- [ ] **Step 6: Commit**

```
git add src/libs/mesh/src/mesh_render_builder.cpp src/libs/mesh/test/mesh_render_builder_test.cpp
git commit -m "feat(mesh): add straight-edge rendering for Tri6, Quad8, Quad9

Second-order elements use corner-only triangulation and edges.
Mid-edge nodes are rendered as points by the existing node pass.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 3: Mesh topology — second-order edge support

**Files:**
- Modify: `src/libs/mesh/src/mesh_topology.cpp:22-55`
- Test: `src/libs/mesh/test/mesh_topology_test.cpp`

- [ ] **Step 1: Write failing tests for Tri6 and Quad9 topology**

Append helper functions to `src/libs/mesh/test/mesh_topology_test.cpp` (in the anonymous namespace):

```cpp
MeshEntry makeSingleTri6() {
    MeshEntry entry;
    entry.shapeId = 10;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}},
        MeshNode{{2.0F, 0.0F, 0.0F}},
        MeshNode{{1.0F, 2.0F, 0.0F}},
        MeshNode{{1.0F, 0.0F, 0.0F}},
        MeshNode{{1.5F, 1.0F, 0.0F}},
        MeshNode{{0.5F, 1.0F, 0.0F}},
    };
    MeshElement tri6{};
    tri6.type = MeshElementType::Tri6;
    tri6.nodeLocalIds = {1, 2, 3, 4, 5, 6, 0, 0, 0};
    entry.elements = {tri6};
    return entry;
}

MeshEntry makeSingleQuad9() {
    MeshEntry entry;
    entry.shapeId = 11;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}},
        MeshNode{{2.0F, 0.0F, 0.0F}},
        MeshNode{{2.0F, 2.0F, 0.0F}},
        MeshNode{{0.0F, 2.0F, 0.0F}},
        MeshNode{{1.0F, 0.0F, 0.0F}},
        MeshNode{{2.0F, 1.0F, 0.0F}},
        MeshNode{{1.0F, 2.0F, 0.0F}},
        MeshNode{{0.0F, 1.0F, 0.0F}},
        MeshNode{{1.0F, 1.0F, 0.0F}},
    };
    MeshElement quad9{};
    quad9.type = MeshElementType::Quad9;
    quad9.nodeLocalIds = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    entry.elements = {quad9};
    return entry;
}
```

Then append test cases:

```cpp
TEST_CASE("MeshTopology: Tri6 has 3 edges (corner-only)") {
    const auto entry = makeSingleTri6();
    const auto topo = MeshTopology::build(entry);
    REQUIRE(topo.has_value());
    CHECK(topo->edges.size() == 3);
    CHECK(topo->elementAdjacency.size() == 1);
}

TEST_CASE("MeshTopology: Quad9 has 4 edges (corner-only)") {
    const auto entry = makeSingleQuad9();
    const auto topo = MeshTopology::build(entry);
    REQUIRE(topo.has_value());
    CHECK(topo->edges.size() == 4);
    CHECK(topo->elementAdjacency.size() == 1);
}
```

- [ ] **Step 2: Run tests to verify they fail**

```
cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_mesh_test --output-on-failure'
```

Expected: FAIL — topology `appendElementEdges` has no cases for Tri6/Quad9, producing 0 edges.

- [ ] **Step 3: Add Tri6, Quad8, Quad9 cases to appendElementEdges in mesh_topology.cpp**

In `src/libs/mesh/src/mesh_topology.cpp`, add after the `Pyramid` case in `appendElementEdges()`:

```cpp
case MeshElementType::Tri6:
    out.insert(out.end(), {{0, 1}, {1, 2}, {2, 0}});
    return;
case MeshElementType::Quad8:
case MeshElementType::Quad9:
    out.insert(out.end(), {{0, 1}, {1, 2}, {2, 3}, {3, 0}});
    return;
```

- [ ] **Step 4: Run tests to verify they pass**

```
cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_mesh_test --output-on-failure'
```

Expected: ALL TESTS PASS

- [ ] **Step 5: Run clang-format and commit**

```
clang-format -i src/libs/mesh/src/mesh_topology.cpp src/libs/mesh/test/mesh_topology_test.cpp
git add src/libs/mesh/src/mesh_topology.cpp src/libs/mesh/test/mesh_topology_test.cpp
git commit -m "feat(mesh): add Tri6/Quad8/Quad9 edge derivation in topology

Corner-only edges match the straight-edge rendering approach.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 4: gmsh type mapping + percentage sizing

**Files:**
- Modify: `src/libs/mesh/src/action/generate_mesh_action.cpp:6-19,36-49,138-193,277-467,476-509,512-579`

- [ ] **Step 1: Add second-order gmsh type mappings**

In `src/libs/mesh/src/action/generate_mesh_action.cpp`, update `mapGmshElementType()`:

```cpp
std::optional<MeshElementType> mapGmshElementType(const int gmsh_type) {
    switch(gmsh_type) {
    case 2:
        return MeshElementType::Triangle;
    case 3:
        return MeshElementType::Quad;
    case 4:
        return MeshElementType::Tetra;
    case 5:
        return MeshElementType::Hexa;
    case 6:
        return MeshElementType::Prism;
    case 7:
        return MeshElementType::Pyramid;
    case 9:
        return MeshElementType::Tri6;
    case 10:
        return MeshElementType::Quad9;
    case 16:
        return MeshElementType::Quad8;
    default:
        return std::nullopt;
    }
}
```

- [ ] **Step 2: Add sizeMode + percentage support**

Add include at the top of `generate_mesh_action.cpp` (after the OCC includes):

```cpp
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
```

Add a `sizeMode` field to `MeshSettings`:

```cpp
struct MeshSettings {
    double minSize{1.0};
    double maxSize{1.0};
    int dimension{2};
    int order{1};
    bool recombine{false};
    bool optimize{false};
    int algorithmCode{5};
    std::string sizeMode{"absolute"};
};
```

In `parseSettings()`, parse `sizeMode` from `param`:

After the existing `settings.dimension` and `settings.recombine` lines, add:

```cpp
settings.sizeMode = toLower(param.value("sizeMode", std::string{"absolute"}));
if(settings.sizeMode != "absolute" && settings.sizeMode != "percentage") {
    error = "sizeMode must be 'absolute' or 'percentage'";
    return std::nullopt;
}
```

- [ ] **Step 3: Apply percentage scaling in buildMeshEntry**

In `buildMeshEntry()`, after `gmshModelOccSynchronize()` succeeds but before setting `Mesh.MeshSizeMin`, add:

```cpp
auto actual_settings = settings;
if(actual_settings.sizeMode == "percentage") {
    Bnd_Box bbox;
    BRepBndLib::Add(compound, bbox);
    if(!bbox.IsVoid()) {
        const double diagonal = std::sqrt(bbox.SquareExtent());
        actual_settings.minSize = diagonal * (actual_settings.minSize / 100.0);
        actual_settings.maxSize = diagonal * (actual_settings.maxSize / 100.0);
    }
}
```

Then use `actual_settings` instead of `settings` for the `gmshOptionSetNumber` calls that follow (MeshSizeMin, MeshSizeMax, ElementOrder, RecombineAll, Algorithm).

Add `#include <cmath>` at the top if not already present.

- [ ] **Step 4: Update describe() to document sizeMode and order**

In the `describe()` method, add `sizeMode` to the params:

```cpp
{"sizeMode",
 {{"type", "string"},
  {"required", false},
  {"description", "Size mode: 'absolute' (default) or 'percentage' of bounding box diagonal."}}}
```

- [ ] **Step 5: Build to verify**

```
cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh --parallel 4'
```

Expected: BUILD SUCCESS

- [ ] **Step 6: Run clang-format and commit**

```
clang-format -i src/libs/mesh/src/action/generate_mesh_action.cpp
git add src/libs/mesh/src/action/generate_mesh_action.cpp
git commit -m "feat(mesh): add gmsh quadratic type mapping and percentage sizing

Map gmsh types 9/10/16 to Tri6/Quad9/Quad8 for second-order support.
Add sizeMode param: 'percentage' computes element size from bounding
box diagonal of the target compound shape.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 5: Split mesh guard for second-order elements

**Files:**
- Modify: `src/libs/mesh/src/action/split_mesh_action.cpp:203-217`
- Test: `src/libs/mesh/test/split_mesh_action_test.cpp`

- [ ] **Step 1: Write failing test**

Append to `src/libs/mesh/test/split_mesh_action_test.cpp`. First check the existing test helpers — the file creates `MeshStore`, inserts entries, and calls `execute()`. Add a test case:

```cpp
TEST_CASE("SplitMeshAction rejects second-order elements") {
    Mesh::MeshStore store;
    Mesh::MeshEntry entry;
    entry.shapeId = 99;
    entry.nodes = {
        Mesh::MeshNode{{0.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{2.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{1.0F, 2.0F, 0.0F}},
        Mesh::MeshNode{{1.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{1.5F, 1.0F, 0.0F}},
        Mesh::MeshNode{{0.5F, 1.0F, 0.0F}},
    };
    Mesh::MeshElement tri6{};
    tri6.type = Mesh::MeshElementType::Tri6;
    tri6.nodeLocalIds = {1, 2, 3, 4, 5, 6, 0, 0, 0};
    entry.elements = {tri6};
    store.setMesh(99, std::move(entry));

    Mesh::SplitMeshAction action(store);
    const auto param = nlohmann::json{
        {"shapeId", 99},
        {"selections", {{{"type", "edge"}, {"localId", 1}}}},
    };
    const auto result = action.execute(param, nullptr);
    CHECK(result["ok"].get<bool>() == false);
    CHECK(result["summary"].get<std::string>().find("second-order") != std::string::npos);
}
```

- [ ] **Step 2: Run tests to verify it fails**

```
cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_mesh_test --output-on-failure'
```

Expected: FAIL — currently the algorithm would attempt to split and either crash or produce wrong results.

- [ ] **Step 3: Add second-order guard in split_mesh_action.cpp**

In `src/libs/mesh/src/action/split_mesh_action.cpp`, after the line `const auto entry = *entry_ptr;` (line 213) and before `const auto topology = *topology_ptr;` (line 214), add:

```cpp
for(const auto& element : entry.elements) {
    if(element.type != MeshElementType::Triangle && element.type != MeshElementType::Quad) {
        return makeFailure(
            "Mesh split is not supported for second-order elements. "
            "Convert to linear elements first.");
    }
}
```

Add include at the top if needed:
```cpp
#include <opengeolab/mesh/mesh_element_type.hpp>
```

- [ ] **Step 4: Run tests to verify they pass**

```
cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && cmake --build build --config RelWithDebInfo --target opengeolab_mesh_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_mesh_test --output-on-failure'
```

Expected: ALL TESTS PASS

- [ ] **Step 5: Run clang-format and commit**

```
clang-format -i src/libs/mesh/src/action/split_mesh_action.cpp src/libs/mesh/test/split_mesh_action_test.cpp
git add src/libs/mesh/src/action/split_mesh_action.cpp src/libs/mesh/test/split_mesh_action_test.cpp
git commit -m "feat(mesh): reject second-order elements in mesh split

Return explicit error when mesh contains Tri6/Quad8/Quad9 elements
to prevent algorithm crash on unsupported topologies.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 6: Simplify MeshGeneratePage QML

**Files:**
- Modify: `src/app/resource/qml/components/pages/MeshGeneratePage.qml`

- [ ] **Step 1: Rewrite MeshGeneratePage.qml**

Replace the entire file content with the simplified layout. Key changes:

1. Remove `minSize`, `maxSize` properties
2. Add `sizeMode` property (`"absolute"` or `"percentage"`)
3. Add `meshOrder` as 1 or 2 with button group
4. Remove the `advancedHeader` fold and all of `Column { visible: advancedHeader.expanded ... }`
5. Keep `optimizeMesh` as a top-level switch
6. Size input row: `[Absolute ▾] [DimensionInput]` — dropdown switches sizeMode
7. Order row: `[Linear] [Quadratic]` button toggle

The full replacement QML:

```qml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import OpenGeoLab.Services 1.0
import "../.."
import ".."

FunctionPageBase {
    id: root

    pageTitle: qsTr("Generate Mesh")
    pageIcon: "mesh"
    actionId: "generateMesh"
    maxContentHeight: 560

    property real elementSize: 10.0
    property int meshDimension: 2
    property string elementType: "triangle"
    property string algorithm: "delaunay"
    property string sizeMode: "absolute"
    property int meshOrder: 1
    property bool optimizeMesh: false

    readonly property var algorithmOptions2d: [
        { label: qsTr("Automatic"), value: "automatic" },
        { label: qsTr("MeshAdapt"), value: "meshadapt" },
        { label: qsTr("Delaunay"), value: "delaunay" },
        { label: qsTr("Frontal"), value: "frontal" },
        { label: qsTr("BAMG"), value: "bamg" },
        { label: qsTr("Frontal Quad"), value: "frontal_quad" }
    ]
    readonly property var algorithmOptions3d: [
        { label: qsTr("Delaunay"), value: "delaunay" },
        { label: qsTr("Frontal"), value: "frontal" },
        { label: qsTr("MMG3D"), value: "mmg3d" },
        { label: qsTr("RTree"), value: "rtree" },
        { label: qsTr("HXT"), value: "hxt" }
    ]
    readonly property var algorithmOptions: root.meshDimension === 3
        ? root.algorithmOptions3d
        : root.algorithmOptions2d

    onMeshDimensionChanged: {
        if (!root.algorithmSupported(root.algorithm)) {
            root.algorithm = "delaunay";
        }
    }

    function algorithmLabel(value) {
        for (var i = 0; i < root.algorithmOptions.length; i++) {
            if (root.algorithmOptions[i].value === value) {
                return root.algorithmOptions[i].label;
            }
        }
        return value;
    }

    function algorithmSupported(value) {
        for (var i = 0; i < root.algorithmOptions.length; i++) {
            if (root.algorithmOptions[i].value === value) {
                return true;
            }
        }
        return false;
    }

    function open(payload) {
        root.x = 292;
        root.y = 0;
        root.pageVisible = true;
        root.forceActiveFocus();
        sceneCommand("set_pick_mode", {
            pickMask: geoTypeSelector.mask,
            enabled: true
        });
    }

    function close() {
        sceneCommand("set_pick_mode", { enabled: false });
        sceneCommand("clear_selection", {});
        root.pageVisible = false;
        if (MainPages.currentOpenPage === root.actionId) {
            MainPages.currentOpenPage = "";
        }
    }

    function getParameters() {
        var entities = [];
        var selections = SelectionService.selections;
        for (var i = 0; i < selections.length; i++) {
            entities.push({
                shapeId: selections[i].shapeId,
                type: entityTypeTag(selections[i].entityType),
                localId: selections[i].localId
            });
        }
        return {
            module: "mesh",
            action: "generate_mesh",
            param: {
                entities: entities,
                elementSize: root.elementSize,
                sizeMode: root.sizeMode,
                dimension: root.meshDimension,
                elementType: root.elementType,
                algorithm: root.algorithm,
                advanced: {
                    order: root.meshOrder,
                    optimize: root.optimizeMesh
                }
            },
            mute: false
        };
    }

    function execute() {
        if (RequestService.busy || SelectionService.selections.length === 0) {
            return;
        }
        RequestService.submitAsync(JSON.stringify(root.getParameters()));
        root.close();
    }

    function sceneCommand(action, param) {
        RequestService.submitAsync(JSON.stringify({
            module: "scene",
            action: action,
            param: param ?? {},
            mute: true
        }));
    }

    function entityTypeTag(typeInt) {
        var map = { 3: "GeoFace", 4: "GeoSolid" };
        return map[typeInt] ?? "GeoFace";
    }

    // --- Target Geometry ---

    Text {
        text: qsTr("Target Geometry")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    EntityTypeSelector {
        id: geoTypeSelector

        width: parent.width
        theme: root.theme
        mask: geoTypeSelector.maskFace
        exclusiveMasks: [geoTypeSelector.maskFace, geoTypeSelector.maskSolid]
        typeModel: [
            { label: qsTr("Face"),  icon: "entityFace",  mask: geoTypeSelector.maskFace },
            { label: qsTr("Solid"), icon: "entitySolid", mask: geoTypeSelector.maskSolid }
        ]

        onMaskChanged: {
            sceneCommand("set_pick_mode", { pickMask: geoTypeSelector.mask });
        }
    }

    Rectangle {
        width: parent.width
        height: 24
        radius: root.theme.radiusSmall
        color: SelectionService.pickEnabled
            ? root.theme.tint(root.theme.accentA, root.theme.darkMode ? 0.14 : 0.08)
            : "transparent"
        visible: SelectionService.pickEnabled

        Behavior on color {
            ColorAnimation { duration: root.theme.animNormal }
        }

        Row {
            anchors.centerIn: parent
            spacing: 6

            Rectangle {
                width: 6; height: 6; radius: 3
                anchors.verticalCenter: parent.verticalCenter
                color: root.theme.accentA

                SequentialAnimation on opacity {
                    running: SelectionService.pickEnabled
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 0.3; duration: 800; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 0.3; to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
                }
            }

            Text {
                text: qsTr("Click faces or solids in the viewport")
                color: root.theme.accentA
                font.pixelSize: 10
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Text {
        text: qsTr("Selected: %1").arg(SelectionService.selections.length)
        color: root.theme.textSecondary
        font.pixelSize: 12
        visible: SelectionService.selections.length > 0
    }

    Flickable {
        id: chipFlickable

        width: parent.width
        height: Math.min(chipFlow.implicitHeight, 80)
        contentHeight: chipFlow.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height
        visible: SelectionService.selections.length > 0

        Flow {
            id: chipFlow

            width: chipFlickable.width
            spacing: 6

            Repeater {
                model: SelectionService.selections

                EntityChip {
                    required property var modelData

                    theme: root.theme
                    shapeId: modelData.shapeId
                    entityType: modelData.entityType
                    localId: modelData.localId

                    onRemoveRequested: function(sid, etype, lid) {
                        sceneCommand("deselect", {
                            entities: [{ shapeId: sid, type: root.entityTypeTag(etype), localId: lid }]
                        });
                    }
                }
            }
        }
    }

    Rectangle {
        width: parent.width
        height: 40
        radius: root.theme.radiusSmall
        color: root.theme.surfaceMuted
        visible: SelectionService.selections.length === 0

        Text {
            anchors.centerIn: parent
            text: qsTr("No geometry selected.\nPick faces or solids from the viewport.")
            color: root.theme.textTertiary
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
            lineHeight: 1.3
        }
    }

    // --- Mesh Parameters ---

    Text {
        text: qsTr("Mesh Parameters")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    RowLayout {
        width: parent.width
        spacing: 6

        Row {
            spacing: 4

            Repeater {
                model: [
                    { label: qsTr("Absolute"), value: "absolute" },
                    { label: "%", value: "percentage" }
                ]

                delegate: Rectangle {
                    required property var modelData

                    width: modelData.value === "percentage" ? 30 : 62
                    height: 24
                    radius: root.theme.radiusSmall
                    color: root.sizeMode === modelData.value
                        ? root.theme.tint(root.theme.accentB, root.theme.darkMode ? 0.24 : 0.14)
                        : root.theme.surfaceMuted
                    border.width: root.sizeMode === modelData.value ? 1.5 : 1
                    border.color: root.sizeMode === modelData.value
                        ? root.theme.accentB
                        : root.theme.borderSubtle

                    Text {
                        anchors.centerIn: parent
                        text: parent.modelData.label
                        font.pixelSize: 10
                        font.bold: root.sizeMode === parent.modelData.value
                        color: root.sizeMode === parent.modelData.value
                            ? root.theme.accentB
                            : root.theme.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.sizeMode = parent.modelData.value
                    }
                }
            }
        }

        DimensionInput {
            Layout.fillWidth: true
            theme: root.theme
            label: qsTr("Size")
            value: root.elementSize
            accentColor: root.theme.accentB
            tooltipText: root.sizeMode === "percentage"
                ? qsTr("Percentage of bounding box diagonal")
                : qsTr("Target element size")

            onValueEdited: function(newVal) {
                root.elementSize = newVal;
            }
        }
    }

    RowLayout {
        width: parent.width
        spacing: 6

        Text {
            text: qsTr("Element")
            color: root.theme.textSecondary
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        Row {
            spacing: 4

            Repeater {
                model: [
                    { label: qsTr("Tri"), value: "triangle" },
                    { label: qsTr("Quad"), value: "quad" }
                ]

                delegate: Rectangle {
                    required property var modelData

                    width: 44
                    height: 24
                    radius: root.theme.radiusSmall
                    color: root.elementType === modelData.value
                        ? root.theme.tint(root.theme.accentB, root.theme.darkMode ? 0.24 : 0.14)
                        : root.theme.surfaceMuted
                    border.width: root.elementType === modelData.value ? 1.5 : 1
                    border.color: root.elementType === modelData.value
                        ? root.theme.accentB
                        : root.theme.borderSubtle

                    Text {
                        anchors.centerIn: parent
                        text: parent.modelData.label
                        font.pixelSize: 10
                        font.bold: root.elementType === parent.modelData.value
                        color: root.elementType === parent.modelData.value
                            ? root.theme.accentB
                            : root.theme.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.elementType = parent.modelData.value
                    }
                }
            }
        }
    }

    RowLayout {
        width: parent.width
        spacing: 6

        Text {
            text: qsTr("Order")
            color: root.theme.textSecondary
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        Row {
            spacing: 4

            Repeater {
                model: [
                    { label: qsTr("Linear"), value: 1 },
                    { label: qsTr("Quadratic"), value: 2 }
                ]

                delegate: Rectangle {
                    required property var modelData

                    width: 68
                    height: 24
                    radius: root.theme.radiusSmall
                    color: root.meshOrder === modelData.value
                        ? root.theme.tint(root.theme.accentB, root.theme.darkMode ? 0.24 : 0.14)
                        : root.theme.surfaceMuted
                    border.width: root.meshOrder === modelData.value ? 1.5 : 1
                    border.color: root.meshOrder === modelData.value
                        ? root.theme.accentB
                        : root.theme.borderSubtle

                    Text {
                        anchors.centerIn: parent
                        text: parent.modelData.label
                        font.pixelSize: 10
                        font.bold: root.meshOrder === parent.modelData.value
                        color: root.meshOrder === parent.modelData.value
                            ? root.theme.accentB
                            : root.theme.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.meshOrder = parent.modelData.value
                    }
                }
            }
        }
    }

    RowLayout {
        width: parent.width
        spacing: 6

        Text {
            text: qsTr("Dimension")
            color: root.theme.textSecondary
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        Row {
            spacing: 4

            Repeater {
                model: [
                    { label: qsTr("2D"), value: 2 },
                    { label: qsTr("3D"), value: 3 }
                ]

                delegate: Rectangle {
                    required property var modelData

                    width: 36
                    height: 24
                    radius: root.theme.radiusSmall
                    color: root.meshDimension === modelData.value
                        ? root.theme.tint(root.theme.accentB, root.theme.darkMode ? 0.24 : 0.14)
                        : root.theme.surfaceMuted
                    border.width: root.meshDimension === modelData.value ? 1.5 : 1
                    border.color: root.meshDimension === modelData.value
                        ? root.theme.accentB
                        : root.theme.borderSubtle

                    Text {
                        anchors.centerIn: parent
                        text: parent.modelData.label
                        font.pixelSize: 10
                        font.bold: root.meshDimension === parent.modelData.value
                        color: root.meshDimension === parent.modelData.value
                            ? root.theme.accentB
                            : root.theme.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.meshDimension = parent.modelData.value
                    }
                }
            }
        }
    }

    Column {
        width: parent.width
        spacing: 6

        Text {
            text: qsTr("Algorithm")
            color: root.theme.textSecondary
            font.pixelSize: 12
        }

        Rectangle {
            width: parent.width
            height: algorithmInfoCol.implicitHeight + 16
            radius: root.theme.radiusSmall
            color: root.theme.surfaceMuted

            Column {
                id: algorithmInfoCol

                anchors.fill: parent
                anchors.margins: 8
                spacing: 4

                Text {
                    id: algorithmSummary

                    width: parent.width
                    text: qsTr("Selected algorithm: %1").arg(root.algorithmLabel(root.algorithm))
                    color: root.theme.textPrimary
                    font.pixelSize: 11
                    font.bold: true
                    wrapMode: Text.WordWrap
                }

                Text {
                    width: parent.width
                    text: root.meshDimension === 3
                        ? qsTr("3D mesh generation supports Delaunay, Frontal, MMG3D, RTree and HXT.")
                        : qsTr("2D mesh generation supports Automatic, MeshAdapt, Delaunay, Frontal, BAMG and Frontal Quad.")
                    color: root.theme.textSecondary
                    font.pixelSize: 10
                    wrapMode: Text.WordWrap
                }
            }
        }

        Flow {
            width: parent.width
            spacing: 6

            Repeater {
                model: root.algorithmOptions

                delegate: Rectangle {
                    required property var modelData

                    width: algorithmText.implicitWidth + 18
                    height: 26
                    radius: 13
                    color: root.algorithm === modelData.value
                        ? root.theme.tint(root.theme.accentC, root.theme.darkMode ? 0.24 : 0.14)
                        : root.theme.surfaceMuted
                    border.width: root.algorithm === modelData.value ? 1.5 : 1
                    border.color: root.algorithm === modelData.value
                        ? root.theme.accentC
                        : root.theme.borderSubtle

                    Text {
                        id: algorithmText

                        anchors.centerIn: parent
                        text: parent.modelData.label
                        font.pixelSize: 10
                        font.bold: root.algorithm === parent.modelData.value
                        color: root.algorithm === parent.modelData.value
                            ? root.theme.accentC
                            : root.theme.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.algorithm = parent.modelData.value
                    }
                }
            }
        }
    }

    RowLayout {
        width: parent.width
        spacing: 8

        Text {
            text: qsTr("Optimize")
            color: root.theme.textSecondary
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        Switch {
            checked: root.optimizeMesh
            onToggled: root.optimizeMesh = checked
        }
    }

    // --- Footer ---

    Rectangle {
        width: parent.width
        height: helperText.implicitHeight + 16
        radius: root.theme.radiusSmall
        color: root.theme.surfaceMuted

        Text {
            id: helperText

            anchors.fill: parent
            anchors.margins: 8
            text: SelectionService.selections.length === 0
                ? qsTr("Pick at least one face or solid before running mesh generation.")
                : qsTr("Generate Mesh uses the current selection and closes this panel after submitting the request.")
            color: SelectionService.selections.length === 0 ? root.theme.warning : root.theme.textSecondary
            font.pixelSize: 10
            wrapMode: Text.WordWrap
        }
    }

    Rectangle {
        width: parent.width
        height: 32
        radius: root.theme.radiusSmall
        color: clearMeshMouse.pressed
            ? root.theme.surfaceStrong
            : (clearMeshMouse.containsMouse
                ? root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.96 : 0.9)
                : root.theme.surfaceMuted)
        border.width: 1
        border.color: clearMeshMouse.containsMouse
            ? root.theme.tint(root.theme.danger, root.theme.darkMode ? 0.56 : 0.36)
            : root.theme.borderSubtle

        Behavior on color {
            ColorAnimation { duration: root.theme.animFast }
        }

        Text {
            anchors.centerIn: parent
            text: qsTr("Clear All Mesh")
            color: clearMeshMouse.containsMouse ? root.theme.danger : root.theme.textPrimary
            font.pixelSize: 12
            font.bold: true
        }

        MouseArea {
            id: clearMeshMouse

            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                RequestService.submitAsync(JSON.stringify({
                    module: "mesh",
                    action: "clear_mesh",
                    param: {},
                    mute: false
                }));
            }
        }
    }
}
```

- [ ] **Step 2: Build app to verify QML loads**

```
cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && cmake --build build --config RelWithDebInfo --target opengeolab_app --parallel 4'
```

Expected: BUILD SUCCESS

- [ ] **Step 3: Commit**

```
git add src/app/resource/qml/components/pages/MeshGeneratePage.qml
git commit -m "refactor(app): simplify MeshGeneratePage UI

Remove Advanced fold — promote Order, Optimize to main panel.
Remove minSize/maxSize inputs — single Size field with mode toggle.
Add sizeMode toggle (Absolute / %) for percentage-based sizing.
Add Order toggle (Linear / Quadratic) for second-order mesh.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 7: Translations

**Files:**
- Modify: `src/app/resource/translations/opengeolab_zh_CN.ts`

- [ ] **Step 1: Add MeshGeneratePage translation context**

Add a new `<context>` block for the new/changed strings in MeshGeneratePage. The context name must match the QML file name without extension:

```xml
<context>
    <name>MeshGeneratePage</name>
    <message>
        <source>Generate Mesh</source>
        <translation>生成网格</translation>
    </message>
    <message>
        <source>Target Geometry</source>
        <translation>目标几何</translation>
    </message>
    <message>
        <source>Face</source>
        <translation>面</translation>
    </message>
    <message>
        <source>Solid</source>
        <translation>实体</translation>
    </message>
    <message>
        <source>Click faces or solids in the viewport</source>
        <translation>在视口中点击面或实体</translation>
    </message>
    <message>
        <source>Selected: %1</source>
        <translation>已选择: %1</translation>
    </message>
    <message>
        <source>No geometry selected.
Pick faces or solids from the viewport.</source>
        <translation>未选择几何。
请从视口中选取面或实体。</translation>
    </message>
    <message>
        <source>Mesh Parameters</source>
        <translation>网格参数</translation>
    </message>
    <message>
        <source>Absolute</source>
        <translation>绝对值</translation>
    </message>
    <message>
        <source>Size</source>
        <translation>尺寸</translation>
    </message>
    <message>
        <source>Percentage of bounding box diagonal</source>
        <translation>包围盒对角线的百分比</translation>
    </message>
    <message>
        <source>Target element size</source>
        <translation>目标单元尺寸</translation>
    </message>
    <message>
        <source>Element</source>
        <translation>单元</translation>
    </message>
    <message>
        <source>Tri</source>
        <translation>三角</translation>
    </message>
    <message>
        <source>Quad</source>
        <translation>四边</translation>
    </message>
    <message>
        <source>Order</source>
        <translation>阶次</translation>
    </message>
    <message>
        <source>Linear</source>
        <translation>一阶</translation>
    </message>
    <message>
        <source>Quadratic</source>
        <translation>二阶</translation>
    </message>
    <message>
        <source>Dimension</source>
        <translation>维度</translation>
    </message>
    <message>
        <source>2D</source>
        <translation>2D</translation>
    </message>
    <message>
        <source>3D</source>
        <translation>3D</translation>
    </message>
    <message>
        <source>Algorithm</source>
        <translation>算法</translation>
    </message>
    <message>
        <source>Selected algorithm: %1</source>
        <translation>选定算法: %1</translation>
    </message>
    <message>
        <source>2D mesh generation supports Automatic, MeshAdapt, Delaunay, Frontal, BAMG and Frontal Quad.</source>
        <translation>2D 网格生成支持 Automatic、MeshAdapt、Delaunay、Frontal、BAMG 和 Frontal Quad 算法。</translation>
    </message>
    <message>
        <source>3D mesh generation supports Delaunay, Frontal, MMG3D, RTree and HXT.</source>
        <translation>3D 网格生成支持 Delaunay、Frontal、MMG3D、RTree 和 HXT 算法。</translation>
    </message>
    <message>
        <source>Automatic</source>
        <translation>自动</translation>
    </message>
    <message>
        <source>MeshAdapt</source>
        <translation>MeshAdapt</translation>
    </message>
    <message>
        <source>Delaunay</source>
        <translation>Delaunay</translation>
    </message>
    <message>
        <source>Frontal</source>
        <translation>Frontal</translation>
    </message>
    <message>
        <source>BAMG</source>
        <translation>BAMG</translation>
    </message>
    <message>
        <source>Frontal Quad</source>
        <translation>Frontal Quad</translation>
    </message>
    <message>
        <source>MMG3D</source>
        <translation>MMG3D</translation>
    </message>
    <message>
        <source>RTree</source>
        <translation>RTree</translation>
    </message>
    <message>
        <source>HXT</source>
        <translation>HXT</translation>
    </message>
    <message>
        <source>Optimize</source>
        <translation>优化</translation>
    </message>
    <message>
        <source>Pick at least one face or solid before running mesh generation.</source>
        <translation>生成网格前请先选取至少一个面或实体。</translation>
    </message>
    <message>
        <source>Generate Mesh uses the current selection and closes this panel after submitting the request.</source>
        <translation>生成网格将使用当前选择，提交后关闭面板。</translation>
    </message>
    <message>
        <source>Clear All Mesh</source>
        <translation>清除所有网格</translation>
    </message>
</context>
```

- [ ] **Step 2: Commit**

```
git add src/app/resource/translations/opengeolab_zh_CN.ts
git commit -m "docs(i18n): add Chinese translations for simplified MeshGeneratePage

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 8: Full build + test verification

- [ ] **Step 1: Full build**

```
cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && cmake --build build --config RelWithDebInfo --parallel 4'
```

Expected: BUILD SUCCESS with no warnings about unhandled enum cases.

- [ ] **Step 2: Full test suite**

```
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

Expected: ALL TESTS PASS

- [ ] **Step 3: Manual visual testing**

Launch app with `--start-http-server` and verify:

1. Create a box → generate **linear** tri mesh → verify normal triangle rendering
2. Create a box → generate **quadratic** tri mesh (Order = Quadratic) → verify Tri6 elements render with mid-edge nodes visible as points
3. Create a box → generate **quadratic** quad mesh → verify Quad9 elements render
4. Test percentage size mode → verify mesh density scales with geometry size
5. Verify mesh split rejects second-order mesh with clear error
6. Verify X-ray mode still works correctly with second-order mesh
