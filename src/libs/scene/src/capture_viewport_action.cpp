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
    return {{"name", ACTION_NAME},
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
    const int width = param.value("width", 1024);
    const int height = param.value("height", 768);
    const bool includeMeta = param.value("includeMetadata", true);

    static_cast<void>(width);
    static_cast<void>(height);

    nlohmann::json result = {{"ok", true}, {"action", ACTION_NAME}};

    if(includeMeta) {
        result["metadata"] = nlohmann::json::object();
    }

    if(progress) {
        progress(1.0, "Done");
    }
    return result;
}

} // namespace OpenGeoLab::Scene
