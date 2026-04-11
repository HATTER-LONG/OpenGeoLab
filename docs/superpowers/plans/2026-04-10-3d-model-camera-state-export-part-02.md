# 3D Model & Camera State Export — Part 2 of 4

> Part file: Geometry topology actions — DescribeTopologyAction and QueryEntityInfoAction.
>
> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Create two geometry-module actions that expose OCC topology to the
LLM via the existing Command/Action JSON protocol.

**Prerequisites:** Part 1 (topology_utils) must be complete and committed.

**Design spec:** `docs/superpowers/specs/2026-04-10-3d-model-camera-state-export-design.md`

---

## File Structure (Part 2 scope)

| Action | Path | Purpose |
|--------|------|---------|
| Create | `src/libs/geometry/include/opengeolab/geometry/describe_topology_action.hpp` | Action header |
| Create | `src/libs/geometry/src/describe_topology_action.cpp` | Action implementation |
| Create | `src/libs/geometry/test/describe_topology_action_test.cpp` | Action tests |
| Create | `src/libs/geometry/include/opengeolab/geometry/query_entity_info_action.hpp` | Action header |
| Create | `src/libs/geometry/src/query_entity_info_action.cpp` | Action implementation |
| Create | `src/libs/geometry/test/query_entity_info_action_test.cpp` | Action tests |
| Modify | `src/libs/geometry/src/geometry_module.cpp` | Register new actions |
| Modify | `src/libs/geometry/CMakeLists.txt` | Add new files |
| Modify | `src/libs/geometry/test/geometry_module_test.cpp:27` | Update action count (12 → 14) |

---

### Task 4: Create `DescribeTopologyAction`

**Files:**
- Create: `src/libs/geometry/include/opengeolab/geometry/describe_topology_action.hpp`
- Create: `src/libs/geometry/src/describe_topology_action.cpp`
- Create: `src/libs/geometry/test/describe_topology_action_test.cpp`

- [ ] **Step 1: Create the header file**

```cpp
/**
 * @file describe_topology_action.hpp
 * @brief DescribeTopologyAction — shape topology overview for LLM context
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Returns a structured overview of a shape's topology.
 *
 * Extracts face/edge/vertex counts and per-entity summaries with type,
 * coordinates, dimensions, and bounding box.  Designed to give an LLM
 * enough context to reason about a 3D model.
 *
 * Depends on topology_utils for OCC extraction.
 */
class OPENGEOLAB_GEOMETRY_EXPORT DescribeTopologyAction final : public Core::IAction {
public:
    explicit DescribeTopologyAction(ShapeStore& store);
    ~DescribeTopologyAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"describe_topology"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
```

- [ ] **Step 2: Create the test file (red phase)**

```cpp
/**
 * @file describe_topology_action_test.cpp
 * @brief Unit tests for DescribeTopologyAction
 */

#include <opengeolab/geometry/describe_topology_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>

#include <doctest/doctest.h>

using OpenGeoLab::Geometry::DescribeTopologyAction;
using OpenGeoLab::Geometry::ShapeStore;

static TopoDS_Shape makeBox(double w = 10.0, double h = 10.0, double d = 10.0) {
    return BRepPrimAPI_MakeBox(w, h, d).Shape();
}

TEST_SUITE("DescribeTopologyAction") {

TEST_CASE("describe returns expected schema") {
    ShapeStore store;
    DescribeTopologyAction action(store);
    auto desc = action.describe();
    CHECK(desc["name"] == "describe_topology");
    CHECK(desc.contains("params"));
    CHECK(desc["params"].contains("shapeId"));
}

TEST_CASE("execute with valid shapeId returns topology overview") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    DescribeTopologyAction action(store);

    auto result = action.execute({{"shapeId", id}}, nullptr);
    CHECK(result["ok"] == true);
    CHECK(result["action"] == "describe_topology");
    CHECK(result["shapeId"] == id);
    CHECK(result["shapeName"] == "Box");

    // Counts
    CHECK(result["counts"]["faces"] == 6);
    CHECK(result["counts"]["edges"] == 12);
    CHECK(result["counts"]["vertices"] == 8);

    // Bounding box
    CHECK(result.contains("boundingBox"));
    auto bb = result["boundingBox"];
    CHECK(bb["min"][0] == doctest::Approx(0.0).epsilon(0.01));
    CHECK(bb["max"][0] == doctest::Approx(10.0).epsilon(0.01));

    // Faces array
    CHECK(result["faces"].is_array());
    CHECK(result["faces"].size() == 6);
    for(const auto& f : result["faces"]) {
        CHECK(f.contains("localId"));
        CHECK(f.contains("surfaceType"));
        CHECK(f["surfaceType"] == "plane");
        CHECK(f.contains("center"));
        CHECK(f.contains("normal"));
        CHECK(f.contains("area"));
    }

    // Edges array
    CHECK(result["edges"].is_array());
    CHECK(result["edges"].size() == 12);
    for(const auto& e : result["edges"]) {
        CHECK(e.contains("localId"));
        CHECK(e.contains("curveType"));
        CHECK(e["curveType"] == "line");
        CHECK(e.contains("length"));
    }
}

TEST_CASE("execute with cylinder includes cylinder face info") {
    ShapeStore store;
    auto id = store.add("Cyl", BRepPrimAPI_MakeCylinder(5.0, 10.0).Shape());
    DescribeTopologyAction action(store);

    auto result = action.execute({{"shapeId", id}}, nullptr);
    CHECK(result["ok"] == true);

    bool found_cylinder = false;
    for(const auto& f : result["faces"]) {
        if(f["surfaceType"] == "cylinder") {
            found_cylinder = true;
            CHECK(f.contains("axis"));
            CHECK(f.contains("radius"));
            CHECK(f["radius"].get<double>() == doctest::Approx(5.0).epsilon(0.01));
        }
    }
    CHECK(found_cylinder);
}

TEST_CASE("execute with unknown shapeId returns error") {
    ShapeStore store;
    DescribeTopologyAction action(store);
    auto result = action.execute({{"shapeId", 999}}, nullptr);
    CHECK(result["ok"] == false);
    CHECK(result.contains("error"));
}

TEST_CASE("execute without shapeId returns error") {
    ShapeStore store;
    DescribeTopologyAction action(store);
    auto result = action.execute(nlohmann::json::object(), nullptr);
    CHECK(result["ok"] == false);
}

TEST_CASE("execute reports progress") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    DescribeTopologyAction action(store);

    std::vector<double> progress_values;
    auto progress_cb = [&](double p, const std::string&) {
        progress_values.push_back(p);
        return true;
    };
    action.execute({{"shapeId", id}}, progress_cb);
    CHECK_FALSE(progress_values.empty());
    CHECK(progress_values.back() == doctest::Approx(1.0));
}

} // TEST_SUITE
```

- [ ] **Step 3: Build test — verify linker failure (red phase)**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_geometry_test --parallel 4
```

Expected: Linker errors for `DescribeTopologyAction` symbols.

- [ ] **Step 4: Implement the action**

```cpp
/**
 * @file describe_topology_action.cpp
 * @brief DescribeTopologyAction — shape topology overview for LLM context
 */

#include <opengeolab/geometry/describe_topology_action.hpp>

#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/geometry/topology_utils.hpp>

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS.hxx>

namespace OpenGeoLab::Geometry {

DescribeTopologyAction::DescribeTopologyAction(ShapeStore& store) : m_store(store) {}
DescribeTopologyAction::~DescribeTopologyAction() = default;

nlohmann::json DescribeTopologyAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description",
         "Return a structured overview of a shape's topology: face/edge/vertex "
         "counts and per-entity summary with type, coordinates, and dimensions."},
        {"params",
         {{"shapeId",
           {{"type", "integer"},
            {"required", true},
            {"description", "Shape identifier to inspect."}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "true on success."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"shapeId", {{"type", "integer"}, {"description", "Queried shape identifier."}}},
          {"shapeName", {{"type", "string"}, {"description", "Registered shape name."}}},
          {"boundingBox",
           {{"type", "object"},
            {"description", "AABB as {min: [x,y,z], max: [x,y,z]}."}}},
          {"counts",
           {{"type", "object"},
            {"description", "Topology counts: {faces, edges, vertices}."}}},
          {"faces",
           {{"type", "array"},
            {"description",
             "Per-face summary: localId, surfaceType, center, normal, area, "
             "and optional axis/radius for curved faces."}}},
          {"edges",
           {{"type", "array"},
            {"description",
             "Per-edge summary: localId, curveType, start, end, length, "
             "and optional center/radius for curved edges."}}}}}};
}

nlohmann::json DescribeTopologyAction::execute(const nlohmann::json& param,
                                               const Core::ProgressCallback& progress) {
    if(!param.contains("shapeId") || !param["shapeId"].is_number()) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "Missing or invalid 'shapeId' parameter."}};
    }

    const auto shape_id = param["shapeId"].get<uint32_t>();
    const auto* entry = m_store.find(shape_id);
    if(!entry) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Unknown shapeId."}};
    }

    if(progress) {
        progress(0.0, "Extracting topology...");
    }

    // Bounding box
    Bnd_Box bbox;
    BRepBndLib::Add(entry->shape, bbox);
    nlohmann::json bb_json;
    if(!bbox.IsVoid()) {
        Standard_Real x_min = 0;
        Standard_Real y_min = 0;
        Standard_Real z_min = 0;
        Standard_Real x_max = 0;
        Standard_Real y_max = 0;
        Standard_Real z_max = 0;
        bbox.Get(x_min, y_min, z_min, x_max, y_max, z_max);
        bb_json = {{"min", {x_min, y_min, z_min}}, {"max", {x_max, y_max, z_max}}};
    }

    // Faces
    nlohmann::json faces_json = nlohmann::json::array();
    for(int i = 1; i <= entry->faceMap.Extent(); ++i) {
        faces_json.push_back(
            toJson(extractFaceInfo(static_cast<uint32_t>(i), TopoDS::Face(entry->faceMap(i)))));
    }

    if(progress) {
        progress(0.5, "Extracting edges...");
    }

    // Edges
    nlohmann::json edges_json = nlohmann::json::array();
    for(int i = 1; i <= entry->edgeMap.Extent(); ++i) {
        edges_json.push_back(
            toJson(extractEdgeInfo(static_cast<uint32_t>(i), TopoDS::Edge(entry->edgeMap(i)))));
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true},
            {"action", ACTION_NAME},
            {"shapeId", shape_id},
            {"shapeName", entry->name},
            {"boundingBox", std::move(bb_json)},
            {"counts",
             {{"faces", entry->faceMap.Extent()},
              {"edges", entry->edgeMap.Extent()},
              {"vertices", entry->vertexMap.Extent()}}},
            {"faces", std::move(faces_json)},
            {"edges", std::move(edges_json)}};
}

} // namespace OpenGeoLab::Geometry
```

- [ ] **Step 5: Build and run tests — verify pass (green phase)**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_geometry_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_geometry_test --output-on-failure
```

Expected: All DescribeTopologyAction tests **pass**.

- [ ] **Step 6: Commit**

```bash
git add src/libs/geometry/include/opengeolab/geometry/describe_topology_action.hpp \
        src/libs/geometry/src/describe_topology_action.cpp \
        src/libs/geometry/test/describe_topology_action_test.cpp
git commit -m "feat(geometry): add DescribeTopologyAction

Returns structured topology overview for a shape: face/edge/vertex counts,
per-face summary (surfaceType, center, normal, area, axis/radius),
per-edge summary (curveType, start, end, length, center/radius), and
bounding box.  Uses topology_utils for OCC extraction.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 5: Create `QueryEntityInfoAction`

**Files:**
- Create: `src/libs/geometry/include/opengeolab/geometry/query_entity_info_action.hpp`
- Create: `src/libs/geometry/src/query_entity_info_action.cpp`
- Create: `src/libs/geometry/test/query_entity_info_action_test.cpp`

- [ ] **Step 1: Create the header file**

```cpp
/**
 * @file query_entity_info_action.hpp
 * @brief QueryEntityInfoAction — detailed info for a single face/edge/vertex
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Returns detailed information about a single face, edge, or vertex.
 *
 * Includes type-specific properties, bounding box, and adjacency lists.
 * Designed to give an LLM detailed context about a specific entity.
 */
class OPENGEOLAB_GEOMETRY_EXPORT QueryEntityInfoAction final : public Core::IAction {
public:
    explicit QueryEntityInfoAction(ShapeStore& store);
    ~QueryEntityInfoAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"query_entity_info"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
```

- [ ] **Step 2: Create the test file (red phase)**

```cpp
/**
 * @file query_entity_info_action_test.cpp
 * @brief Unit tests for QueryEntityInfoAction
 */

#include <opengeolab/geometry/query_entity_info_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>

#include <doctest/doctest.h>

using OpenGeoLab::Geometry::QueryEntityInfoAction;
using OpenGeoLab::Geometry::ShapeStore;

static TopoDS_Shape makeBox(double w = 10.0, double h = 10.0, double d = 10.0) {
    return BRepPrimAPI_MakeBox(w, h, d).Shape();
}

TEST_SUITE("QueryEntityInfoAction") {

TEST_CASE("describe returns expected schema") {
    ShapeStore store;
    QueryEntityInfoAction action(store);
    auto desc = action.describe();
    CHECK(desc["name"] == "query_entity_info");
    CHECK(desc["params"].contains("shapeId"));
    CHECK(desc["params"].contains("entityType"));
    CHECK(desc["params"].contains("localId"));
}

TEST_CASE("query face returns face info with adjacency") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    QueryEntityInfoAction action(store);

    auto result = action.execute(
        {{"shapeId", id}, {"entityType", "face"}, {"localId", 1}}, nullptr);
    CHECK(result["ok"] == true);
    CHECK(result["action"] == "query_entity_info");
    CHECK(result["shapeId"] == id);
    CHECK(result["entityType"] == "face");
    CHECK(result["localId"] == 1);
    CHECK(result["surfaceType"] == "plane");
    CHECK(result.contains("center"));
    CHECK(result.contains("normal"));
    CHECK(result.contains("area"));
    CHECK(result.contains("boundingBox"));

    // Adjacency: a box face has 4 edges
    CHECK(result.contains("adjacentEdges"));
    CHECK(result["adjacentEdges"].size() == 4);
    // Adjacent faces: each box face shares edges with 4 other faces
    CHECK(result.contains("adjacentFaces"));
    CHECK(result["adjacentFaces"].size() == 4);
}

TEST_CASE("query edge returns edge info with adjacency") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    QueryEntityInfoAction action(store);

    auto result = action.execute(
        {{"shapeId", id}, {"entityType", "edge"}, {"localId", 1}}, nullptr);
    CHECK(result["ok"] == true);
    CHECK(result["entityType"] == "edge");
    CHECK(result["curveType"] == "line");
    CHECK(result.contains("start"));
    CHECK(result.contains("end"));
    CHECK(result.contains("length"));
    CHECK(result.contains("boundingBox"));

    // A box edge is shared by exactly 2 faces
    CHECK(result.contains("adjacentFaces"));
    CHECK(result["adjacentFaces"].size() == 2);
}

TEST_CASE("query vertex returns vertex info with adjacency") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    QueryEntityInfoAction action(store);

    auto result = action.execute(
        {{"shapeId", id}, {"entityType", "vertex"}, {"localId", 1}}, nullptr);
    CHECK(result["ok"] == true);
    CHECK(result["entityType"] == "vertex");
    CHECK(result.contains("position"));

    // A box vertex touches exactly 3 edges
    CHECK(result.contains("adjacentEdges"));
    CHECK(result["adjacentEdges"].size() == 3);
}

TEST_CASE("query with unknown shapeId returns error") {
    ShapeStore store;
    QueryEntityInfoAction action(store);
    auto result = action.execute(
        {{"shapeId", 999}, {"entityType", "face"}, {"localId", 1}}, nullptr);
    CHECK(result["ok"] == false);
}

TEST_CASE("query with out-of-range localId returns error") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    QueryEntityInfoAction action(store);

    auto result = action.execute(
        {{"shapeId", id}, {"entityType", "face"}, {"localId", 999}}, nullptr);
    CHECK(result["ok"] == false);
}

TEST_CASE("query with invalid entityType returns error") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    QueryEntityInfoAction action(store);

    auto result = action.execute(
        {{"shapeId", id}, {"entityType", "unknown"}, {"localId", 1}}, nullptr);
    CHECK(result["ok"] == false);
}

TEST_CASE("query with missing required params returns error") {
    ShapeStore store;
    QueryEntityInfoAction action(store);

    CHECK(action.execute(nlohmann::json::object(), nullptr)["ok"] == false);
    CHECK(action.execute({{"shapeId", 0}}, nullptr)["ok"] == false);
    CHECK(action.execute({{"shapeId", 0}, {"entityType", "face"}}, nullptr)["ok"] == false);
}

} // TEST_SUITE
```

- [ ] **Step 3: Implement the action**

```cpp
/**
 * @file query_entity_info_action.cpp
 * @brief QueryEntityInfoAction — detailed info for a single face/edge/vertex
 */

#include <opengeolab/geometry/query_entity_info_action.hpp>

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/geometry/topology_utils.hpp>

#include <TopoDS.hxx>

#include <algorithm>
#include <set>

namespace OpenGeoLab::Geometry {

QueryEntityInfoAction::QueryEntityInfoAction(ShapeStore& store) : m_store(store) {}
QueryEntityInfoAction::~QueryEntityInfoAction() = default;

nlohmann::json QueryEntityInfoAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description",
         "Return detailed information about a single face, edge, or vertex, "
         "including type-specific properties, bounding box, and adjacency."},
        {"params",
         {{"shapeId",
           {{"type", "integer"},
            {"required", true},
            {"description", "Shape identifier."}}},
          {"entityType",
           {{"type", "string"},
            {"required", true},
            {"description",
             "Entity type: 'face', 'edge', or 'vertex'."}}},
          {"localId",
           {{"type", "integer"},
            {"required", true},
            {"description", "1-based local index within the shape."}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "true on success."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"shapeId", {{"type", "integer"}}},
          {"entityType", {{"type", "string"}}},
          {"localId", {{"type", "integer"}}},
          {"boundingBox", {{"type", "object"}}},
          {"adjacentEdges",
           {{"type", "array"}, {"description", "For face/vertex: adjacent edge localIds."}}},
          {"adjacentFaces",
           {{"type", "array"}, {"description", "For face/edge: adjacent face localIds."}}}}}};
}

nlohmann::json QueryEntityInfoAction::execute(const nlohmann::json& param,
                                              const Core::ProgressCallback& progress) {
    // ── Validate parameters ──
    if(!param.contains("shapeId") || !param["shapeId"].is_number()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'shapeId'."}};
    }
    if(!param.contains("entityType") || !param["entityType"].is_string()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'entityType'."}};
    }
    if(!param.contains("localId") || !param["localId"].is_number()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'localId'."}};
    }

    const auto shape_id = param["shapeId"].get<uint32_t>();
    const auto type_str = param["entityType"].get<std::string>();
    const auto local_id = param["localId"].get<uint32_t>();

    const auto entity_type = Core::parseEntityType(type_str);
    if(!entity_type.has_value()) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "Invalid entityType '" + type_str + "'."}};
    }

    const auto* entry = m_store.find(shape_id);
    if(!entry) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Unknown shapeId."}};
    }

    // ── Retrieve sub-shape ──
    auto sub = m_store.subShape(shape_id, *entity_type, local_id);
    if(sub.IsNull()) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "localId out of range for this entityType."}};
    }

    if(progress) {
        progress(0.3, "Extracting info...");
    }

    nlohmann::json result = {{"ok", true},
                             {"action", ACTION_NAME},
                             {"shapeId", shape_id},
                             {"entityType", type_str},
                             {"localId", local_id}};

    // Bounding box
    auto bounds = computeSubShapeBounds(sub);
    if(bounds) {
        result["boundingBox"] = {{"min", bounds->first}, {"max", bounds->second}};
    }

    // ── Type-specific info + adjacency ──
    switch(*entity_type) {
    case Core::EntityType::GeoFace: {
        auto info = extractFaceInfo(local_id, TopoDS::Face(sub));
        result["surfaceType"] = info.surfaceType;
        result["center"] = info.center;
        result["normal"] = info.normal;
        result["area"] = info.area;
        if(info.axis) {
            result["axis"] = *info.axis;
        }
        if(info.radius) {
            result["radius"] = *info.radius;
        }

        // Adjacent edges (edges of this face)
        auto face_to_edge = buildFaceToEdgeAdjacency(*entry);
        if(auto it = face_to_edge.find(local_id); it != face_to_edge.end()) {
            result["adjacentEdges"] = it->second;
        } else {
            result["adjacentEdges"] = nlohmann::json::array();
        }

        // Adjacent faces (faces that share an edge with this face)
        auto edge_to_face = buildEdgeToFaceAdjacency(*entry);
        std::set<uint32_t> adj_faces;
        if(auto it = face_to_edge.find(local_id); it != face_to_edge.end()) {
            for(auto edge_id : it->second) {
                if(auto eit = edge_to_face.find(edge_id); eit != edge_to_face.end()) {
                    for(auto fid : eit->second) {
                        if(fid != local_id) {
                            adj_faces.insert(fid);
                        }
                    }
                }
            }
        }
        result["adjacentFaces"] = std::vector<uint32_t>(adj_faces.begin(), adj_faces.end());
        break;
    }

    case Core::EntityType::GeoEdge: {
        auto info = extractEdgeInfo(local_id, TopoDS::Edge(sub));
        result["curveType"] = info.curveType;
        result["start"] = info.start;
        result["end"] = info.end;
        result["length"] = info.length;
        if(info.center) {
            result["center"] = *info.center;
        }
        if(info.radius) {
            result["radius"] = *info.radius;
        }

        // Adjacent faces
        auto edge_to_face = buildEdgeToFaceAdjacency(*entry);
        if(auto it = edge_to_face.find(local_id); it != edge_to_face.end()) {
            result["adjacentFaces"] = it->second;
        } else {
            result["adjacentFaces"] = nlohmann::json::array();
        }
        break;
    }

    case Core::EntityType::GeoVertex: {
        auto info = extractVertexInfo(local_id, TopoDS::Vertex(sub));
        result["position"] = info.position;

        // Adjacent edges
        auto vtx_to_edge = buildVertexToEdgeAdjacency(*entry);
        if(auto it = vtx_to_edge.find(local_id); it != vtx_to_edge.end()) {
            result["adjacentEdges"] = it->second;
        } else {
            result["adjacentEdges"] = nlohmann::json::array();
        }
        break;
    }

    default:
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "Only 'face', 'edge', and 'vertex' are supported."}};
    }

    if(progress) {
        progress(1.0, "Done");
    }
    return result;
}

} // namespace OpenGeoLab::Geometry
```

- [ ] **Step 4: Build and run tests — verify pass**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_geometry_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_geometry_test --output-on-failure
```

Expected: All QueryEntityInfoAction tests **pass**.

- [ ] **Step 5: Commit**

```bash
git add src/libs/geometry/include/opengeolab/geometry/query_entity_info_action.hpp \
        src/libs/geometry/src/query_entity_info_action.cpp \
        src/libs/geometry/test/query_entity_info_action_test.cpp
git commit -m "feat(geometry): add QueryEntityInfoAction

Returns detailed info for a single face/edge/vertex including
type-specific properties, bounding box, and adjacency lists.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 6: Register actions in GeometryModule and update CMakeLists

**Files:**
- Modify: `src/libs/geometry/src/geometry_module.cpp`
- Modify: `src/libs/geometry/CMakeLists.txt`
- Modify: `src/libs/geometry/test/geometry_module_test.cpp:27`

- [ ] **Step 1: Add includes and registration to geometry_module.cpp**

Add includes (after the existing action includes):

```cpp
#include <opengeolab/geometry/describe_topology_action.hpp>
#include <opengeolab/geometry/query_entity_info_action.hpp>
```

Add registration calls (after the existing `registerAction<DeleteEntityAction>` line):

```cpp
    registerAction<DescribeTopologyAction>(std::ref(m_shapeStore));
    registerAction<QueryEntityInfoAction>(std::ref(m_shapeStore));
```

- [ ] **Step 2: Update geometry CMakeLists.txt**

Add to `geometry_public_headers` (after the existing entries, before the closing paren):

```cmake
    include/opengeolab/geometry/describe_topology_action.hpp
    include/opengeolab/geometry/query_entity_info_action.hpp
    include/opengeolab/geometry/topology_utils.hpp)
```

Add to `geometry_sources` (after the existing entries, before the closing paren):

```cmake
    src/describe_topology_action.cpp
    src/query_entity_info_action.cpp
    src/topology_utils.cpp)
```

Add to test SOURCES (after the existing entries):

```cmake
        test/describe_topology_action_test.cpp
        test/query_entity_info_action_test.cpp
        test/topology_utils_test.cpp
```

- [ ] **Step 3: Update action count in geometry_module_test.cpp**

In `src/libs/geometry/test/geometry_module_test.cpp`, line 27, change:

```cpp
    CHECK(desc["actions"].size() == 12);
```

to:

```cpp
    CHECK(desc["actions"].size() == 14);
```

- [ ] **Step 4: Build and run ALL geometry tests**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_geometry_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_geometry_test --output-on-failure
```

Expected: All tests pass, including the updated action count check.

- [ ] **Step 5: Commit**

```bash
git add src/libs/geometry/src/geometry_module.cpp \
        src/libs/geometry/CMakeLists.txt \
        src/libs/geometry/test/geometry_module_test.cpp
git commit -m "feat(geometry): register DescribeTopologyAction and QueryEntityInfoAction

Adds two new actions to the geometry module and updates the CMakeLists
to include topology_utils, describe_topology_action, and
query_entity_info_action sources, headers, and tests.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```
