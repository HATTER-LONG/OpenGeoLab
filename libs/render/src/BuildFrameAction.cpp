#include <ogl/render/BuildFrameAction.hpp>

#include <ogl/geometry/GeometryModel.hpp>
#include <ogl/render/RenderFrame.hpp>
#include <ogl/render/RenderLogger.hpp>
#include <ogl/scene/SceneGraph.hpp>

#include <sstream>

namespace {

auto reportProgress(const OGL::Core::ProgressCallback& progress_callback,
                    double progress,
                    const std::string& message) -> bool {
    return !progress_callback || progress_callback(progress, message);
}

auto cancellationResponse(const OGL::Core::ServiceRequest& request, const std::string& message)
    -> OGL::Core::ServiceResponse {
    return {.success = false,
            .module = request.module,
            .action = request.action,
            .message = message,
            .payload = nlohmann::json::object()};
}

auto buildRenderEquivalentPython(const OGL::Core::ServiceRequest& request) -> std::string {
    std::ostringstream script;
    script << "import json\n";
    script << "import opengeolab\n\n";
    script << "bridge = opengeolab.OpenGeoLabPythonBridge()\n";
    script << "request = json.loads(r'''" << request.toJson().dump(2) << "''')\n";
    script << "result = bridge.process(request)\n";
    script << "print(result)";
    return script.str();
}

auto buildGeometryModel(const nlohmann::json& param) -> OGL::Geometry::GeometryModel {
    return OGL::Geometry::GeometryModel(
        {.modelName = param.value("modelName", std::string{"Bracket_A01"}),
         .bodyCount = param.value("bodyCount", 3),
         .source = param.value("source", std::string{"render-service"})});
}

} // namespace

namespace OGL::Render {

auto BuildFrameAction::execute(const OGL::Core::ServiceRequest& request,
                               const OGL::Core::ProgressCallback& progress_callback)
    -> OGL::Core::ServiceResponse {
    if(!reportProgress(progress_callback, 0.2, "Preparing geometry model for render frame...")) {
        return cancellationResponse(request, "Render frame construction was cancelled.");
    }

    const auto geometry_model = buildGeometryModel(request.param);

    if(!reportProgress(progress_callback, 0.55, "Building scene graph for render frame...")) {
        return cancellationResponse(request, "Render frame construction was cancelled.");
    }

    const auto scene_graph = OGL::Scene::buildSceneGraph(geometry_model);

    if(!reportProgress(progress_callback, 0.85, "Building render draw items...")) {
        return cancellationResponse(request, "Render frame construction was cancelled.");
    }

    const auto render_frame = buildRenderFrame(scene_graph, request.param);

    OGL_RENDER_LOG_INFO("Built render frame frameId={} drawItemCount={}", render_frame.frameId(),
                        render_frame.drawItems().size());

    reportProgress(progress_callback, 0.95, "Render frame completed.");

    return {.success = true,
            .module = request.module,
            .action = request.action,
            .message = "Render frame assembled from scene graph.",
            .payload = {
                {"sceneGraph", scene_graph.toJson()},
                {"renderFrame", render_frame.toJson()},
                {"summary", render_frame.summary()},
                {"equivalentPython",
                 buildRenderEquivalentPython({.module = request.module,
                                              .action = BuildFrameAction::actionName(),
                                              .param = request.param})},
            }};
}

} // namespace OGL::Render

