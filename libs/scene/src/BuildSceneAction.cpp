#include <ogl/scene/BuildSceneAction.hpp>

#include <ogl/core/ActionExecutionUtilities.hpp>
#include <ogl/geometry/GeometryModel.hpp>
#include <ogl/scene/SceneGraph.hpp>
#include <ogl/scene/SceneLogger.hpp>

#include <optional>

namespace {

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
    OGL::Core::ServiceResponse early_response;
    std::optional<OGL::Geometry::GeometryModel> geometry_model;
    std::optional<OGL::Scene::SceneGraph> scene_graph;

    if(!OGL::Core::runProgressStage(
           request, progress_callback, 0.25, "Preparing geometry model for scene graph...",
           "Scene graph construction was cancelled.",
           [&]() { geometry_model = buildGeometryModel(request.param); }, early_response)) {
        return early_response;
    }

    if(!OGL::Core::runProgressStage(
           request, progress_callback, 0.7, "Building scene graph nodes...",
           "Scene graph construction was cancelled.",
           [&]() {
               scene_graph = buildSceneGraph(*geometry_model);
               OGL_SCENE_LOG_INFO("Built scene graph sceneId={} nodeCount={}",
                                  scene_graph->sceneId(), scene_graph->nodes().size());
           },
           early_response)) {
        return early_response;
    }

    OGL::Core::reportProgress(progress_callback, 0.95, "Scene graph completed.");

    return {.success = true,
            .module = request.module,
            .action = request.action,
            .message = "Scene graph assembled from geometry model.",
            .payload = {
                {"sceneGraph", scene_graph->toJson()},
                {"summary", scene_graph->summary()},
                {"equivalentPython",
                 OGL::Core::buildEquivalentPythonSnippet({.module = request.module,
                                                          .action = BuildSceneAction::actionName(),
                                                          .param = request.param})},
            }};
}

} // namespace OGL::Scene
