# 3D Model & Camera State Export — Part 1 of 4

> Part file: Topology extraction foundation — shared OCC utilities that all later actions depend on.
>
> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Build shared OCC topology extraction utilities (`topology_utils`) that
`DescribeTopologyAction`, `QueryEntityInfoAction`, and the enhanced
`CaptureViewportAction` all depend on.

**Architecture:** A pure utility module (header + source) inside `src/libs/geometry/`
with struct types (`FaceInfo`, `EdgeInfo`, `VertexInfo`), extraction functions that
wrap OCC BRep adaptor APIs, adjacency-map builders, and JSON serialisation. All
functions are free functions — no state, no singleton, easily testable.

**Tech Stack:** C++20, OpenCASCADE (BRepAdaptor, BRepGProp, TopExp), nlohmann/json, doctest.

**Design spec:** `docs/superpowers/specs/2026-04-10-3d-model-camera-state-export-design.md`

---

## File Structure (Part 1 scope)

| Action | Path | Purpose |
|--------|------|---------|
| Create | `src/libs/geometry/include/opengeolab/geometry/topology_utils.hpp` | Structs + function declarations |
| Create | `src/libs/geometry/src/topology_utils.cpp` | OCC extraction implementations |
| Create | `src/libs/geometry/test/topology_utils_test.cpp` | Tests for all topology utils |
| Modify | `src/libs/geometry/CMakeLists.txt` | Add new files to build |

---

### Task 1: Create `topology_utils.hpp`

**Files:**
- Create: `src/libs/geometry/include/opengeolab/geometry/topology_utils.hpp`

- [ ] **Step 1: Create the header file**

```cpp
/**
 * @file topology_utils.hpp
 * @brief Shared OCC topology extraction utilities
 *
 * Provides lightweight value types (FaceInfo, EdgeInfo, VertexInfo) and free
 * functions to extract topology summaries from OCC shapes.  Used by
 * DescribeTopologyAction, QueryEntityInfoAction, and CaptureViewportAction.
 */

#pragma once

#include <nlohmann/json_fwd.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class TopoDS_Face;
class TopoDS_Edge;
class TopoDS_Vertex;
class TopoDS_Shape;

namespace OpenGeoLab::Geometry {

struct ShapeEntry;

/// @brief Summary of a topological face.
struct FaceInfo {
    uint32_t localId{0};
    std::string surfaceType;
    std::array<double, 3> center{};
    std::array<double, 3> normal{};
    std::optional<std::array<double, 3>> axis;
    std::optional<double> radius;
    double area{0.0};
};

/// @brief Summary of a topological edge.
struct EdgeInfo {
    uint32_t localId{0};
    std::string curveType;
    std::array<double, 3> start{};
    std::array<double, 3> end{};
    std::optional<std::array<double, 3>> center;
    std::optional<double> radius;
    double length{0.0};
};

/// @brief Summary of a topological vertex.
struct VertexInfo {
    uint32_t localId{0};
    std::array<double, 3> position{};
};

// ── Extraction ───────────────────────────────────────────────

/// @brief Extract face summary from a TopoDS_Face.
[[nodiscard]] FaceInfo extractFaceInfo(uint32_t local_id, const TopoDS_Face& face);

/// @brief Extract edge summary from a TopoDS_Edge.
[[nodiscard]] EdgeInfo extractEdgeInfo(uint32_t local_id, const TopoDS_Edge& edge);

/// @brief Extract vertex summary from a TopoDS_Vertex.
[[nodiscard]] VertexInfo extractVertexInfo(uint32_t local_id, const TopoDS_Vertex& vertex);

// ── JSON Serialisation ───────────────────────────────────────

/// @brief Convert FaceInfo to JSON.
[[nodiscard]] nlohmann::json toJson(const FaceInfo& info);

/// @brief Convert EdgeInfo to JSON.
[[nodiscard]] nlohmann::json toJson(const EdgeInfo& info);

/// @brief Convert VertexInfo to JSON.
[[nodiscard]] nlohmann::json toJson(const VertexInfo& info);

// ── Adjacency ────────────────────────────────────────────────

/// @brief Build edge→face adjacency map (all localIds are 1-based).
[[nodiscard]] std::unordered_map<uint32_t, std::vector<uint32_t>>
buildEdgeToFaceAdjacency(const ShapeEntry& entry);

/// @brief Build vertex→edge adjacency map (all localIds are 1-based).
[[nodiscard]] std::unordered_map<uint32_t, std::vector<uint32_t>>
buildVertexToEdgeAdjacency(const ShapeEntry& entry);

/// @brief Build face→edge adjacency map (all localIds are 1-based).
[[nodiscard]] std::unordered_map<uint32_t, std::vector<uint32_t>>
buildFaceToEdgeAdjacency(const ShapeEntry& entry);

// ── Bounding Box ─────────────────────────────────────────────

/// @brief Compute AABB of an OCC sub-shape.
/// @return {min, max} arrays, or nullopt if the shape is degenerate/void.
[[nodiscard]] std::optional<std::pair<std::array<double, 3>, std::array<double, 3>>>
computeSubShapeBounds(const TopoDS_Shape& sub_shape);

} // namespace OpenGeoLab::Geometry
```

- [ ] **Step 2: Verify the header compiles**

Add it temporarily to the `geometry_public_headers` list in `src/libs/geometry/CMakeLists.txt`:

```cmake
# In geometry_public_headers, after the last entry:
    include/opengeolab/geometry/topology_utils.hpp)
```

Run:

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_geometry --parallel 4
```

Expected: Build succeeds (header-only, no link errors yet).

---

### Task 2: Write topology utils test (failing — no implementation yet)

**Files:**
- Create: `src/libs/geometry/test/topology_utils_test.cpp`

- [ ] **Step 1: Create the test file**

```cpp
/**
 * @file topology_utils_test.cpp
 * @brief Unit tests for topology extraction utilities
 */

#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/geometry/topology_utils.hpp>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <TopoDS.hxx>

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>

using OpenGeoLab::Geometry::EdgeInfo;
using OpenGeoLab::Geometry::FaceInfo;
using OpenGeoLab::Geometry::ShapeStore;
using OpenGeoLab::Geometry::VertexInfo;

static TopoDS_Shape makeBox(double w = 10.0, double h = 10.0, double d = 10.0) {
    return BRepPrimAPI_MakeBox(w, h, d).Shape();
}

static TopoDS_Shape makeCylinder(double r = 5.0, double h = 10.0) {
    return BRepPrimAPI_MakeCylinder(r, h).Shape();
}

TEST_SUITE("topology_utils") {

TEST_CASE("extractFaceInfo returns plane for box face") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);
    REQUIRE(entry->faceMap.Extent() == 6);

    const auto& face = TopoDS::Face(entry->faceMap(1));
    auto info = OpenGeoLab::Geometry::extractFaceInfo(1, face);
    CHECK(info.localId == 1);
    CHECK(info.surfaceType == "plane");
    CHECK(info.area == doctest::Approx(100.0).epsilon(0.01));
    // Plane faces should not have axis or radius
    CHECK_FALSE(info.axis.has_value());
    CHECK_FALSE(info.radius.has_value());
}

TEST_CASE("extractFaceInfo returns cylinder for cylinder lateral face") {
    ShapeStore store;
    auto id = store.add("Cyl", makeCylinder(5.0, 10.0));
    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);

    // Find the cylindrical face (there should be exactly one with surfaceType "cylinder")
    bool found_cylinder = false;
    for(int i = 1; i <= entry->faceMap.Extent(); ++i) {
        const auto& face = TopoDS::Face(entry->faceMap(i));
        auto info = OpenGeoLab::Geometry::extractFaceInfo(
            static_cast<uint32_t>(i), face);
        if(info.surfaceType == "cylinder") {
            found_cylinder = true;
            CHECK(info.radius.has_value());
            CHECK(info.radius.value() == doctest::Approx(5.0).epsilon(0.01));
            CHECK(info.axis.has_value());
            // Cylinder lateral area = 2*pi*r*h
            CHECK(info.area == doctest::Approx(2.0 * M_PI * 5.0 * 10.0).epsilon(0.5));
            break;
        }
    }
    CHECK(found_cylinder);
}

TEST_CASE("extractEdgeInfo returns line for box edge") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);
    REQUIRE(entry->edgeMap.Extent() == 12);

    const auto& edge = TopoDS::Edge(entry->edgeMap(1));
    auto info = OpenGeoLab::Geometry::extractEdgeInfo(1, edge);
    CHECK(info.localId == 1);
    CHECK(info.curveType == "line");
    CHECK(info.length == doctest::Approx(10.0).epsilon(0.01));
    CHECK_FALSE(info.center.has_value());
    CHECK_FALSE(info.radius.has_value());
}

TEST_CASE("extractEdgeInfo returns circle for cylinder edge") {
    ShapeStore store;
    auto id = store.add("Cyl", makeCylinder(5.0, 10.0));
    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);

    bool found_circle = false;
    for(int i = 1; i <= entry->edgeMap.Extent(); ++i) {
        const auto& edge = TopoDS::Edge(entry->edgeMap(i));
        auto info = OpenGeoLab::Geometry::extractEdgeInfo(
            static_cast<uint32_t>(i), edge);
        if(info.curveType == "circle") {
            found_circle = true;
            CHECK(info.radius.has_value());
            CHECK(info.radius.value() == doctest::Approx(5.0).epsilon(0.01));
            CHECK(info.center.has_value());
            // Circumference = 2*pi*r
            CHECK(info.length == doctest::Approx(2.0 * M_PI * 5.0).epsilon(0.1));
            break;
        }
    }
    CHECK(found_circle);
}

TEST_CASE("extractVertexInfo returns vertex position") {
    ShapeStore store;
    auto id = store.add("Box", makeBox(10.0, 20.0, 30.0));
    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);
    REQUIRE(entry->vertexMap.Extent() == 8);

    // Collect all vertex positions — all should be corners of [0,10]×[0,20]×[0,30]
    for(int i = 1; i <= entry->vertexMap.Extent(); ++i) {
        const auto& vtx = TopoDS::Vertex(entry->vertexMap(i));
        auto info = OpenGeoLab::Geometry::extractVertexInfo(
            static_cast<uint32_t>(i), vtx);
        CHECK(info.localId == static_cast<uint32_t>(i));
        // Each coordinate should be at one end of the box
        CHECK((info.position[0] == doctest::Approx(0.0) ||
               info.position[0] == doctest::Approx(10.0)));
        CHECK((info.position[1] == doctest::Approx(0.0) ||
               info.position[1] == doctest::Approx(20.0)));
        CHECK((info.position[2] == doctest::Approx(0.0) ||
               info.position[2] == doctest::Approx(30.0)));
    }
}

TEST_CASE("toJson(FaceInfo) produces expected keys") {
    OpenGeoLab::Geometry::FaceInfo info;
    info.localId = 3;
    info.surfaceType = "plane";
    info.center = {1.0, 2.0, 3.0};
    info.normal = {0.0, 0.0, 1.0};
    info.area = 42.0;
    auto j = OpenGeoLab::Geometry::toJson(info);
    CHECK(j["localId"] == 3);
    CHECK(j["surfaceType"] == "plane");
    CHECK(j["area"] == doctest::Approx(42.0));
    CHECK(j["center"].is_array());
    CHECK(j["normal"].is_array());
    CHECK_FALSE(j.contains("axis"));
    CHECK_FALSE(j.contains("radius"));
}

TEST_CASE("toJson(FaceInfo) includes axis and radius when present") {
    OpenGeoLab::Geometry::FaceInfo info;
    info.localId = 1;
    info.surfaceType = "cylinder";
    info.center = {0, 0, 0};
    info.normal = {1, 0, 0};
    info.axis = std::array<double, 3>{0.0, 0.0, 1.0};
    info.radius = 5.0;
    info.area = 100.0;
    auto j = OpenGeoLab::Geometry::toJson(info);
    CHECK(j.contains("axis"));
    CHECK(j.contains("radius"));
    CHECK(j["radius"] == doctest::Approx(5.0));
}

TEST_CASE("toJson(EdgeInfo) produces expected keys") {
    OpenGeoLab::Geometry::EdgeInfo info;
    info.localId = 2;
    info.curveType = "line";
    info.start = {0, 0, 0};
    info.end = {10, 0, 0};
    info.length = 10.0;
    auto j = OpenGeoLab::Geometry::toJson(info);
    CHECK(j["localId"] == 2);
    CHECK(j["curveType"] == "line");
    CHECK(j["length"] == doctest::Approx(10.0));
    CHECK_FALSE(j.contains("center"));
    CHECK_FALSE(j.contains("radius"));
}

TEST_CASE("toJson(VertexInfo) produces expected keys") {
    OpenGeoLab::Geometry::VertexInfo info;
    info.localId = 5;
    info.position = {1.0, 2.0, 3.0};
    auto j = OpenGeoLab::Geometry::toJson(info);
    CHECK(j["localId"] == 5);
    CHECK(j["position"][0] == doctest::Approx(1.0));
    CHECK(j["position"][1] == doctest::Approx(2.0));
    CHECK(j["position"][2] == doctest::Approx(3.0));
}

TEST_CASE("buildEdgeToFaceAdjacency for a box") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);

    auto adj = OpenGeoLab::Geometry::buildEdgeToFaceAdjacency(*entry);
    // A box has 12 edges, each shared by exactly 2 faces
    CHECK(adj.size() == 12);
    for(const auto& [edgeId, faceIds] : adj) {
        CHECK(faceIds.size() == 2);
    }
}

TEST_CASE("buildVertexToEdgeAdjacency for a box") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);

    auto adj = OpenGeoLab::Geometry::buildVertexToEdgeAdjacency(*entry);
    // A box has 8 vertices, each touching exactly 3 edges
    CHECK(adj.size() == 8);
    for(const auto& [vtxId, edgeIds] : adj) {
        CHECK(edgeIds.size() == 3);
    }
}

TEST_CASE("buildFaceToEdgeAdjacency for a box") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);

    auto adj = OpenGeoLab::Geometry::buildFaceToEdgeAdjacency(*entry);
    // A box has 6 faces, each bounded by exactly 4 edges
    CHECK(adj.size() == 6);
    for(const auto& [faceId, edgeIds] : adj) {
        CHECK(edgeIds.size() == 4);
    }
}

TEST_CASE("computeSubShapeBounds for a box face") {
    ShapeStore store;
    auto id = store.add("Box", makeBox(10.0, 20.0, 30.0));
    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);

    // Get any face and check bounds are within the shape bounds
    const auto& face = entry->faceMap(1);
    auto bounds = OpenGeoLab::Geometry::computeSubShapeBounds(face);
    REQUIRE(bounds.has_value());
    const auto& [mn, mx] = *bounds;
    // Each coordinate of min should be >= 0 and max should be <= shape extent
    for(int i = 0; i < 3; ++i) {
        CHECK(mn[i] >= doctest::Approx(-0.01));
    }
    CHECK(mx[0] <= doctest::Approx(10.01));
    CHECK(mx[1] <= doctest::Approx(20.01));
    CHECK(mx[2] <= doctest::Approx(30.01));
}

} // TEST_SUITE
```

- [ ] **Step 2: Add test to CMakeLists**

In `src/libs/geometry/CMakeLists.txt`, add to the test SOURCES list:

```cmake
        test/topology_utils_test.cpp
```

(Add after the last existing test source, e.g. after `test/tessellator_test.cpp`.)

- [ ] **Step 3: Build test — verify it fails (linker errors)**

Run:

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_geometry_test --parallel 4
```

Expected: **Linker errors** — `extractFaceInfo`, `extractEdgeInfo`, etc. are declared
but not defined. This confirms TDD red phase.

---

### Task 3: Implement `topology_utils.cpp`

**Files:**
- Create: `src/libs/geometry/src/topology_utils.cpp`

- [ ] **Step 1: Create the implementation file**

```cpp
/**
 * @file topology_utils.cpp
 * @brief OCC topology extraction implementations
 */

#include <opengeolab/geometry/topology_utils.hpp>

#include <opengeolab/geometry/shape_entry.hpp>

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <BRepGProp_Face.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp_Explorer.hxx>

#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>

namespace OpenGeoLab::Geometry {

namespace {

std::string_view surfaceTypeName(GeomAbs_SurfaceType type) {
    switch(type) {
    case GeomAbs_Plane: return "plane";
    case GeomAbs_Cylinder: return "cylinder";
    case GeomAbs_Cone: return "cone";
    case GeomAbs_Sphere: return "sphere";
    case GeomAbs_Torus: return "torus";
    case GeomAbs_BSplineSurface: return "bspline";
    default: return "other";
    }
}

std::string_view curveTypeName(GeomAbs_CurveType type) {
    switch(type) {
    case GeomAbs_Line: return "line";
    case GeomAbs_Circle: return "circle";
    case GeomAbs_Ellipse: return "ellipse";
    case GeomAbs_Parabola: return "parabola";
    case GeomAbs_Hyperbola: return "hyperbola";
    case GeomAbs_BSplineCurve: return "bspline";
    default: return "other";
    }
}

std::array<double, 3> toArray(const gp_Pnt& p) {
    return {p.X(), p.Y(), p.Z()};
}

std::array<double, 3> toArray(const gp_Dir& d) {
    return {d.X(), d.Y(), d.Z()};
}

} // anonymous namespace

// ── Extraction ───────────────────────────────────────────────

FaceInfo extractFaceInfo(uint32_t local_id, const TopoDS_Face& face) {
    FaceInfo info;
    info.localId = local_id;

    BRepAdaptor_Surface adaptor(face);
    info.surfaceType = std::string(surfaceTypeName(adaptor.GetType()));

    // Area
    GProp_GProps props;
    BRepGProp::SurfaceProperties(face, props);
    info.area = props.Mass();

    // Center and normal at parametric midpoint
    BRepGProp_Face face_props(face);
    Standard_Real u1 = 0;
    Standard_Real u2 = 0;
    Standard_Real v1 = 0;
    Standard_Real v2 = 0;
    face_props.Bounds(u1, u2, v1, v2);
    gp_Pnt center;
    gp_Vec normal;
    face_props.Normal((u1 + u2) / 2.0, (v1 + v2) / 2.0, center, normal);
    info.center = toArray(center);

    constexpr double kEpsilon = 1e-10;
    if(normal.Magnitude() > kEpsilon) {
        normal.Normalize();
        info.normal = {normal.X(), normal.Y(), normal.Z()};
    }

    // Type-specific axis/radius
    switch(adaptor.GetType()) {
    case GeomAbs_Cylinder: {
        auto cyl = adaptor.Cylinder();
        info.axis = toArray(cyl.Axis().Direction());
        info.radius = cyl.Radius();
        break;
    }
    case GeomAbs_Cone: {
        auto cone = adaptor.Cone();
        info.axis = toArray(cone.Axis().Direction());
        info.radius = cone.RefRadius();
        break;
    }
    case GeomAbs_Sphere: {
        auto sph = adaptor.Sphere();
        info.radius = sph.Radius();
        break;
    }
    case GeomAbs_Torus: {
        auto tor = adaptor.Torus();
        info.axis = toArray(tor.Axis().Direction());
        info.radius = tor.MajorRadius();
        break;
    }
    default: break;
    }

    return info;
}

EdgeInfo extractEdgeInfo(uint32_t local_id, const TopoDS_Edge& edge) {
    EdgeInfo info;
    info.localId = local_id;

    BRepAdaptor_Curve adaptor(edge);
    info.curveType = std::string(curveTypeName(adaptor.GetType()));

    // Length
    GProp_GProps props;
    BRepGProp::LinearProperties(edge, props);
    info.length = props.Mass();

    // Endpoints
    gp_Pnt start_pt;
    gp_Pnt end_pt;
    adaptor.D0(adaptor.FirstParameter(), start_pt);
    adaptor.D0(adaptor.LastParameter(), end_pt);
    info.start = toArray(start_pt);
    info.end = toArray(end_pt);

    // Type-specific center/radius
    switch(adaptor.GetType()) {
    case GeomAbs_Circle: {
        auto circle = adaptor.Circle();
        info.center = toArray(circle.Location());
        info.radius = circle.Radius();
        break;
    }
    case GeomAbs_Ellipse: {
        auto ellipse = adaptor.Ellipse();
        info.center = toArray(ellipse.Location());
        info.radius = ellipse.MajorRadius();
        break;
    }
    default: break;
    }

    return info;
}

VertexInfo extractVertexInfo(uint32_t local_id, const TopoDS_Vertex& vertex) {
    return {local_id, toArray(BRep_Tool::Pnt(vertex))};
}

// ── JSON Serialisation ───────────────────────────────────────

nlohmann::json toJson(const FaceInfo& info) {
    nlohmann::json j = {{"localId", info.localId},
                        {"surfaceType", info.surfaceType},
                        {"center", info.center},
                        {"normal", info.normal},
                        {"area", info.area}};
    if(info.axis) {
        j["axis"] = *info.axis;
    }
    if(info.radius) {
        j["radius"] = *info.radius;
    }
    return j;
}

nlohmann::json toJson(const EdgeInfo& info) {
    nlohmann::json j = {{"localId", info.localId},
                        {"curveType", info.curveType},
                        {"start", info.start},
                        {"end", info.end},
                        {"length", info.length}};
    if(info.center) {
        j["center"] = *info.center;
    }
    if(info.radius) {
        j["radius"] = *info.radius;
    }
    return j;
}

nlohmann::json toJson(const VertexInfo& info) {
    return {{"localId", info.localId}, {"position", info.position}};
}

// ── Adjacency ────────────────────────────────────────────────

std::unordered_map<uint32_t, std::vector<uint32_t>>
buildEdgeToFaceAdjacency(const ShapeEntry& entry) {
    TopTools_IndexedDataMapOfShapeListOfShape map;
    TopExp::MapShapesAndAncestors(entry.shape, TopAbs_EDGE, TopAbs_FACE, map);

    std::unordered_map<uint32_t, std::vector<uint32_t>> result;
    for(int i = 1; i <= entry.edgeMap.Extent(); ++i) {
        const auto& edge = entry.edgeMap(i);
        if(!map.Contains(edge)) {
            continue;
        }
        const auto& faces = map.FindFromKey(edge);
        std::vector<uint32_t> ids;
        for(TopTools_ListIteratorOfListOfShape it(faces); it.More(); it.Next()) {
            int idx = entry.faceMap.FindIndex(it.Value());
            if(idx > 0) {
                ids.push_back(static_cast<uint32_t>(idx));
            }
        }
        if(!ids.empty()) {
            result[static_cast<uint32_t>(i)] = std::move(ids);
        }
    }
    return result;
}

std::unordered_map<uint32_t, std::vector<uint32_t>>
buildVertexToEdgeAdjacency(const ShapeEntry& entry) {
    TopTools_IndexedDataMapOfShapeListOfShape map;
    TopExp::MapShapesAndAncestors(entry.shape, TopAbs_VERTEX, TopAbs_EDGE, map);

    std::unordered_map<uint32_t, std::vector<uint32_t>> result;
    for(int i = 1; i <= entry.vertexMap.Extent(); ++i) {
        const auto& vtx = entry.vertexMap(i);
        if(!map.Contains(vtx)) {
            continue;
        }
        const auto& edges = map.FindFromKey(vtx);
        std::vector<uint32_t> ids;
        for(TopTools_ListIteratorOfListOfShape it(edges); it.More(); it.Next()) {
            int idx = entry.edgeMap.FindIndex(it.Value());
            if(idx > 0) {
                ids.push_back(static_cast<uint32_t>(idx));
            }
        }
        if(!ids.empty()) {
            result[static_cast<uint32_t>(i)] = std::move(ids);
        }
    }
    return result;
}

std::unordered_map<uint32_t, std::vector<uint32_t>>
buildFaceToEdgeAdjacency(const ShapeEntry& entry) {
    std::unordered_map<uint32_t, std::vector<uint32_t>> result;
    for(int i = 1; i <= entry.faceMap.Extent(); ++i) {
        const auto& face = entry.faceMap(i);
        std::vector<uint32_t> edge_ids;
        for(TopExp_Explorer exp(face, TopAbs_EDGE); exp.More(); exp.Next()) {
            int idx = entry.edgeMap.FindIndex(exp.Current());
            if(idx > 0) {
                edge_ids.push_back(static_cast<uint32_t>(idx));
            }
        }
        std::sort(edge_ids.begin(), edge_ids.end());
        edge_ids.erase(std::unique(edge_ids.begin(), edge_ids.end()), edge_ids.end());
        if(!edge_ids.empty()) {
            result[static_cast<uint32_t>(i)] = std::move(edge_ids);
        }
    }
    return result;
}

// ── Bounding Box ─────────────────────────────────────────────

std::optional<std::pair<std::array<double, 3>, std::array<double, 3>>>
computeSubShapeBounds(const TopoDS_Shape& sub_shape) {
    Bnd_Box box;
    BRepBndLib::Add(sub_shape, box);
    if(box.IsVoid()) {
        return std::nullopt;
    }
    Standard_Real x_min = 0;
    Standard_Real y_min = 0;
    Standard_Real z_min = 0;
    Standard_Real x_max = 0;
    Standard_Real y_max = 0;
    Standard_Real z_max = 0;
    box.Get(x_min, y_min, z_min, x_max, y_max, z_max);
    return std::pair{std::array{x_min, y_min, z_min}, std::array{x_max, y_max, z_max}};
}

} // namespace OpenGeoLab::Geometry
```

- [ ] **Step 2: Add source to CMakeLists**

In `src/libs/geometry/CMakeLists.txt`, add to `geometry_sources`:

```cmake
    src/topology_utils.cpp
```

(Add after the last existing source, e.g. after `src/tessellator.cpp`.)

- [ ] **Step 3: Build and run tests**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_geometry_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_geometry_test --output-on-failure
```

Expected: All topology_utils tests **pass**.

- [ ] **Step 4: Commit**

```bash
git add src/libs/geometry/include/opengeolab/geometry/topology_utils.hpp \
        src/libs/geometry/src/topology_utils.cpp \
        src/libs/geometry/test/topology_utils_test.cpp \
        src/libs/geometry/CMakeLists.txt
git commit -m "feat(geometry): add topology extraction utilities

Shared value types (FaceInfo, EdgeInfo, VertexInfo) and free functions
for OCC topology extraction: surface/curve type detection, area/length
computation, center/normal calculation, adjacency map building, and
sub-shape bounding boxes.  Foundation for DescribeTopologyAction,
QueryEntityInfoAction, and enhanced CaptureViewportAction.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```
