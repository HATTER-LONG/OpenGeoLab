# 3D Model & Camera State Export — Part 4 of 4

> Part file: Enhanced CaptureViewportAction — remove base64 image, require
> filePath, add includeTopology.
>
> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **Recommended models:** Use Claude Opus 4.6 for design or plan revisions. Use GPT-5.4 for implementation, debugging, verification, and code-review subtasks.

**Goal:** Simplify CaptureViewportAction by removing base64 image encoding,
making filePath required, and adding per-shape topology data when requested.

**Prerequisites:** Parts 1–3 must be complete and committed.

**Design spec:** `docs/superpowers/specs/2026-04-10-3d-model-camera-state-export-design.md`
(section "Enhanced `capture_viewport`", lines 287–352)

---

## File Structure (Part 4 scope)

| Action | Path | Purpose |
|--------|------|---------|
| Modify | `src/libs/scene/include/opengeolab/scene/viewport_state.hpp:44-58` | Simplify CaptureResult/PendingCapture |
| Modify | `src/app/src/gl_viewport_renderer.cpp:544-594` | Remove base64 encoding branch |
| Modify | `src/libs/scene/src/capture_viewport_action.cpp` | Remove captureImage, add filePath + includeTopology |
| Modify | `src/libs/scene/test/capture_viewport_action_test.cpp` | Update all tests |

---

### Task 13: Simplify `CaptureResult` and `PendingCapture`

The base64 image path is being removed. `PendingCapture` no longer needs a
`captureImage` bool, and `CaptureResult` no longer needs `image`/`imageError`.

**Files:**
- Modify: `src/libs/scene/include/opengeolab/scene/viewport_state.hpp:44-58`

- [ ] **Step 1: Update CaptureResult and PendingCapture structs**

Replace lines 44–58 of `viewport_state.hpp`:

Old code:
```cpp
/// @brief Pending viewport capture request (async, consumed by renderer).
struct CaptureResult {
    std::string image;          ///< Base64 PNG for JSON response
    std::string imageError;     ///< Error while producing JSON image data
    std::string savedPath;      ///< Output path written successfully
    std::string savedPathError; ///< Error while writing outputPath
};

/// @brief Pending viewport capture request (async, consumed by renderer).
struct PendingCapture {
    int width{1024};             ///< Desired capture width in pixels
    int height{768};             ///< Desired capture height in pixels
    std::string outputPath;      ///< Optional PNG output path
    bool captureImage{true};     ///< Whether JSON image data is requested
    /// Shared promise to deliver capture results back to the requester.
    std::shared_ptr<std::promise<CaptureResult>> promise;
};
```

New code:
```cpp
/// @brief Result of a viewport capture request.
struct CaptureResult {
    std::string savedPath;      ///< Output path written successfully
    std::string savedPathError; ///< Error while writing filePath
};

/// @brief Pending viewport capture request (async, consumed by renderer).
struct PendingCapture {
    int width{1024};             ///< Desired capture width in pixels
    int height{768};             ///< Desired capture height in pixels
    std::string outputPath;      ///< PNG output file path (always set)
    /// Shared promise to deliver capture results back to the requester.
    std::shared_ptr<std::promise<CaptureResult>> promise;
};
```

- [ ] **Step 2: Build scene library to verify header compiles**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 4
```

Expected: Compile errors in `capture_viewport_action.cpp` and
`gl_viewport_renderer.cpp` (they still reference removed fields). This is
expected and will be fixed in subsequent steps.

- [ ] **Step 3: Commit struct changes**

```bash
git add src/libs/scene/include/opengeolab/scene/viewport_state.hpp
git commit -m "refactor(scene): simplify CaptureResult and PendingCapture

Remove base64 image fields (image, imageError) from CaptureResult and
captureImage bool from PendingCapture. Screenshots are now always
saved to filePath — no more in-JSON base64 encoding.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 14: Update `GLViewportRenderer::executeCaptureRequest`

Remove the base64 encoding branch. The renderer now only saves to file.

**Files:**
- Modify: `src/app/src/gl_viewport_renderer.cpp:544-594`

- [ ] **Step 1: Simplify executeCaptureRequest**

Replace the `executeCaptureRequest` method body (lines 544–594):

Old code:
```cpp
void GLViewportRenderer::executeCaptureRequest(const Scene::PendingCapture& capture) {
    const int w = m_frameState.viewportWidth;
    const int h = m_frameState.viewportHeight;

    if(w <= 0 || h <= 0) {
        Scene::CaptureResult result;
        if(capture.captureImage) {
            result.imageError = "Capture failed because the viewport size is invalid.";
        }
        if(!capture.outputPath.empty()) {
            result.savedPathError = "Capture failed because the viewport size is invalid.";
        }
        fulfillCapturePromise(capture, std::move(result));
        return;
    }

    // The viewport FBO uses 4× MSAA, so direct glReadPixels yields
    // undefined data.  QOpenGLFramebufferObject::toImage() resolves the
    // multisample buffer, reads the pixels and flips the image in one step.
    // Convert to RGB32 (opaque) so the saved PNG matches the on-screen
    // composite regardless of FBO alpha state.
    QImage image = framebufferObject()->toImage().convertToFormat(QImage::Format_RGB32);

    Scene::CaptureResult result;

    if(!capture.outputPath.empty()) {
        const QString outputPath = QString::fromStdString(capture.outputPath);
        const QFileInfo outputFile(outputPath);
        QDir outputDir = outputFile.dir();
        if(!outputDir.exists() && !outputDir.mkpath(QStringLiteral("."))) {
            result.savedPathError = "Failed to create the output directory for outputPath.";
        } else if(!image.save(outputPath, "PNG")) {
            result.savedPathError = "Failed to write PNG to outputPath.";
        } else {
            result.savedPath = capture.outputPath;
        }
    }

    if(capture.captureImage) {
        QByteArray pngBytes;
        QBuffer buffer(&pngBytes);
        buffer.open(QIODevice::WriteOnly);
        if(!image.save(&buffer, "PNG") || pngBytes.isEmpty()) {
            result.imageError = "Failed to encode the captured viewport as PNG.";
        } else {
            result.image = pngBytes.toBase64().toStdString();
        }
    }

    fulfillCapturePromise(capture, std::move(result));
}
```

New code:
```cpp
void GLViewportRenderer::executeCaptureRequest(const Scene::PendingCapture& capture) {
    const int w = m_frameState.viewportWidth;
    const int h = m_frameState.viewportHeight;

    if(w <= 0 || h <= 0) {
        Scene::CaptureResult result;
        result.savedPathError = "Capture failed because the viewport size is invalid.";
        fulfillCapturePromise(capture, std::move(result));
        return;
    }

    // The viewport FBO uses 4× MSAA, so direct glReadPixels yields
    // undefined data.  QOpenGLFramebufferObject::toImage() resolves the
    // multisample buffer, reads the pixels and flips the image in one step.
    // Convert to RGB32 (opaque) so the saved PNG matches the on-screen
    // composite regardless of FBO alpha state.
    QImage image = framebufferObject()->toImage().convertToFormat(QImage::Format_RGB32);

    Scene::CaptureResult result;

    const QString outputPath = QString::fromStdString(capture.outputPath);
    const QFileInfo outputFile(outputPath);
    QDir outputDir = outputFile.dir();
    if(!outputDir.exists() && !outputDir.mkpath(QStringLiteral("."))) {
        result.savedPathError = "Failed to create the output directory for filePath.";
    } else if(!image.save(outputPath, "PNG")) {
        result.savedPathError = "Failed to write PNG to filePath.";
    } else {
        result.savedPath = capture.outputPath;
    }

    fulfillCapturePromise(capture, std::move(result));
}
```

- [ ] **Step 2: Build app target to verify**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_app --parallel 4
```

Expected: Compile errors in `capture_viewport_action.cpp` remain (fixed next
task). The renderer itself should compile.

- [ ] **Step 3: Commit**

```bash
git add src/app/src/gl_viewport_renderer.cpp
git commit -m "refactor(render): remove base64 encoding from viewport capture

The renderer now only saves to file — no more in-memory PNG encoding
for JSON transport. This matches the new filePath-only capture API.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 15: Update `CaptureViewportAction`

Replace `captureImage`/`outputPath` with required `filePath`. Add
`includeTopology` that enriches each visibleShape entry with topology data.

**Files:**
- Modify: `src/libs/scene/src/capture_viewport_action.cpp`

- [ ] **Step 1: Rewrite describe()**

Replace the `describe()` method entirely:

```cpp
nlohmann::json CaptureViewportAction::describe() const {
    return {{"name", ACTION_NAME},
            {"description",
             "Capture a viewport screenshot to file and return structured "
             "scene metadata for AI context."},
            {"params",
             {{"filePath",
               {{"type", "string"},
                {"required", true},
                {"description",
                 "Absolute path where the PNG screenshot will be saved."}}},
              {"width",
               {{"type", "integer"},
                {"required", false},
                {"description", "Desired capture width in pixels (default 1024)."}}},
              {"height",
               {{"type", "integer"},
                {"required", false},
                {"description", "Desired capture height in pixels (default 768)."}}},
              {"includeMetadata",
               {{"type", "boolean"},
                {"required", false},
                {"description", "Whether to collect scene metadata (default true)."}}},
              {"includeTopology",
               {{"type", "boolean"},
                {"required", false},
                {"description",
                 "When true, each visibleShape includes a topology summary "
                 "(counts, faces, edges). Default false."}}}}},
            {"returns",
             {{"ok",
               {{"type", "boolean"},
                {"description", "true when the action completes successfully."}}},
              {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
              {"savedPath",
               {{"type", "string"},
                {"description", "PNG file path written successfully."}}},
              {"savedPathError",
               {{"type", "string"},
                {"description", "Reason writing filePath failed, if any."}}},
              {"metadata",
               {{"type", "object"},
                {"description",
                 "Scene metadata: viewport, camera, visibleShapes (with "
                 "optional topology), selections, labels, hover."}}}}}};
}
```

- [ ] **Step 2: Rewrite execute() — parameter parsing**

Replace the entire `execute()` method. The new method:

```cpp
nlohmann::json CaptureViewportAction::execute(const nlohmann::json& param,
                                              const Core::ProgressCallback& progress) {
    const auto args = param.is_object() ? param : nlohmann::json::object();

    // filePath is required
    if(!args.contains("filePath") || !args["filePath"].is_string() ||
       args["filePath"].get<std::string>().empty()) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "Missing or empty required parameter 'filePath'."}};
    }

    std::string filePath = args["filePath"].get<std::string>();
    int width = 1024;
    int height = 768;
    bool includeMeta = true;
    bool includeTopology = false;
    try {
        if(args.contains("width") && args["width"].is_number()) {
            width = args["width"].get<int>();
        }
        if(args.contains("height") && args["height"].is_number()) {
            height = args["height"].get<int>();
        }
        if(args.contains("includeMetadata") && args["includeMetadata"].is_boolean()) {
            includeMeta = args["includeMetadata"].get<bool>();
        }
        if(args.contains("includeTopology") && args["includeTopology"].is_boolean()) {
            includeTopology = args["includeTopology"].get<bool>();
        }
    } catch(const nlohmann::json::exception& e) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", e.what()}};
    }

    nlohmann::json result = {{"ok", true}, {"action", ACTION_NAME}};

    // ── Collect metadata ──
    if(includeMeta) {
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

        nlohmann::json shapes_json = nlohmann::json::array();

        m_graph.traverseVisible([&](const SceneNode& node) {
            if(node.sourceType().empty()) {
                return;
            }

            nlohmann::json shape = {
                {"shapeId", node.sourceId()},
                {"name", std::string(node.name())}};

            const auto bounds = node.worldBounds();
            if(bounds.isValid()) {
                const auto mn = bounds.min;
                const auto mx = bounds.max;
                shape["worldBounds"] = {{"min", {mn.x, mn.y, mn.z}},
                                         {"max", {mx.x, mx.y, mx.z}}};

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
                    const auto clip = mvp * glm::vec4(corner, 1.0F);
                    if(clip.w <= 0.0F) {
                        continue;
                    }

                    const auto ndc = glm::vec3(clip) / clip.w;
                    const float sx = (ndc.x * 0.5F + 0.5F) * static_cast<float>(width);
                    const float sy =
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

            // ── Per-shape topology (when requested) ──
            if(includeTopology) {
                auto* store = m_graph.shapeStore();
                if(store) {
                    const auto* entry = store->find(node.sourceId());
                    if(entry) {
                        nlohmann::json topo;
                        topo["counts"] = {
                            {"faces", entry->faceMap.Extent()},
                            {"edges", entry->edgeMap.Extent()},
                            {"vertices", entry->vertexMap.Extent()}};

                        nlohmann::json faces_arr = nlohmann::json::array();
                        for(int i = 1; i <= entry->faceMap.Extent(); ++i) {
                            faces_arr.push_back(Geometry::toJson(
                                Geometry::extractFaceInfo(
                                    static_cast<uint32_t>(i),
                                    TopoDS::Face(entry->faceMap(i)))));
                        }
                        topo["faces"] = std::move(faces_arr);

                        nlohmann::json edges_arr = nlohmann::json::array();
                        for(int i = 1; i <= entry->edgeMap.Extent(); ++i) {
                            edges_arr.push_back(Geometry::toJson(
                                Geometry::extractEdgeInfo(
                                    static_cast<uint32_t>(i),
                                    TopoDS::Edge(entry->edgeMap(i)))));
                        }
                        topo["edges"] = std::move(edges_arr);

                        shape["topology"] = std::move(topo);
                    }
                }
            }

            shapes_json.push_back(std::move(shape));
        });

        nlohmann::json selections_json = nlohmann::json::array();
        for(const auto& entity : m_graph.selectionState().selections()) {
            selections_json.push_back(
                {{"shapeId", entity.shapeId},
                 {"type", Core::entityTypeName(entity.entityType)},
                 {"localId", entity.localId}});
        }

        nlohmann::json labels_json = nlohmann::json::array();
        for(const auto& label : m_graph.labelManager().labels()) {
            labels_json.push_back(
                {{"text", label.text},
                 {"entity",
                  {{"shapeId", label.entity.shapeId},
                   {"type", Core::entityTypeName(label.entity.entityType)},
                   {"localId", label.entity.localId}}}});
        }

        nlohmann::json hover_json = nlohmann::json();
        if(const auto hovered = m_graph.selectionState().hovered()) {
            hover_json = {{"shapeId", hovered->shapeId},
                          {"type", Core::entityTypeName(hovered->entityType)},
                          {"localId", hovered->localId}};
        }

        result["metadata"] = {{"viewport", {{"width", width}, {"height", height}}},
                              {"camera", std::move(camera_json)},
                              {"visibleShapes", std::move(shapes_json)},
                              {"selections", std::move(selections_json)},
                              {"labels", std::move(labels_json)},
                              {"hover", std::move(hover_json)}};
    }

    // ── Request image capture from render thread ──
    auto promise = std::make_shared<std::promise<CaptureResult>>();
    auto future = promise->get_future();

    PendingCapture capture_req;
    capture_req.width = width;
    capture_req.height = height;
    capture_req.outputPath = filePath;
    capture_req.promise = promise;
    m_graph.viewportState().requestCapture(std::move(capture_req));

    if(future.wait_for(std::chrono::seconds(5)) == std::future_status::ready) {
        auto captureResult = future.get();
        if(!captureResult.savedPath.empty()) {
            result["savedPath"] = std::move(captureResult.savedPath);
        }
        if(!captureResult.savedPathError.empty()) {
            result["savedPathError"] = captureResult.savedPathError;
        }
    } else {
        result["savedPathError"] =
            "Capture timed out — render thread did not respond within 5s.";
    }

    if(progress) {
        progress(1.0, "Done");
    }
    return result;
}
```

- [ ] **Step 3: Add the new includes at the top of capture_viewport_action.cpp**

After the existing includes, add:

```cpp
#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/geometry/topology_utils.hpp>

#include <TopoDS.hxx>
```

- [ ] **Step 4: Build scene library**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 4
```

Expected: Build succeeds (the test target will have failures because tests
still reference old API — fixed in the next task).

- [ ] **Step 5: Commit**

```bash
git add src/libs/scene/src/capture_viewport_action.cpp
git commit -m "feat(scene): require filePath and add includeTopology to capture_viewport

Replace captureImage/outputPath with required filePath parameter.
No more base64 image in the response. When includeTopology=true,
each visibleShape entry includes topology counts, face summaries,
and edge summaries. Also adds worldBounds to each visibleShape.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 16: Update capture_viewport_action_test.cpp

All tests must be updated to:
- Pass `filePath` instead of `captureImage: false`
- Remove assertions on `image`/`imageError`/`captureImage`
- Add tests for filePath validation and includeTopology

**Files:**
- Modify: `src/libs/scene/test/capture_viewport_action_test.cpp`

- [ ] **Step 1: Rewrite the test file**

Replace the entire file content:

```cpp
/**
 * @file capture_viewport_action_test.cpp
 * @brief Tests for CaptureViewportAction metadata collection
 */

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/scene/bounding_box3d.hpp>
#include <opengeolab/scene/camera_state.hpp>
#include <opengeolab/scene/capture_viewport_action.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/selection_state.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <BRepPrimAPI_MakeBox.hxx>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

using OpenGeoLab::Scene::CaptureViewportAction;
using OpenGeoLab::Scene::SceneGraph;

namespace {

/// Helper: params with a dummy filePath (no render thread available).
/// Capture will time out, but metadata is collected synchronously.
nlohmann::json metaParams(nlohmann::json extra = {}) {
    nlohmann::json params = {{"filePath", "C:/nonexistent/test_capture.png"}};
    if(extra.is_object()) {
        params.merge_patch(extra);
    }
    return params;
}

} // namespace

TEST_SUITE("CaptureViewportAction") {

    TEST_CASE("describe returns expected schema") {
        SceneGraph graph;
        CaptureViewportAction action(graph);
        auto desc = action.describe();

        CHECK(desc["name"] == "capture_viewport");
        CHECK(desc.contains("description"));
        CHECK(desc.contains("params"));
        CHECK(desc["params"].contains("filePath"));
        CHECK(desc["params"].contains("width"));
        CHECK(desc["params"].contains("height"));
        CHECK(desc["params"].contains("includeMetadata"));
        CHECK(desc["params"].contains("includeTopology"));
        CHECK_FALSE(desc["params"].contains("captureImage"));
        CHECK_FALSE(desc["params"].contains("outputPath"));
        CHECK(desc.contains("returns"));
        CHECK(desc["returns"].contains("ok"));
        CHECK(desc["returns"].contains("savedPath"));
        CHECK_FALSE(desc["returns"].contains("image"));
    }

    TEST_CASE("execute requires filePath") {
        SceneGraph graph;
        CaptureViewportAction action(graph);

        auto result = action.execute(nlohmann::json::object(), nullptr);
        CHECK(result["ok"] == false);
        CHECK(result["error"].get<std::string>().find("filePath") != std::string::npos);
    }

    TEST_CASE("execute with empty filePath returns error") {
        SceneGraph graph;
        CaptureViewportAction action(graph);

        auto result = action.execute({{"filePath", ""}}, nullptr);
        CHECK(result["ok"] == false);
    }

    TEST_CASE("execute returns valid metadata") {
        SceneGraph graph;
        CaptureViewportAction action(graph);

        auto result = action.execute(metaParams(), nullptr);

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

        CHECK(meta.contains("labels"));
        CHECK(meta["labels"].is_array());
    }

    TEST_CASE("execute with custom resolution") {
        SceneGraph graph;
        CaptureViewportAction action(graph);

        auto result = action.execute(metaParams({{"width", 512}, {"height", 384}}), nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["metadata"]["viewport"]["width"] == 512);
        CHECK(result["metadata"]["viewport"]["height"] == 384);
    }

    TEST_CASE("execute with includeMetadata=false omits metadata") {
        SceneGraph graph;
        CaptureViewportAction action(graph);

        auto result = action.execute(metaParams({{"includeMetadata", false}}), nullptr);

        CHECK(result["ok"] == true);
        CHECK_FALSE(result.contains("metadata"));
    }

    TEST_CASE("camera state is captured from ViewportState") {
        SceneGraph graph;

        OpenGeoLab::Scene::CameraState cam;
        cam.position = {10.0F, 20.0F, 30.0F};
        cam.target = {1.0F, 2.0F, 3.0F};
        cam.up = {0.0F, 1.0F, 0.0F};
        graph.viewportState().setCamera(cam);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

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

        OpenGeoLab::Core::EntityRef entity{1, OpenGeoLab::Core::EntityType::GeoFace, 3};
        graph.selectionState().addSelection(entity);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        auto& sels = result["metadata"]["selections"];
        REQUIRE(sels.size() == 1);
        CHECK(sels[0]["shapeId"] == 1);
        CHECK(sels[0]["type"] == "GeoFace");
        CHECK(sels[0]["localId"] == 3);
    }

    TEST_CASE("labels are captured from LabelManager") {
        SceneGraph graph;

        OpenGeoLab::Core::EntityRef entity{2, OpenGeoLab::Core::EntityType::GeoEdge, 5};
        graph.labelManager().addLabel({entity, "E:5"});

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        auto& labels = result["metadata"]["labels"];
        REQUIRE(labels.size() == 1);
        CHECK(labels[0]["text"] == "E:5");
        CHECK(labels[0]["entity"]["shapeId"] == 2);
    }

    TEST_CASE("visible nodes appear in visibleShapes") {
        SceneGraph graph;

        auto node_id = graph.addNode("Box_1");
        auto* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        node->setSource("geometry", 1);
        node->setVisible(true);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

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
        auto result = action.execute(metaParams(), nullptr);

        CHECK(result["metadata"]["visibleShapes"].empty());
    }

    TEST_CASE("child of hidden parent is excluded from visibleShapes") {
        SceneGraph graph;

        auto parent_id = graph.addNode("Parent");
        auto* parent = graph.findNode(parent_id);
        REQUIRE(parent != nullptr);
        parent->setSource("geometry", 1);
        parent->setVisible(false);

        auto child_id = graph.addNode("Child", parent_id);
        auto* child = graph.findNode(child_id);
        REQUIRE(child != nullptr);
        child->setSource("geometry", 2);
        child->setVisible(true);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        CHECK(result["metadata"]["visibleShapes"].empty());
    }

    TEST_CASE("hover entity is captured from SelectionState") {
        SceneGraph graph;

        OpenGeoLab::Core::EntityRef entity{3, OpenGeoLab::Core::EntityType::GeoVertex, 7};
        graph.selectionState().setHovered(entity);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        auto& hover = result["metadata"]["hover"];
        CHECK_FALSE(hover.is_null());
        CHECK(hover["shapeId"] == 3);
    }

    TEST_CASE("hover is null when nothing is hovered") {
        SceneGraph graph;

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        CHECK(result["metadata"]["hover"].is_null());
    }

    TEST_CASE("screenBBox is computed for nodes with valid bounds") {
        SceneGraph graph;

        auto node_id = graph.addNode("Box_1");
        auto* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        node->setSource("geometry", 1);
        node->setVisible(true);

        OpenGeoLab::Scene::BoundingBox3D bounds;
        bounds.min = {-1.0F, -1.0F, -1.0F};
        bounds.max = {1.0F, 1.0F, 1.0F};
        node->setLocalBounds(bounds);

        OpenGeoLab::Scene::CameraState cam;
        cam.position = {0.0F, 0.0F, 5.0F};
        cam.target = {0.0F, 0.0F, 0.0F};
        cam.up = {0.0F, 1.0F, 0.0F};
        graph.viewportState().setCamera(cam);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams({{"width", 800}, {"height", 600}}), nullptr);

        auto& shapes = result["metadata"]["visibleShapes"];
        REQUIRE(shapes.size() == 1);
        REQUIRE(shapes[0].contains("screenBBox"));

        auto& bbox = shapes[0]["screenBBox"];
        CHECK(bbox.contains("x"));
        CHECK(bbox.contains("y"));
        CHECK(bbox.contains("w"));
        CHECK(bbox.contains("h"));
        CHECK(bbox["w"].get<int>() > 0);
        CHECK(bbox["h"].get<int>() > 0);
    }

    TEST_CASE("invalid param types use defaults instead of crashing") {
        SceneGraph graph;
        CaptureViewportAction action(graph);

        auto result = action.execute(
            {{"filePath", "C:/temp/test.png"}, {"width", "not_a_number"}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["metadata"]["viewport"]["width"] == 1024);
    }

    TEST_CASE("includeTopology adds topology to visibleShapes") {
        SceneGraph graph;
        OpenGeoLab::Geometry::ShapeStore store;
        auto shape_id = store.add("Box", BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape());
        graph.setShapeStore(&store);

        auto node_id = graph.addNode("Box");
        auto* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        node->setSource("geometry", shape_id);
        node->setVisible(true);

        CaptureViewportAction action(graph);
        auto result = action.execute(
            metaParams({{"includeTopology", true}}), nullptr);

        CHECK(result["ok"] == true);
        auto& shapes = result["metadata"]["visibleShapes"];
        REQUIRE(shapes.size() == 1);
        REQUIRE(shapes[0].contains("topology"));

        auto& topo = shapes[0]["topology"];
        CHECK(topo["counts"]["faces"] == 6);
        CHECK(topo["counts"]["edges"] == 12);
        CHECK(topo["counts"]["vertices"] == 8);
        CHECK(topo["faces"].is_array());
        CHECK(topo["faces"].size() == 6);
        CHECK(topo["edges"].is_array());
        CHECK(topo["edges"].size() == 12);
    }

    TEST_CASE("includeTopology=false does not add topology") {
        SceneGraph graph;
        OpenGeoLab::Geometry::ShapeStore store;
        auto shape_id = store.add("Box", BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape());
        graph.setShapeStore(&store);

        auto node_id = graph.addNode("Box");
        auto* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        node->setSource("geometry", shape_id);
        node->setVisible(true);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        CHECK(result["ok"] == true);
        auto& shapes = result["metadata"]["visibleShapes"];
        REQUIRE(shapes.size() == 1);
        CHECK_FALSE(shapes[0].contains("topology"));
    }

    TEST_CASE("includeTopology without ShapeStore gracefully omits topology") {
        SceneGraph graph; // no shapeStore set

        auto node_id = graph.addNode("Box");
        auto* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        node->setSource("geometry", 1);
        node->setVisible(true);

        CaptureViewportAction action(graph);
        auto result = action.execute(
            metaParams({{"includeTopology", true}}), nullptr);

        CHECK(result["ok"] == true);
        auto& shapes = result["metadata"]["visibleShapes"];
        REQUIRE(shapes.size() == 1);
        CHECK_FALSE(shapes[0].contains("topology"));
    }

    TEST_CASE("worldBounds included for nodes with valid bounds") {
        SceneGraph graph;

        auto node_id = graph.addNode("Box_1");
        auto* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        node->setSource("geometry", 1);
        node->setVisible(true);

        OpenGeoLab::Scene::BoundingBox3D bounds;
        bounds.min = {-1.0F, -1.0F, -1.0F};
        bounds.max = {1.0F, 1.0F, 1.0F};
        node->setLocalBounds(bounds);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        auto& shapes = result["metadata"]["visibleShapes"];
        REQUIRE(shapes.size() == 1);
        REQUIRE(shapes[0].contains("worldBounds"));
        CHECK(shapes[0]["worldBounds"]["min"][0].get<float>() == doctest::Approx(-1.0F));
        CHECK(shapes[0]["worldBounds"]["max"][0].get<float>() == doctest::Approx(1.0F));
    }

    TEST_CASE("capture times out without render thread but metadata is still valid") {
        SceneGraph graph;
        CaptureViewportAction action(graph);

        auto result = action.execute(metaParams(), nullptr);

        CHECK(result["ok"] == true);
        CHECK(result.contains("metadata"));
        // No render thread → capture times out
        CHECK(result.contains("savedPathError"));
        CHECK_FALSE(result.contains("savedPath"));
    }

} // TEST_SUITE
```

- [ ] **Step 2: Build and run ALL scene tests**

```powershell
cmake --build build --config RelWithDebInfo --target opengeolab_scene_test --parallel 4 && ctest --test-dir build -C RelWithDebInfo -R opengeolab_scene_test --output-on-failure
```

Expected: All tests pass.

- [ ] **Step 3: Commit**

```bash
git add src/libs/scene/test/capture_viewport_action_test.cpp
git commit -m "test(scene): update capture_viewport tests for filePath and includeTopology

Remove captureImage/image assertions. Add tests for required filePath
validation, includeTopology with/without ShapeStore, worldBounds in
visibleShapes, and graceful degradation without render thread.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

### Task 17: Full build and integration verification

- [ ] **Step 1: Build all targets**

```powershell
cmake --build build --config RelWithDebInfo --parallel 4
```

Expected: Full build succeeds.

- [ ] **Step 2: Run all tests**

```powershell
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

Expected: All tests pass.

- [ ] **Step 3: Commit any remaining fixes if needed**

If the full build reveals issues (e.g. other code referencing old
`captureImage` field), fix them and commit.

- [ ] **Step 4: Final commit summarizing Part 4**

If all tests pass without extra fixes:

```bash
git log --oneline -8
```

Verify the commit history looks clean.
