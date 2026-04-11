# 3D Model & Camera State Export — Part 3 of 4

> Part file: Scene camera actions — LookAtEntityAction and BestViewForEntityAction.
>
> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Create two scene-module actions that point the camera at a specific
topology entity, giving the LLM semantic camera control.

**Prerequisites:** Part 1 (topology_utils) and Part 2 (geometry actions) must
be complete and committed.

**Design spec:** `docs/superpowers/specs/2026-04-10-3d-model-camera-state-export-design.md`

---

## File Structure (Part 3 scope)

| Action | Path | Purpose |
|--------|------|---------|
| Modify | `src/libs/scene/include/opengeolab/scene/scene_graph.hpp` | Add `ShapeStore*` pointer + getter/setter |
| Modify | `src/libs/scene/src/scene_graph.cpp` | Implement setter (trivial) |
| Modify | `src/libs/scene/src/scene_module.cpp` | Set ShapeStore in initBridge + register new actions |
| Create | `src/libs/scene/src/entity_camera_utils.hpp` | Internal header: shared camera target computation |
| Create | `src/libs/scene/src/entity_camera_utils.cpp` | Implementation of shared helpers |
| Create | `src/libs/scene/include/opengeolab/scene/look_at_entity_action.hpp` | Action header |
| Create | `src/libs/scene/src/look_at_entity_action.cpp` | Action implementation |
| Create | `src/libs/scene/include/opengeolab/scene/best_view_for_entity_action.hpp` | Action header |
| Create | `src/libs/scene/src/best_view_for_entity_action.cpp` | Action implementation |
| Create | `src/libs/scene/test/camera_entity_actions_test.cpp` | Tests for both camera actions |
| Modify | `src/libs/scene/CMakeLists.txt` | Add new files |

---

### Task 7: Add `ShapeStore*` pointer to SceneGraph

Camera actions need access to `Geometry::ShapeStore` to resolve entity
bounding boxes and normals. Since `ShapeStore` is only available after
`initBridge()`, we store a nullable pointer in SceneGraph and set it during
bridge initialization.

**Files:**
- Modify: `src/libs/scene/include/opengeolab/scene/scene_graph.hpp`
- Modify: `src/libs/scene/src/scene_graph.cpp`
- Modify: `src/libs/scene/src/scene_module.cpp:108-114`

- [ ] **Step 1: Add forward declaration and members to scene_graph.hpp**

After the existing `#include` block (before `namespace OpenGeoLab::Scene`),
add the forward declaration:

```cpp
namespace OpenGeoLab::Geometry {
class ShapeStore;
} // namespace OpenGeoLab::Geometry
```

Inside the `SceneGraph` class, after the `viewportState()` accessors (line 182),
add:

```cpp
    /**
     * @brief Optional ShapeStore pointer, set via initBridge.
     *
     * May be nullptr before initBridge is called. Camera actions that need
     * topology data must null-check before use.
     */
    void setShapeStore(Geometry::ShapeStore* store);

    /** @brief Get ShapeStore pointer (may be nullptr). */
    [[nodiscard]] Geometry::ShapeStore* shapeStore() const;
```

In the private section, after `TopologyIndex m_topologyIndex;` (line 222), add:

```cpp
    Geometry::ShapeStore* m_shapeStore{nullptr};
```

- [ ] **Step 2: Implement setter and getter in scene_graph.cpp**

At the end of `scene_graph.cpp`, before the closing namespace brace, add:

```cpp
void SceneGraph::setShapeStore(Geometry::ShapeStore* store) { m_shapeStore = store; }
Geometry::ShapeStore* SceneGraph::shapeStore() const { return m_shapeStore; }
```

- [ ] **Step 3: Set ShapeStore in initBridge**

In `src/libs/scene/src/scene_module.cpp`, modify `initBridge` to set the
ShapeStore pointer before creating the bridge:

```cpp
void SceneModule::initBridge(Geometry::ShapeStore& store) {
    if(m_bridge) {
        return; // Already initialized.
    }
    m_sceneGraph.setShapeStore(&store);
    m_bridge = std::make_unique<GeometrySceneBridge>(m_sceneGraph, store);
    LOG_INFO("SceneModule: GeometrySceneBridge created successfully");
}
```

- [ ] **Step 4: Build scene library to verify**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 4
```

Expected: Build succeeds with no errors.

- [ ] **Step 5: Commit**

```bash
git add src/libs/scene/include/opengeolab/scene/scene_graph.hpp \
        src/libs/scene/src/scene_graph.cpp \
        src/libs/scene/src/scene_module.cpp
git commit -m "refactor(scene): add ShapeStore pointer to SceneGraph

Camera actions need access to geometry ShapeStore for entity lookup.
The pointer is set during initBridge() and may be null before that.
Actions must null-check before use.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 8: Create internal `entity_camera_utils`

Shared helper functions for computing camera target point, viewing direction,
and up vector from an entity reference.  Used by both LookAtEntityAction and
BestViewForEntityAction.

These files live in `src/` (not `include/`) — internal to the scene library.

**Files:**
- Create: `src/libs/scene/src/entity_camera_utils.hpp`
- Create: `src/libs/scene/src/entity_camera_utils.cpp`

- [ ] **Step 1: Create the internal header**

```cpp
/**
 * @file entity_camera_utils.hpp
 * @brief Internal helpers for computing camera targets from topology entities
 */

#pragma once

#include <opengeolab/scene/bounding_box3d.hpp>

#include <glm/glm.hpp>

#include <cstdint>
#include <optional>
#include <string_view>

namespace OpenGeoLab::Geometry {
class ShapeStore;
} // namespace OpenGeoLab::Geometry

namespace OpenGeoLab::Scene {

/**
 * @brief Result of computing a camera target from a topology entity.
 *
 * Contains the point to look at, the ideal viewing direction (from
 * entity toward camera), and the entity's local bounding box.
 */
struct EntityCameraTarget {
    glm::vec3 center{0};              ///< Point to look at (face center / edge midpoint / vertex)
    glm::vec3 direction{0, 0, 1};     ///< Viewing direction (outward from entity, normalized)
    BoundingBox3D entityBounds;        ///< Entity local bounding box
};

/**
 * @brief Compute camera target for a topology entity.
 *
 * @param store ShapeStore to look up the entity.
 * @param shape_id Shape identifier.
 * @param entity_type "face", "edge", or "vertex".
 * @param local_id 1-based index within the shape.
 * @return Target info, or nullopt with error string.
 *
 * Direction logic (per design spec):
 * - Face: outward face normal
 * - Edge: average normal of adjacent faces (fallback: world +Z)
 * - Vertex: average normal of adjacent faces via adjacent edges
 */
std::optional<EntityCameraTarget> computeEntityCameraTarget(
    const Geometry::ShapeStore& store,
    uint32_t shape_id,
    std::string_view entity_type,
    uint32_t local_id,
    std::string* out_error = nullptr);

/**
 * @brief Choose the best world-axis up vector for a viewing direction.
 *
 * Returns world Y (0,1,0) or Z (0,0,1), whichever is more orthogonal
 * to the viewing direction.
 */
glm::vec3 chooseUpVector(const glm::vec3& direction);

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 2: Implement the helpers**

```cpp
/**
 * @file entity_camera_utils.cpp
 * @brief Entity-to-camera target computation for semantic camera commands
 */

#include "entity_camera_utils.hpp"

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/geometry/topology_utils.hpp>

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS.hxx>

#include <glm/geometric.hpp>

#include <cmath>
#include <set>

namespace OpenGeoLab::Scene {

namespace {

/// Convert a 3-element double array to glm::vec3.
glm::vec3 toVec3(const std::array<double, 3>& a) {
    return {static_cast<float>(a[0]), static_cast<float>(a[1]), static_cast<float>(a[2])};
}

/// Compute average face normal for a set of face localIds.
glm::vec3 averageFaceNormal(const Geometry::ShapeEntry& entry,
                            const std::set<uint32_t>& face_ids) {
    glm::vec3 sum{0};
    for(auto fid : face_ids) {
        if(fid >= 1 && fid <= static_cast<uint32_t>(entry.faceMap.Extent())) {
            auto info =
                Geometry::extractFaceInfo(fid, TopoDS::Face(entry.faceMap(static_cast<int>(fid))));
            sum += toVec3(info.normal);
        }
    }
    const float len = glm::length(sum);
    if(len > 1.0e-6F) {
        return sum / len;
    }
    return {0.0F, 0.0F, 1.0F}; // fallback
}

/// Build BoundingBox3D from an OCC sub-shape.
BoundingBox3D boundsFromShape(const TopoDS_Shape& shape) {
    BoundingBox3D bb;
    Bnd_Box occ_box;
    BRepBndLib::Add(shape, occ_box);
    if(!occ_box.IsVoid()) {
        Standard_Real xn = 0;
        Standard_Real yn = 0;
        Standard_Real zn = 0;
        Standard_Real xx = 0;
        Standard_Real yx = 0;
        Standard_Real zx = 0;
        occ_box.Get(xn, yn, zn, xx, yx, zx);
        bb.expand(glm::vec3{static_cast<float>(xn), static_cast<float>(yn),
                            static_cast<float>(zn)});
        bb.expand(glm::vec3{static_cast<float>(xx), static_cast<float>(yx),
                            static_cast<float>(zx)});
    }
    return bb;
}

} // anonymous namespace

std::optional<EntityCameraTarget> computeEntityCameraTarget(
    const Geometry::ShapeStore& store,
    uint32_t shape_id,
    std::string_view entity_type,
    uint32_t local_id,
    std::string* out_error) {
    auto parsed_type = Core::parseEntityType(entity_type);
    if(!parsed_type) {
        if(out_error) {
            *out_error = "Invalid entityType '" + std::string(entity_type) + "'.";
        }
        return std::nullopt;
    }

    const auto* entry = store.find(shape_id);
    if(!entry) {
        if(out_error) {
            *out_error = "Unknown shapeId.";
        }
        return std::nullopt;
    }

    auto sub = store.subShape(shape_id, *parsed_type, local_id);
    if(sub.IsNull()) {
        if(out_error) {
            *out_error = "localId out of range for this entityType.";
        }
        return std::nullopt;
    }

    EntityCameraTarget result;
    result.entityBounds = boundsFromShape(sub);

    switch(*parsed_type) {
    case Core::EntityType::GeoFace: {
        auto info = Geometry::extractFaceInfo(
            local_id, TopoDS::Face(entry->faceMap(static_cast<int>(local_id))));
        result.center = toVec3(info.center);
        result.direction = toVec3(info.normal);
        // Ensure direction is normalized
        float len = glm::length(result.direction);
        if(len > 1.0e-6F) {
            result.direction /= len;
        } else {
            result.direction = {0, 0, 1};
        }
        break;
    }

    case Core::EntityType::GeoEdge: {
        auto info = Geometry::extractEdgeInfo(
            local_id, TopoDS::Edge(entry->edgeMap(static_cast<int>(local_id))));
        // Midpoint of edge
        result.center = (toVec3(info.start) + toVec3(info.end)) * 0.5F;

        // Average normal of adjacent faces
        auto edge_to_face = Geometry::buildEdgeToFaceAdjacency(*entry);
        std::set<uint32_t> adj_faces;
        if(auto it = edge_to_face.find(local_id); it != edge_to_face.end()) {
            adj_faces.insert(it->second.begin(), it->second.end());
        }
        result.direction = averageFaceNormal(*entry, adj_faces);
        break;
    }

    case Core::EntityType::GeoVertex: {
        auto info = Geometry::extractVertexInfo(
            local_id, TopoDS::Vertex(entry->vertexMap(static_cast<int>(local_id))));
        result.center = toVec3(info.position);

        // Average normal of faces adjacent through edges
        auto vtx_to_edge = Geometry::buildVertexToEdgeAdjacency(*entry);
        auto edge_to_face = Geometry::buildEdgeToFaceAdjacency(*entry);
        std::set<uint32_t> adj_faces;
        if(auto vit = vtx_to_edge.find(local_id); vit != vtx_to_edge.end()) {
            for(auto eid : vit->second) {
                if(auto eit = edge_to_face.find(eid); eit != edge_to_face.end()) {
                    adj_faces.insert(eit->second.begin(), eit->second.end());
                }
            }
        }
        result.direction = averageFaceNormal(*entry, adj_faces);
        break;
    }

    default:
        if(out_error) {
            *out_error = "Only 'face', 'edge', and 'vertex' are supported.";
        }
        return std::nullopt;
    }

    return result;
}

glm::vec3 chooseUpVector(const glm::vec3& direction) {
    const glm::vec3 world_y{0.0F, 1.0F, 0.0F};
    const glm::vec3 world_z{0.0F, 0.0F, 1.0F};
    // Pick the axis more orthogonal to direction (smaller |dot|)
    if(std::abs(glm::dot(direction, world_y)) < std::abs(glm::dot(direction, world_z))) {
        return world_y;
    }
    return world_z;
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 3: Build to verify compilation**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 4
```

Expected: Build succeeds.

- [ ] **Step 4: Commit**

```bash
git add src/libs/scene/src/entity_camera_utils.hpp \
        src/libs/scene/src/entity_camera_utils.cpp
git commit -m "feat(scene): add entity_camera_utils for semantic camera targets

Internal helpers that compute camera target point, viewing direction,
and bounding box from a topology entity (face/edge/vertex). Shared
by LookAtEntityAction and BestViewForEntityAction.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 9: Create `LookAtEntityAction`

Points camera at an entity, keeping the current viewing distance.

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/look_at_entity_action.hpp`
- Create: `src/libs/scene/src/look_at_entity_action.cpp`

- [ ] **Step 1: Create the header**

```cpp
/**
 * @file look_at_entity_action.hpp
 * @brief LookAtEntityAction — point camera at a topology entity
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SceneGraph;

/**
 * @brief Points the camera at a specific topology entity.
 *
 * Keeps the current viewing distance and places the camera along
 * the entity's outward direction (face normal, or average face normal
 * for edges/vertices).
 */
class OPENGEOLAB_SCENE_EXPORT LookAtEntityAction final : public Core::IAction {
public:
    explicit LookAtEntityAction(SceneGraph& graph);
    ~LookAtEntityAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"look_at_entity"};

private:
    SceneGraph& m_graph;
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 2: Implement the action**

```cpp
/**
 * @file look_at_entity_action.cpp
 * @brief LookAtEntityAction — point camera at a topology entity
 */

#include <opengeolab/scene/look_at_entity_action.hpp>

#include <opengeolab/scene/scene_graph.hpp>

#include "entity_camera_utils.hpp"

namespace OpenGeoLab::Scene {

LookAtEntityAction::LookAtEntityAction(SceneGraph& graph) : m_graph(graph) {}
LookAtEntityAction::~LookAtEntityAction() = default;

nlohmann::json LookAtEntityAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description",
         "Point the camera at a specific face/edge/vertex, keeping "
         "the current viewing distance."},
        {"params",
         {{"shapeId",
           {{"type", "integer"},
            {"required", true},
            {"description", "Shape identifier."}}},
          {"entityType",
           {{"type", "string"},
            {"required", true},
            {"description", "'face', 'edge', or 'vertex'."}}},
          {"localId",
           {{"type", "integer"},
            {"required", true},
            {"description", "1-based local index."}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}}},
          {"action", {{"type", "string"}}},
          {"camera",
           {{"type", "object"},
            {"description",
             "Resulting camera state: {position, target, up}."}}}}}};
}

nlohmann::json LookAtEntityAction::execute(const nlohmann::json& param,
                                           const Core::ProgressCallback& progress) {
    // Validate parameters
    if(!param.contains("shapeId") || !param["shapeId"].is_number()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'shapeId'."}};
    }
    if(!param.contains("entityType") || !param["entityType"].is_string()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'entityType'."}};
    }
    if(!param.contains("localId") || !param["localId"].is_number()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'localId'."}};
    }

    auto* store = m_graph.shapeStore();
    if(!store) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "ShapeStore not available (bridge not initialized)."}};
    }

    std::string error;
    auto target_opt = computeEntityCameraTarget(
        *store,
        param["shapeId"].get<uint32_t>(),
        param["entityType"].get<std::string>(),
        param["localId"].get<uint32_t>(),
        &error);

    if(!target_opt) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", error}};
    }

    if(progress) {
        progress(0.5, "Computing camera...");
    }

    // Keep current viewing distance
    auto cam = m_graph.viewportState().camera();
    const float dist = cam.distance();

    cam.target = target_opt->center;
    cam.position = cam.target + target_opt->direction * dist;
    cam.up = chooseUpVector(target_opt->direction);
    cam.updateClipping();

    m_graph.viewportState().setCamera(cam);

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true},
            {"action", ACTION_NAME},
            {"camera",
             {{"position", {cam.position.x, cam.position.y, cam.position.z}},
              {"target", {cam.target.x, cam.target.y, cam.target.z}},
              {"up", {cam.up.x, cam.up.y, cam.up.z}}}}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 3: Commit header and implementation**

```bash
git add src/libs/scene/include/opengeolab/scene/look_at_entity_action.hpp \
        src/libs/scene/src/look_at_entity_action.cpp
git commit -m "feat(scene): add LookAtEntityAction

Points the camera at a face/edge/vertex keeping the current viewing
distance. Uses entity normal (or average adjacent face normal) as the
viewing direction.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 10: Create `BestViewForEntityAction`

Computes optimal camera position to view an entity, including auto-distance.

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/best_view_for_entity_action.hpp`
- Create: `src/libs/scene/src/best_view_for_entity_action.cpp`

- [ ] **Step 1: Create the header**

```cpp
/**
 * @file best_view_for_entity_action.hpp
 * @brief BestViewForEntityAction — optimal camera for viewing an entity
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SceneGraph;

/**
 * @brief Computes and applies the optimal camera to view a topology entity.
 *
 * Auto-computes viewing distance from the entity's bounding box diagonal
 * multiplied by a padding factor.
 */
class OPENGEOLAB_SCENE_EXPORT BestViewForEntityAction final : public Core::IAction {
public:
    explicit BestViewForEntityAction(SceneGraph& graph);
    ~BestViewForEntityAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"best_view_for_entity"};

    static constexpr float DEFAULT_PADDING = 1.5F;

private:
    SceneGraph& m_graph;
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 2: Implement the action**

```cpp
/**
 * @file best_view_for_entity_action.cpp
 * @brief BestViewForEntityAction — optimal camera for viewing an entity
 */

#include <opengeolab/scene/best_view_for_entity_action.hpp>

#include <opengeolab/scene/scene_graph.hpp>

#include "entity_camera_utils.hpp"

#include <algorithm>

namespace OpenGeoLab::Scene {

BestViewForEntityAction::BestViewForEntityAction(SceneGraph& graph) : m_graph(graph) {}
BestViewForEntityAction::~BestViewForEntityAction() = default;

nlohmann::json BestViewForEntityAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description",
         "Compute and apply the optimal camera to view a face/edge/vertex, "
         "auto-fitting distance to the entity's bounding box."},
        {"params",
         {{"shapeId",
           {{"type", "integer"},
            {"required", true},
            {"description", "Shape identifier."}}},
          {"entityType",
           {{"type", "string"},
            {"required", true},
            {"description", "'face', 'edge', or 'vertex'."}}},
          {"localId",
           {{"type", "integer"},
            {"required", true},
            {"description", "1-based local index."}}},
          {"padding",
           {{"type", "number"},
            {"required", false},
            {"description",
             "Distance multiplier on bbox diagonal (default 1.5). "
             "Larger values show more context."}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}}},
          {"action", {{"type", "string"}}},
          {"camera",
           {{"type", "object"},
            {"description", "Resulting camera state: {position, target, up}."}}},
          {"entityBounds",
           {{"type", "object"},
            {"description", "Entity AABB: {min: [x,y,z], max: [x,y,z]}."}}}}}};
}

nlohmann::json BestViewForEntityAction::execute(const nlohmann::json& param,
                                                const Core::ProgressCallback& progress) {
    // Validate parameters
    if(!param.contains("shapeId") || !param["shapeId"].is_number()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'shapeId'."}};
    }
    if(!param.contains("entityType") || !param["entityType"].is_string()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'entityType'."}};
    }
    if(!param.contains("localId") || !param["localId"].is_number()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'localId'."}};
    }

    float padding = DEFAULT_PADDING;
    if(param.contains("padding") && param["padding"].is_number()) {
        padding = param["padding"].get<float>();
        padding = std::max(padding, 0.1F); // Clamp to sane minimum
    }

    auto* store = m_graph.shapeStore();
    if(!store) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "ShapeStore not available (bridge not initialized)."}};
    }

    std::string error;
    auto target_opt = computeEntityCameraTarget(
        *store,
        param["shapeId"].get<uint32_t>(),
        param["entityType"].get<std::string>(),
        param["localId"].get<uint32_t>(),
        &error);

    if(!target_opt) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", error}};
    }

    if(progress) {
        progress(0.5, "Computing camera...");
    }

    // Distance = entity bbox diagonal × padding
    const float diag = target_opt->entityBounds.isValid()
                           ? target_opt->entityBounds.diagonal()
                           : 10.0F; // fallback for degenerate entities (points)
    const float dist = std::max(diag * padding, 1.0F);

    CameraState cam;
    cam.target = target_opt->center;
    cam.position = cam.target + target_opt->direction * dist;
    cam.up = chooseUpVector(target_opt->direction);
    cam.updateClipping();

    m_graph.viewportState().setCamera(cam);

    if(progress) {
        progress(1.0, "Done");
    }

    // Build response
    nlohmann::json result = {
        {"ok", true},
        {"action", ACTION_NAME},
        {"camera",
         {{"position", {cam.position.x, cam.position.y, cam.position.z}},
          {"target", {cam.target.x, cam.target.y, cam.target.z}},
          {"up", {cam.up.x, cam.up.y, cam.up.z}}}}};

    if(target_opt->entityBounds.isValid()) {
        auto& bb = target_opt->entityBounds;
        result["entityBounds"] = {{"min", {bb.min.x, bb.min.y, bb.min.z}},
                                  {"max", {bb.max.x, bb.max.y, bb.max.z}}};
    }

    return result;
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 3: Commit**

```bash
git add src/libs/scene/include/opengeolab/scene/best_view_for_entity_action.hpp \
        src/libs/scene/src/best_view_for_entity_action.cpp
git commit -m "feat(scene): add BestViewForEntityAction

Computes optimal camera position to view a face/edge/vertex. Distance
is auto-calculated from entity bbox diagonal × padding factor.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 11: Write tests for both camera entity actions

**Files:**
- Create: `src/libs/scene/test/camera_entity_actions_test.cpp`

- [ ] **Step 1: Create the test file**

```cpp
/**
 * @file camera_entity_actions_test.cpp
 * @brief Tests for LookAtEntityAction and BestViewForEntityAction
 */

#include <opengeolab/scene/best_view_for_entity_action.hpp>
#include <opengeolab/scene/look_at_entity_action.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <BRepPrimAPI_MakeBox.hxx>

#include <doctest/doctest.h>

#include <glm/geometric.hpp>

#include <cmath>

using OpenGeoLab::Geometry::ShapeStore;
using OpenGeoLab::Scene::BestViewForEntityAction;
using OpenGeoLab::Scene::LookAtEntityAction;
using OpenGeoLab::Scene::SceneGraph;

/// Create a SceneGraph with ShapeStore configured.
struct TestFixture {
    ShapeStore store;
    SceneGraph graph;
    uint32_t box_id{0};

    TestFixture() {
        box_id = store.add("Box", BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape());
        graph.setShapeStore(&store);
    }
};

TEST_SUITE("LookAtEntityAction") {

TEST_CASE("describe returns expected schema") {
    SceneGraph graph;
    LookAtEntityAction action(graph);
    auto desc = action.describe();
    CHECK(desc["name"] == "look_at_entity");
    CHECK(desc["params"].contains("shapeId"));
    CHECK(desc["params"].contains("entityType"));
    CHECK(desc["params"].contains("localId"));
}

TEST_CASE("look at face updates camera") {
    TestFixture fix;
    LookAtEntityAction action(fix.graph);

    // Get initial camera distance
    auto cam_before = fix.graph.viewportState().camera();
    float dist_before = cam_before.distance();

    auto result = action.execute(
        {{"shapeId", fix.box_id}, {"entityType", "face"}, {"localId", 1}}, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["action"] == "look_at_entity");
    CHECK(result.contains("camera"));
    CHECK(result["camera"].contains("position"));
    CHECK(result["camera"].contains("target"));
    CHECK(result["camera"].contains("up"));

    // Camera distance should be preserved (within tolerance)
    auto cam_after = fix.graph.viewportState().camera();
    CHECK(cam_after.distance() == doctest::Approx(dist_before).epsilon(0.01));
}

TEST_CASE("look at edge updates camera") {
    TestFixture fix;
    LookAtEntityAction action(fix.graph);

    auto result = action.execute(
        {{"shapeId", fix.box_id}, {"entityType", "edge"}, {"localId", 1}}, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result.contains("camera"));
}

TEST_CASE("look at vertex updates camera") {
    TestFixture fix;
    LookAtEntityAction action(fix.graph);

    auto result = action.execute(
        {{"shapeId", fix.box_id}, {"entityType", "vertex"}, {"localId", 1}}, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result.contains("camera"));
}

TEST_CASE("error when ShapeStore not set") {
    SceneGraph graph; // no shapeStore set
    LookAtEntityAction action(graph);

    auto result = action.execute(
        {{"shapeId", 1}, {"entityType", "face"}, {"localId", 1}}, nullptr);
    CHECK(result["ok"] == false);
    CHECK(result["error"].get<std::string>().find("ShapeStore") != std::string::npos);
}

TEST_CASE("error with unknown shapeId") {
    TestFixture fix;
    LookAtEntityAction action(fix.graph);

    auto result = action.execute(
        {{"shapeId", 999}, {"entityType", "face"}, {"localId", 1}}, nullptr);
    CHECK(result["ok"] == false);
}

TEST_CASE("error with missing params") {
    TestFixture fix;
    LookAtEntityAction action(fix.graph);

    CHECK(action.execute(nlohmann::json::object(), nullptr)["ok"] == false);
    CHECK(action.execute({{"shapeId", 1}}, nullptr)["ok"] == false);
    CHECK(action.execute({{"shapeId", 1}, {"entityType", "face"}}, nullptr)["ok"] == false);
}

TEST_CASE("up vector is orthogonal to viewing direction") {
    TestFixture fix;
    LookAtEntityAction action(fix.graph);

    auto result = action.execute(
        {{"shapeId", fix.box_id}, {"entityType", "face"}, {"localId", 1}}, nullptr);
    REQUIRE(result["ok"] == true);

    auto cam = fix.graph.viewportState().camera();
    glm::vec3 view_dir = glm::normalize(cam.target - cam.position);
    float dot = std::abs(glm::dot(view_dir, cam.up));
    CHECK(dot < 0.1F); // nearly orthogonal
}

} // TEST_SUITE

TEST_SUITE("BestViewForEntityAction") {

TEST_CASE("describe returns expected schema") {
    SceneGraph graph;
    BestViewForEntityAction action(graph);
    auto desc = action.describe();
    CHECK(desc["name"] == "best_view_for_entity");
    CHECK(desc["params"].contains("padding"));
}

TEST_CASE("best view for face auto-computes distance") {
    TestFixture fix;
    BestViewForEntityAction action(fix.graph);

    auto result = action.execute(
        {{"shapeId", fix.box_id}, {"entityType", "face"}, {"localId", 1}}, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["action"] == "best_view_for_entity");
    CHECK(result.contains("camera"));
    CHECK(result.contains("entityBounds"));
    CHECK(result["entityBounds"].contains("min"));
    CHECK(result["entityBounds"].contains("max"));
}

TEST_CASE("padding affects distance") {
    TestFixture fix;
    BestViewForEntityAction action(fix.graph);

    auto result_small = action.execute(
        {{"shapeId", fix.box_id}, {"entityType", "face"}, {"localId", 1}, {"padding", 1.0}},
        nullptr);
    auto result_large = action.execute(
        {{"shapeId", fix.box_id}, {"entityType", "face"}, {"localId", 1}, {"padding", 3.0}},
        nullptr);

    REQUIRE(result_small["ok"] == true);
    REQUIRE(result_large["ok"] == true);

    // Larger padding should produce greater distance from target
    auto pos_small = result_small["camera"]["position"];
    auto tgt_small = result_small["camera"]["target"];
    auto pos_large = result_large["camera"]["position"];
    auto tgt_large = result_large["camera"]["target"];

    auto dist = [](const nlohmann::json& p, const nlohmann::json& t) {
        float dx = p[0].get<float>() - t[0].get<float>();
        float dy = p[1].get<float>() - t[1].get<float>();
        float dz = p[2].get<float>() - t[2].get<float>();
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    };

    CHECK(dist(pos_large, tgt_large) > dist(pos_small, tgt_small));
}

TEST_CASE("best view for edge works") {
    TestFixture fix;
    BestViewForEntityAction action(fix.graph);

    auto result = action.execute(
        {{"shapeId", fix.box_id}, {"entityType", "edge"}, {"localId", 1}}, nullptr);
    CHECK(result["ok"] == true);
}

TEST_CASE("best view for vertex works") {
    TestFixture fix;
    BestViewForEntityAction action(fix.graph);

    auto result = action.execute(
        {{"shapeId", fix.box_id}, {"entityType", "vertex"}, {"localId", 1}}, nullptr);
    CHECK(result["ok"] == true);
}

TEST_CASE("error when ShapeStore not set") {
    SceneGraph graph;
    BestViewForEntityAction action(graph);

    auto result = action.execute(
        {{"shapeId", 1}, {"entityType", "face"}, {"localId", 1}}, nullptr);
    CHECK(result["ok"] == false);
}

TEST_CASE("default padding is 1.5") {
    CHECK(BestViewForEntityAction::DEFAULT_PADDING == doctest::Approx(1.5F));
}

} // TEST_SUITE
```

- [ ] **Step 2: Build and run tests — verify pass**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_scene_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_scene_test --output-on-failure
```

Expected: All camera entity action tests **pass**.

- [ ] **Step 3: Commit tests**

```bash
git add src/libs/scene/test/camera_entity_actions_test.cpp
git commit -m "test(scene): add tests for LookAtEntityAction and BestViewForEntityAction

Covers face/edge/vertex targeting, distance preservation, padding effect,
error cases, up vector orthogonality, and ShapeStore availability.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 12: Register actions in SceneModule and update CMakeLists

**Files:**
- Modify: `src/libs/scene/src/scene_module.cpp`
- Modify: `src/libs/scene/CMakeLists.txt`

- [ ] **Step 1: Add includes and registration to scene_module.cpp**

Add includes after existing action includes (after line 30):

```cpp
#include <opengeolab/scene/best_view_for_entity_action.hpp>
#include <opengeolab/scene/look_at_entity_action.hpp>
```

Add registration calls after line 59 (`registerAction<CaptureViewportAction>`):

```cpp
    registerAction<LookAtEntityAction>(std::ref(m_sceneGraph));
    registerAction<BestViewForEntityAction>(std::ref(m_sceneGraph));
```

- [ ] **Step 2: Update scene CMakeLists.txt**

Add to `scene_public_headers` (before the closing paren on line 38):

```cmake
    include/opengeolab/scene/look_at_entity_action.hpp
    include/opengeolab/scene/best_view_for_entity_action.hpp
```

Add to `scene_sources` (before the closing paren on line 71):

```cmake
    src/entity_camera_utils.hpp
    src/entity_camera_utils.cpp
    src/look_at_entity_action.cpp
    src/best_view_for_entity_action.cpp
```

Add to test SOURCES (after the existing test entries, before `LINKS`):

```cmake
        test/camera_entity_actions_test.cpp
```

- [ ] **Step 3: Build and run ALL scene tests**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_scene_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_scene_test --output-on-failure
```

Expected: All tests pass, including the new camera entity action tests.

- [ ] **Step 4: Commit**

```bash
git add src/libs/scene/src/scene_module.cpp \
        src/libs/scene/CMakeLists.txt
git commit -m "feat(scene): register LookAtEntityAction and BestViewForEntityAction

Adds two semantic camera commands to the scene module. Updates
CMakeLists with entity_camera_utils, both action sources/headers,
and test file.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```
