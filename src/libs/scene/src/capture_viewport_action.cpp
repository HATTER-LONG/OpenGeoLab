/**
 * @file capture_viewport_action.cpp
 * @brief CaptureViewportAction — collect scene metadata + save screenshot to file
 */

#include <opengeolab/scene/capture_viewport_action.hpp>

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/geometry/topology_utils.hpp>
#include <opengeolab/scene/bounding_box3d.hpp>
#include <opengeolab/scene/camera_state.hpp>
#include <opengeolab/scene/label_manager.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/scene_node.hpp>
#include <opengeolab/scene/selection_state.hpp>
#include <opengeolab/scene/viewport_state.hpp>

#include <TopoDS.hxx>

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <future>
#include <limits>
#include <memory>
#include <string>

namespace OpenGeoLab::Scene {

CaptureViewportAction::CaptureViewportAction(SceneGraph& graph) : m_graph(graph) {}
CaptureViewportAction::~CaptureViewportAction() = default;

nlohmann::json CaptureViewportAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Capture a viewport screenshot to file and return structured "
                        "scene metadata for AI context."},
        {"params",
         {{"filePath",
           {{"type", "string"},
            {"required", true},
            {"description", "Absolute path where the PNG screenshot will be saved."}}},
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
            {"description", "When true, each visibleShape includes a topology summary "
                            "(counts, faces, edges). Default false."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"savedPath",
           {{"type", "string"}, {"description", "PNG file path written successfully."}}},
          {"savedPathError",
           {{"type", "string"}, {"description", "Reason writing filePath failed, if any."}}},
          {"metadata",
           {{"type", "object"},
            {"description", "Scene metadata: viewport, camera, visibleShapes (with "
                            "optional topology), selections, labels, hover."}}}}}};
}

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

    std::string file_path = args["filePath"].get<std::string>();
    int width = 1024;
    int height = 768;
    bool include_meta = true;
    bool include_topology = false;
    try {
        if(args.contains("width") && args["width"].is_number()) {
            width = args["width"].get<int>();
        }
        if(args.contains("height") && args["height"].is_number()) {
            height = args["height"].get<int>();
        }
        if(args.contains("includeMetadata") && args["includeMetadata"].is_boolean()) {
            include_meta = args["includeMetadata"].get<bool>();
        }
        if(args.contains("includeTopology") && args["includeTopology"].is_boolean()) {
            include_topology = args["includeTopology"].get<bool>();
        }
    } catch(const nlohmann::json::exception& e) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", e.what()}};
    }

    nlohmann::json result = {{"ok", true}, {"action", ACTION_NAME}};

    // ── Collect metadata ──
    if(include_meta) {
        const auto cam = m_graph.viewportState().camera();
        const float aspect =
            (height > 0) ? static_cast<float>(width) / static_cast<float>(height) : 1.0F;
        const auto view_mat = cam.viewMatrix();
        const auto proj_mat = cam.projMatrix(aspect);
        const auto mvp = proj_mat * view_mat;

        nlohmann::json camera_json = {{"eye", {cam.position.x, cam.position.y, cam.position.z}},
                                      {"target", {cam.target.x, cam.target.y, cam.target.z}},
                                      {"up", {cam.up.x, cam.up.y, cam.up.z}}};

        nlohmann::json shapes_json = nlohmann::json::array();

        m_graph.traverseVisible([&](const SceneNode& node) {
            if(node.sourceType().empty()) {
                return;
            }

            nlohmann::json shape = {{"shapeId", node.sourceId()},
                                    {"name", std::string(node.name())}};

            const auto bounds = node.worldBounds();
            if(bounds.isValid()) {
                const auto mn = bounds.min;
                const auto mx = bounds.max;
                shape["worldBounds"] = {{"min", {mn.x, mn.y, mn.z}}, {"max", {mx.x, mx.y, mx.z}}};

                const std::array<glm::vec3, 8> corners = {
                    glm::vec3{mn.x, mn.y, mn.z}, glm::vec3{mx.x, mn.y, mn.z},
                    glm::vec3{mn.x, mx.y, mn.z}, glm::vec3{mx.x, mx.y, mn.z},
                    glm::vec3{mn.x, mn.y, mx.z}, glm::vec3{mx.x, mn.y, mx.z},
                    glm::vec3{mn.x, mx.y, mx.z}, glm::vec3{mx.x, mx.y, mx.z},
                };

                float min_x = std::numeric_limits<float>::max();
                float min_y = std::numeric_limits<float>::max();
                float max_x = std::numeric_limits<float>::lowest();
                float max_y = std::numeric_limits<float>::lowest();
                bool any_visible = false;

                for(const auto& corner : corners) {
                    const auto clip = mvp * glm::vec4(corner, 1.0F);
                    if(clip.w <= 0.0F) {
                        continue;
                    }

                    const auto ndc = glm::vec3(clip) / clip.w;
                    const float sx = (ndc.x * 0.5F + 0.5F) * static_cast<float>(width);
                    const float sy = (1.0F - (ndc.y * 0.5F + 0.5F)) * static_cast<float>(height);
                    min_x = std::min(min_x, sx);
                    min_y = std::min(min_y, sy);
                    max_x = std::max(max_x, sx);
                    max_y = std::max(max_y, sy);
                    any_visible = true;
                }

                if(any_visible) {
                    shape["screenBBox"] = {{"x", static_cast<int>(min_x)},
                                           {"y", static_cast<int>(min_y)},
                                           {"w", static_cast<int>(max_x - min_x)},
                                           {"h", static_cast<int>(max_y - min_y)}};
                }
            }

            // ── Per-shape topology (when requested) ──
            if(include_topology) {
                auto* store = m_graph.shapeStore();
                if(store) {
                    const auto* entry = store->find(node.sourceId());
                    if(entry) {
                        nlohmann::json topo;
                        topo["counts"] = {{"faces", entry->faceMap.Extent()},
                                          {"edges", entry->edgeMap.Extent()},
                                          {"vertices", entry->vertexMap.Extent()}};

                        nlohmann::json faces_arr = nlohmann::json::array();
                        for(int i = 1; i <= entry->faceMap.Extent(); ++i) {
                            faces_arr.push_back(Geometry::toJson(Geometry::extractFaceInfo(
                                static_cast<uint32_t>(i), TopoDS::Face(entry->faceMap(i)))));
                        }
                        topo["faces"] = std::move(faces_arr);

                        nlohmann::json edges_arr = nlohmann::json::array();
                        for(int i = 1; i <= entry->edgeMap.Extent(); ++i) {
                            edges_arr.push_back(Geometry::toJson(Geometry::extractEdgeInfo(
                                static_cast<uint32_t>(i), TopoDS::Edge(entry->edgeMap(i)))));
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
            selections_json.push_back({{"shapeId", entity.shapeId},
                                       {"type", Core::entityTypeName(entity.entityType)},
                                       {"localId", entity.localId}});
        }

        nlohmann::json labels_json = nlohmann::json::array();
        for(const auto& label : m_graph.labelManager().labels()) {
            labels_json.push_back({{"text", label.text},
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
    capture_req.outputPath = file_path;
    capture_req.promise = promise;
    m_graph.viewportState().requestCapture(std::move(capture_req));

    if(future.wait_for(std::chrono::seconds(5)) == std::future_status::ready) {
        auto capture_result = future.get();
        if(!capture_result.savedPath.empty()) {
            result["savedPath"] = std::move(capture_result.savedPath);
        }
        if(!capture_result.savedPathError.empty()) {
            result["savedPathError"] = capture_result.savedPathError;
        }
    } else {
        result["savedPathError"] = "Capture timed out — render thread did not respond within 5s.";
    }

    if(progress) {
        progress(1.0, "Done");
    }
    return result;
}

} // namespace OpenGeoLab::Scene
