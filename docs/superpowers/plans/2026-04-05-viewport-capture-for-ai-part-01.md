# Viewport Capture for AI — Part 1 of 3

> Part 文件：C++ CaptureViewportAction (metadata-only) — Tasks 1-3.
> 依赖关系：Part 1 独立可测试，Part 2 在 Part 1 基础上增加图片捕获，Part 3 接 Python/QML 集成。

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Create a C++ `CaptureViewportAction` that collects structured scene metadata (camera, visible shapes, selections, labels) and returns it as JSON. No image capture yet — that's Part 2.

**Architecture:** The action is registered in the scene module, receives a `const SceneGraph&` reference, and reads camera state from `ViewportState`, visible nodes via `forEachNode`, selection entities from `SelectionState`, and labels from `LabelManager`. Screen bounding box computation uses the MVP matrix and each node's world bounds.

**Tech Stack:** C++20 · nlohmann/json · glm · doctest

---

## File Map

| Action | Path | Responsibility |
|--------|------|----------------|
| Header | `src/libs/scene/include/opengeolab/scene/capture_viewport_action.hpp` | Action class, ACTION_NAME, constructor taking `const SceneGraph&` |
| Impl | `src/libs/scene/src/capture_viewport_action.cpp` | `describe()` + `execute()` with metadata collection |
| Test | `src/libs/scene/test/capture_viewport_action_test.cpp` | Unit tests for metadata JSON structure |
| CMake | `src/libs/scene/CMakeLists.txt` | Add header + source + test |
| Registration | `src/libs/scene/src/scene_module.cpp` | `registerAction<CaptureViewportAction>` |

---

### Task 1: Action Header + Stub Implementation

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/capture_viewport_action.hpp`
- Create: `src/libs/scene/src/capture_viewport_action.cpp`
- Modify: `src/libs/scene/CMakeLists.txt:1-67` (add header + source)

- [ ] **Step 1: Create the action header**

Create `src/libs/scene/include/opengeolab/scene/capture_viewport_action.hpp`:

```cpp
/**
 * @file capture_viewport_action.hpp
 * @brief CaptureViewportAction — capture viewport metadata for AI context
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SceneGraph;

/**
 * @brief Capture viewport metadata (camera, visible shapes, selections, labels).
 *
 * Returns structured JSON describing the current scene state.
 * When image capture is wired (Part 2), also returns a base64-encoded screenshot.
 */
class OPENGEOLAB_SCENE_EXPORT CaptureViewportAction final : public Core::IAction {
public:
    explicit CaptureViewportAction(const SceneGraph& graph);
    ~CaptureViewportAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"capture_viewport"};

private:
    const SceneGraph& m_graph;
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 2: Create the stub implementation**

Create `src/libs/scene/src/capture_viewport_action.cpp`:

```cpp
/**
 * @file capture_viewport_action.cpp
 * @brief CaptureViewportAction — collect scene metadata for AI context
 */

#include <opengeolab/scene/capture_viewport_action.hpp>

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/scene/label_manager.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/scene_node.hpp>
#include <opengeolab/scene/selection_state.hpp>
#include <opengeolab/scene/viewport_state.hpp>

#include <glm/glm.hpp>

namespace OpenGeoLab::Scene {

CaptureViewportAction::CaptureViewportAction(const SceneGraph& graph) : m_graph(graph) {}
CaptureViewportAction::~CaptureViewportAction() = default;

nlohmann::json CaptureViewportAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description",
         "Capture viewport metadata (camera, visible shapes, selections, labels). "
         "Returns structured JSON describing the current scene state for AI context."},
        {"params",
         {{"width",
           {{"type", "integer"},
            {"required", false},
            {"description", "Desired capture width in pixels (default 1024). "
                            "Used for screen bounding-box calculation."}}},
          {"height",
           {{"type", "integer"},
            {"required", false},
            {"description", "Desired capture height in pixels (default 768)."}}},
          {"includeMetadata",
           {{"type", "boolean"},
            {"required", false},
            {"description", "Whether to collect scene metadata (default true)."}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "true on success."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"metadata",
           {{"type", "object"},
            {"description",
             "Scene metadata: viewport, camera, visibleShapes, selections, labels."}}}}}};
}

nlohmann::json CaptureViewportAction::execute(const nlohmann::json& param,
                                               const Core::ProgressCallback& progress) {
    // Parse optional parameters
    const int width = param.value("width", 1024);
    const int height = param.value("height", 768);
    const bool includeMeta = param.value("includeMetadata", true);

    nlohmann::json result = {{"ok", true}, {"action", ACTION_NAME}};

    if(includeMeta) {
        result["metadata"] = nlohmann::json::object();
        // Will be populated in Task 2
    }

    if(progress) {
        progress(1.0, "Done");
    }
    return result;
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 3: Add header and source to CMakeLists.txt**

In `src/libs/scene/CMakeLists.txt`, add the header to `scene_public_headers` (after line 35, before the closing paren):

```cmake
    include/opengeolab/scene/capture_viewport_action.hpp
```

Add the source to `scene_sources` (after line 67, before the closing paren):

```cmake
    src/capture_viewport_action.cpp
```

- [ ] **Step 4: Build to verify compilation**

Run:
```bash
cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 4
```
Expected: BUILD SUCCESS

- [ ] **Step 5: Commit**

```bash
git add src/libs/scene/include/opengeolab/scene/capture_viewport_action.hpp
git add src/libs/scene/src/capture_viewport_action.cpp
git add src/libs/scene/CMakeLists.txt
git commit -m "feat(scene): add CaptureViewportAction stub with describe schema"
```

---

### Task 2: Implement Metadata Collection

**Files:**
- Modify: `src/libs/scene/src/capture_viewport_action.cpp` (fill execute() with metadata)

- [ ] **Step 1: Write test for metadata structure**

Create `src/libs/scene/test/capture_viewport_action_test.cpp`:

```cpp
/**
 * @file capture_viewport_action_test.cpp
 * @brief Tests for CaptureViewportAction metadata collection
 */

#include <opengeolab/scene/capture_viewport_action.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

using OpenGeoLab::Scene::CaptureViewportAction;
using OpenGeoLab::Scene::SceneGraph;

TEST_SUITE("CaptureViewportAction") {

TEST_CASE("describe returns expected schema") {
    SceneGraph graph;
    CaptureViewportAction action(graph);
    auto desc = action.describe();

    CHECK(desc["name"] == "capture_viewport");
    CHECK(desc.contains("description"));
    CHECK(desc.contains("params"));
    CHECK(desc["params"].contains("width"));
    CHECK(desc["params"].contains("height"));
    CHECK(desc["params"].contains("includeMetadata"));
    CHECK(desc.contains("returns"));
    CHECK(desc["returns"].contains("ok"));
    CHECK(desc["returns"].contains("action"));
    CHECK(desc["returns"].contains("metadata"));
}

TEST_CASE("execute on empty scene returns valid metadata") {
    SceneGraph graph;
    CaptureViewportAction action(graph);

    auto result = action.execute({}, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["action"] == "capture_viewport");
    REQUIRE(result.contains("metadata"));

    auto& meta = result["metadata"];
    CHECK(meta.contains("viewport"));
    CHECK(meta["viewport"]["width"] == 1024);
    CHECK(meta["viewport"]["height"] == 768);

    CHECK(meta.contains("camera"));
    CHECK(meta["camera"].contains("eye"));
    CHECK(meta["camera"].contains("target"));
    CHECK(meta["camera"].contains("up"));

    CHECK(meta.contains("visibleShapes"));
    CHECK(meta["visibleShapes"].is_array());
    CHECK(meta["visibleShapes"].empty());

    CHECK(meta.contains("selections"));
    CHECK(meta["selections"].is_array());
    CHECK(meta["selections"].empty());

    CHECK(meta.contains("labels"));
    CHECK(meta["labels"].is_array());
    CHECK(meta["labels"].empty());
}

TEST_CASE("execute with custom resolution") {
    SceneGraph graph;
    CaptureViewportAction action(graph);

    auto result = action.execute({{"width", 512}, {"height", 384}}, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["metadata"]["viewport"]["width"] == 512);
    CHECK(result["metadata"]["viewport"]["height"] == 384);
}

TEST_CASE("execute with includeMetadata=false omits metadata") {
    SceneGraph graph;
    CaptureViewportAction action(graph);

    auto result = action.execute({{"includeMetadata", false}}, nullptr);

    CHECK(result["ok"] == true);
    CHECK_FALSE(result.contains("metadata"));
}

TEST_CASE("camera state is captured from ViewportState") {
    SceneGraph graph;

    // Set a known camera position
    Scene::CameraState cam;
    cam.position = {10.0F, 20.0F, 30.0F};
    cam.target = {1.0F, 2.0F, 3.0F};
    cam.up = {0.0F, 1.0F, 0.0F};
    graph.viewportState().setCamera(cam);

    CaptureViewportAction action(graph);
    auto result = action.execute({}, nullptr);

    auto& camera_json = result["metadata"]["camera"];
    auto eye = camera_json["eye"];
    CHECK(eye[0].get<float>() == doctest::Approx(10.0F));
    CHECK(eye[1].get<float>() == doctest::Approx(20.0F));
    CHECK(eye[2].get<float>() == doctest::Approx(30.0F));

    auto target = camera_json["target"];
    CHECK(target[0].get<float>() == doctest::Approx(1.0F));
    CHECK(target[1].get<float>() == doctest::Approx(2.0F));
    CHECK(target[2].get<float>() == doctest::Approx(3.0F));
}

TEST_CASE("selections are captured from SelectionState") {
    SceneGraph graph;

    // Add a selection
    Core::EntityRef entity{1, Core::EntityType::GeoFace, 3};
    graph.selectionState().addSelection(entity);

    CaptureViewportAction action(graph);
    auto result = action.execute({}, nullptr);

    auto& sels = result["metadata"]["selections"];
    REQUIRE(sels.size() == 1);
    CHECK(sels[0]["shapeId"] == 1);
    CHECK(sels[0]["type"] == "GeoFace");
    CHECK(sels[0]["localId"] == 3);
}

TEST_CASE("labels are captured from LabelManager") {
    SceneGraph graph;

    // Add a label
    Core::EntityRef entity{2, Core::EntityType::GeoEdge, 5};
    graph.labelManager().addLabel(entity, "E:5");

    CaptureViewportAction action(graph);
    auto result = action.execute({}, nullptr);

    auto& labels = result["metadata"]["labels"];
    REQUIRE(labels.size() == 1);
    CHECK(labels[0]["text"] == "E:5");
    CHECK(labels[0]["entity"]["shapeId"] == 2);
    CHECK(labels[0]["entity"]["type"] == "GeoEdge");
    CHECK(labels[0]["entity"]["localId"] == 5);
}

TEST_CASE("visible nodes appear in visibleShapes") {
    SceneGraph graph;

    // Add a visible node with source info
    auto node_id = graph.addNode("Box_1");
    auto* node = graph.findNode(node_id);
    REQUIRE(node != nullptr);
    node->setSource("geometry", 1);
    node->setVisible(true);

    CaptureViewportAction action(graph);
    auto result = action.execute({}, nullptr);

    auto& shapes = result["metadata"]["visibleShapes"];
    REQUIRE(shapes.size() == 1);
    CHECK(shapes[0]["shapeId"] == 1);
    CHECK(shapes[0]["name"] == "Box_1");
}

TEST_CASE("invisible nodes are excluded from visibleShapes") {
    SceneGraph graph;

    auto node_id = graph.addNode("Hidden_1");
    auto* node = graph.findNode(node_id);
    REQUIRE(node != nullptr);
    node->setSource("geometry", 1);
    node->setVisible(false);

    CaptureViewportAction action(graph);
    auto result = action.execute({}, nullptr);

    CHECK(result["metadata"]["visibleShapes"].empty());
}

} // TEST_SUITE
```

- [ ] **Step 2: Add test file to CMakeLists.txt**

In `src/libs/scene/CMakeLists.txt`, add after line 100 (before `LINKS`):

```cmake
        test/capture_viewport_action_test.cpp
```

- [ ] **Step 3: Build test to verify it fails**

Run:
```bash
cmake --build build --config RelWithDebInfo --target opengeolab_scene_test --parallel 4
```

Then run:
```bash
ctest --test-dir build -C RelWithDebInfo -R opengeolab_scene_test --output-on-failure
```

Expected: Tests FAIL because `execute()` doesn't populate metadata yet.

- [ ] **Step 4: Implement full metadata collection in execute()**

Replace the `execute()` method in `src/libs/scene/src/capture_viewport_action.cpp`:

```cpp
nlohmann::json CaptureViewportAction::execute(const nlohmann::json& param,
                                               const Core::ProgressCallback& progress) {
    const int width = param.value("width", 1024);
    const int height = param.value("height", 768);
    const bool includeMeta = param.value("includeMetadata", true);

    nlohmann::json result = {{"ok", true}, {"action", ACTION_NAME}};

    if(!includeMeta) {
        if(progress) {
            progress(1.0, "Done");
        }
        return result;
    }

    // ── Camera state ──
    const auto cam = m_graph.viewportState().camera();
    const float aspect =
        (height > 0) ? static_cast<float>(width) / static_cast<float>(height) : 1.0F;
    const auto viewMat = cam.viewMatrix();
    const auto projMat = cam.projMatrix(aspect);
    const auto mvp = projMat * viewMat;

    nlohmann::json camera_json = {
        {"eye", {cam.position.x, cam.position.y, cam.position.z}},
        {"target", {cam.target.x, cam.target.y, cam.target.z}},
        {"up", {cam.up.x, cam.up.y, cam.up.z}}};

    // ── Visible shapes with screen bounding boxes ──
    nlohmann::json shapes_json = nlohmann::json::array();

    m_graph.forEachNode([&](const SceneNode& node) {
        if(!node.isVisible() || node.sourceType().empty()) {
            return;
        }

        nlohmann::json shape = {
            {"shapeId", node.sourceId()},
            {"name", std::string(node.name())}};

        // Screen bounding box from world AABB + MVP
        const auto bounds = node.worldBounds();
        if(bounds.isValid()) {
            // BoundingBox3D has no corners() — compute 8 corners manually
            const auto mn = bounds.min;
            const auto mx = bounds.max;
            const std::array<glm::vec3, 8> corners = {
                glm::vec3{mn.x, mn.y, mn.z}, glm::vec3{mx.x, mn.y, mn.z},
                glm::vec3{mn.x, mx.y, mn.z}, glm::vec3{mx.x, mx.y, mn.z},
                glm::vec3{mn.x, mn.y, mx.z}, glm::vec3{mx.x, mn.y, mx.z},
                glm::vec3{mn.x, mx.y, mx.z}, glm::vec3{mx.x, mx.y, mx.z},
            };
            float minX = std::numeric_limits<float>::max();
            float minY = std::numeric_limits<float>::max();
            float maxX = std::numeric_limits<float>::lowest();
            float maxY = std::numeric_limits<float>::lowest();
            bool anyVisible = false;

            for(const auto& corner : corners) {
                auto clip = mvp * glm::vec4(corner, 1.0F);
                if(clip.w <= 0.0F) {
                    continue; // behind camera
                }
                auto ndc = glm::vec3(clip) / clip.w;
                float sx = (ndc.x * 0.5F + 0.5F) * static_cast<float>(width);
                float sy =
                    (1.0F - (ndc.y * 0.5F + 0.5F)) * static_cast<float>(height);
                minX = std::min(minX, sx);
                minY = std::min(minY, sy);
                maxX = std::max(maxX, sx);
                maxY = std::max(maxY, sy);
                anyVisible = true;
            }

            if(anyVisible) {
                shape["screenBBox"] = {
                    {"x", static_cast<int>(minX)},
                    {"y", static_cast<int>(minY)},
                    {"w", static_cast<int>(maxX - minX)},
                    {"h", static_cast<int>(maxY - minY)}};
            }
        }

        shapes_json.push_back(std::move(shape));
    });

    // ── Selections ──
    nlohmann::json selections_json = nlohmann::json::array();
    for(const auto& entity : m_graph.selectionState().selections()) {
        selections_json.push_back(
            {{"shapeId", entity.shapeId},
             {"type", Core::entityTypeName(entity.entityType)},
             {"localId", entity.localId}});
    }

    // ── Labels ──
    nlohmann::json labels_json = nlohmann::json::array();
    for(const auto& label : m_graph.labelManager().labels()) {
        labels_json.push_back(
            {{"text", label.text},
             {"entity",
              {{"shapeId", label.entity.shapeId},
               {"type", Core::entityTypeName(label.entity.entityType)},
               {"localId", label.entity.localId}}}});
    }

    // ── Hover ──
    nlohmann::json hover_json = nlohmann::json();
    if(auto hovered = m_graph.selectionState().hovered()) {
        hover_json = {{"shapeId", hovered->shapeId},
                      {"type", Core::entityTypeName(hovered->entityType)},
                      {"localId", hovered->localId}};
    }

    // ── Assemble metadata ──
    result["metadata"] = {{"viewport", {{"width", width}, {"height", height}}},
                          {"camera", std::move(camera_json)},
                          {"visibleShapes", std::move(shapes_json)},
                          {"selections", std::move(selections_json)},
                          {"labels", std::move(labels_json)},
                          {"hover", std::move(hover_json)}};

    if(progress) {
        progress(1.0, "Done");
    }
    return result;
}
```

Note: The full includes at the top of the file should be:

```cpp
#include <opengeolab/scene/capture_viewport_action.hpp>

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/scene/bounding_box3d.hpp>
#include <opengeolab/scene/camera_state.hpp>
#include <opengeolab/scene/label_manager.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/scene_node.hpp>
#include <opengeolab/scene/selection_state.hpp>
#include <opengeolab/scene/viewport_state.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <limits>
#include <string>
```

- [ ] **Step 5: Build and run tests**

Run:
```bash
cmake --build build --config RelWithDebInfo --target opengeolab_scene_test --parallel 4
ctest --test-dir build -C RelWithDebInfo -R opengeolab_scene_test --output-on-failure
```

Expected: All CaptureViewportAction tests PASS.

**Potential issues:**
- `BoundingBox3D::corners()` — verify this method exists and returns `std::array<glm::vec3, 8>`. If not, you'll need to compute corners manually from `min()` and `max()`.
- `LabelManager::labels()` — verify it returns a container of `Label3D` with `.text` and `.entity` fields.

- [ ] **Step 6: Commit**

```bash
git add src/libs/scene/src/capture_viewport_action.cpp
git add src/libs/scene/test/capture_viewport_action_test.cpp
git add src/libs/scene/CMakeLists.txt
git commit -m "feat(scene): implement CaptureViewportAction metadata collection

Collects camera state, visible shapes with screen bounding boxes,
current selections, labels, and hover state. Screen bounding boxes
are computed by projecting world AABB corners through the MVP matrix."
```

---

### Task 3: Register Action in SceneModule

**Files:**
- Modify: `src/libs/scene/src/scene_module.cpp:1-56`

- [ ] **Step 1: Add include and registration**

In `src/libs/scene/src/scene_module.cpp`, add include after line 11 (`clear_labels_action.hpp`):

```cpp
#include <opengeolab/scene/capture_viewport_action.hpp>
```

Add registration after line 55 (`registerAction<SetAutoLabelAction>`):

```cpp
    registerAction<CaptureViewportAction>(std::cref(m_sceneGraph));
```

Note: Using `std::cref` because the action takes `const SceneGraph&`.

- [ ] **Step 2: Build full project**

Run:
```bash
cmake --build build --config RelWithDebInfo --parallel 4
```
Expected: BUILD SUCCESS

- [ ] **Step 3: Run all scene tests**

Run:
```bash
ctest --test-dir build -C RelWithDebInfo -R opengeolab_scene_test --output-on-failure
```
Expected: ALL PASS (existing tests + new CaptureViewportAction tests)

- [ ] **Step 4: Verify action appears in module description**

Run the full test suite to ensure nothing is broken:
```bash
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```
Expected: ALL PASS

- [ ] **Step 5: Commit**

```bash
git add src/libs/scene/src/scene_module.cpp
git commit -m "feat(scene): register CaptureViewportAction in SceneModule"
```

---

## BoundingBox3D Compatibility Notes

The `execute()` implementation computes AABB corners manually from `bounds.min` and `bounds.max` since `BoundingBox3D` (see `bounding_box3d.hpp`) has no `corners()` method. The 8 corners are generated inline.

Similarly, check `LabelManager::labels()` — verify it returns a container of `Label3D` with `.text` and `.entity` fields. If the API differs, adjust the label iteration accordingly.
