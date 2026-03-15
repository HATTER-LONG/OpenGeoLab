#include <ogl/scene/BuildSceneAction.hpp>

#include <ogl/geometry/GeometryModel.hpp>
#include <ogl/scene/SceneGraph.hpp>
#include <ogl/scene/SceneLogger.hpp>

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

auto buildSceneEquivalentPython(const OGL::Core::ServiceRequest& request) -> std::string {
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
         .source = param.value("source", std::string{"scene-service"})});
}

} // namespace

namespace OGL::Scene {

auto BuildSceneAction::execute(const OGL::Core::ServiceRequest& request,
                               const OGL::Core::ProgressCallback& progress_callback)
    -> OGL::Core::ServiceResponse {
    if(!reportProgress(progress_callback, 0.25, "Preparing geometry model for scene graph...")) {
        return cancellationResponse(request, "Scene graph construction was cancelled.");
    }

    const auto geometry_model = buildGeometryModel(request.param);

    if(!reportProgress(progress_callback, 0.7, "Building scene graph nodes...")) {
        return cancellationResponse(request, "Scene graph construction was cancelled.");
    }

    const auto scene_graph = buildSceneGraph(geometry_model);
    OGL_SCENE_LOG_INFO("Built scene graph sceneId={} nodeCount={}", scene_graph.sceneId(),
                       scene_graph.nodes().size());

    reportProgress(progress_callback, 0.95, "Scene graph completed.");

    return {.success = true,
            .module = request.module,
            .action = request.action,
            .message = "Scene graph assembled from geometry model.",
            .payload = {
                {"sceneGraph", scene_graph.toJson()},
                {"summary", scene_graph.summary()},
                {"equivalentPython",
                 buildSceneEquivalentPython({.module = request.module,
                                             .action = BuildSceneAction::actionName(),
                                             .param = request.param})},
            }};
}

} // namespace OGL::Scene

