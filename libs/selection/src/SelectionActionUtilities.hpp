#pragma once

#include <ogl/core/IService.hpp>
#include <ogl/geometry/GeometryModel.hpp>
#include <ogl/render/RenderFrame.hpp>
#include <ogl/scene/SceneGraph.hpp>
#include <ogl/selection/SelectionLogger.hpp>
#include <ogl/selection/SelectionResult.hpp>

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace OGL::Selection::Internal {

inline auto reportProgress(const OGL::Core::ProgressCallback& progress_callback,
                           double progress,
                           const std::string& message) -> bool {
    return !progress_callback || progress_callback(progress, message);
}

inline auto cancellationResponse(const OGL::Core::ServiceRequest& request,
                                 const std::string& message) -> OGL::Core::ServiceResponse {
    return {.success = false,
            .module = request.module,
            .action = request.action,
            .message = message,
            .payload = nlohmann::json::object()};
}

inline auto buildSelectionEquivalentPython(const OGL::Core::ServiceRequest& request)
    -> std::string {
    std::ostringstream script;
    script << "import json\n";
    script << "import opengeolab\n\n";
    script << "bridge = opengeolab.OpenGeoLabPythonBridge()\n";
    script << "request = json.loads(r'''" << request.toJson().dump(2) << "''')\n";
    script << "result = bridge.process(request)\n";
    script << "print(result)";
    return script.str();
}

inline auto buildGeometryModel(const nlohmann::json& param)
    -> OGL::Geometry::GeometryModel {
    return OGL::Geometry::GeometryModel(
        {.modelName = param.value("modelName", std::string{"Bracket_A01"}),
         .bodyCount = param.value("bodyCount", 3),
         .source = param.value("source", std::string{"selection-service"})});
}

inline auto normalizePickParam(const nlohmann::json& param) -> nlohmann::json {
    nlohmann::json normalized = param;
    normalized["mode"] = "pick";
    return normalized;
}

inline auto normalizeBoxSelectParam(const nlohmann::json& param) -> nlohmann::json {
    nlohmann::json normalized = param;
    normalized["mode"] = "box";
    if(!normalized.contains("selectionCount")) {
        normalized["selectionCount"] = 2;
    }
    return normalized;
}

inline auto buildSelectionResponse(const OGL::Core::ServiceRequest& request,
                                   std::string_view canonical_action,
                                   nlohmann::json normalized_param,
                                   const OGL::Core::ProgressCallback& progress_callback)
    -> OGL::Core::ServiceResponse {
    if(!reportProgress(progress_callback, 0.2, "Building scene graph for selection...")) {
        return cancellationResponse(request, "Selection request was cancelled.");
    }

    const auto geometry_model = buildGeometryModel(normalized_param);
    const auto scene_graph = OGL::Scene::buildSceneGraph(geometry_model);

    if(!reportProgress(progress_callback, 0.55, "Building render frame for selection...")) {
        return cancellationResponse(request, "Selection request was cancelled.");
    }

    const auto render_frame = OGL::Render::buildRenderFrame(scene_graph, normalized_param);

    if(!reportProgress(progress_callback, 0.85, "Evaluating selection hits...")) {
        return cancellationResponse(request, "Selection request was cancelled.");
    }

    const auto selection_result = OGL::Selection::evaluateSelection(scene_graph, render_frame,
                                                                    normalized_param);

    OGL_SELECTION_LOG_INFO("Resolved selection action={} mode={} hitCount={}", canonical_action,
                           selection_result.mode(), selection_result.hits().size());

    const auto completion_message = selection_result.mode() == "box"
                                        ? std::string{"Box selection completed."}
                                        : std::string{"Pick selection completed."};
    reportProgress(progress_callback, 0.95, completion_message);

    return {.success = true,
            .module = request.module,
            .action = request.action,
            .message = "Selection completed through geometry, scene, and render data.",
            .payload = {
                {"sceneGraph", scene_graph.toJson()},
                {"renderFrame", render_frame.toJson()},
                {"selectionResult", selection_result.toJson()},
                {"summary", selection_result.summary()},
                {"equivalentPython",
                 buildSelectionEquivalentPython({.module = request.module,
                                                 .action = std::string{canonical_action},
                                                 .param = std::move(normalized_param)})},
            }};
}

} // namespace OGL::Selection::Internal

