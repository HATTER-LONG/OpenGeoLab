/**
 * @file capture_viewport_action.cpp
 * @brief CaptureViewportAction — collect scene metadata + optional screenshot
 */

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
    return {{"name", ACTION_NAME},
            {"description",
             "Capture viewport metadata and optionally a screenshot. "
             "Returns structured JSON with scene state for AI context."},
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
                {"description", "Whether to collect scene metadata (default true)."}}},
              {"captureImage",
               {{"type", "boolean"},
                {"required", false},
                {"description",
                 "Whether to capture a screenshot (default true). "
                 "Set false to skip image capture (e.g. in tests)."}}}}},
            {"returns",
             {{"ok",
               {{"type", "boolean"},
                {"description", "true when the action completes successfully."}}},
              {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
              {"metadata",
               {{"type", "object"},
                {"description",
                 "Scene metadata: viewport, camera, visibleShapes, selections, "
                 "labels, hover."}}},
              {"image",
               {{"type", "string"},
                {"description",
                 "Base64-encoded PNG screenshot, or null if capture timed out."}}}}}};
}

nlohmann::json CaptureViewportAction::execute(const nlohmann::json& param,
                                              const Core::ProgressCallback& progress) {
    const auto args = param.is_object() ? param : nlohmann::json::object();

    int width = 1024;
    int height = 768;
    bool includeMeta = true;
    bool captureImage = true;
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
        if(args.contains("captureImage") && args["captureImage"].is_boolean()) {
            captureImage = args["captureImage"].get<bool>();
        }
    } catch(const nlohmann::json::exception& e) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", e.what()}};
    }

    nlohmann::json result = {{"ok", true}, {"action", ACTION_NAME}};

    if(!includeMeta && !captureImage) {
        if(progress) {
            progress(1.0, "Done");
        }
        return result;
    }

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
    if(captureImage) {
        auto promise = std::make_shared<std::promise<std::string>>();
        auto future = promise->get_future();

        PendingCapture capture_req;
        capture_req.width = width;
        capture_req.height = height;
        capture_req.promise = promise;
        m_graph.viewportState().requestCapture(std::move(capture_req));

        if(future.wait_for(std::chrono::seconds(5)) == std::future_status::ready) {
            auto image_data = future.get();
            if(!image_data.empty()) {
                result["image"] = std::move(image_data);
            } else {
                result["image"] = nullptr;
                result["imageError"] = "Capture returned empty data.";
            }
        } else {
            result["image"] = nullptr;
            result["imageError"] =
                "Capture timed out — render thread did not respond within 5s.";
        }
    }

    if(progress) {
        progress(1.0, "Done");
    }
    return result;
}

} // namespace OpenGeoLab::Scene
