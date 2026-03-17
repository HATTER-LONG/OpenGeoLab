#include <ogl/render/BuildFrameAction.hpp>

#include <ogl/core/ActionExecutionUtilities.hpp>
#include <ogl/geometry/GeometryModel.hpp>
#include <ogl/render/RenderFrame.hpp>
#include <ogl/render/RenderLogger.hpp>
#include <ogl/scene/SceneGraph.hpp>

#include <optional>

namespace {

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
    OGL::Core::ServiceResponse early_response;
    std::optional<OGL::Geometry::GeometryModel> geometry_model;
    std::optional<OGL::Scene::SceneGraph> scene_graph;
    std::optional<OGL::Render::RenderFrame> render_frame;

    if(!OGL::Core::runProgressStage(
           request, progress_callback, 0.2, "Preparing geometry model for render frame...",
           "Render frame construction was cancelled.",
           [&]() { geometry_model = buildGeometryModel(request.param); }, early_response)) {
        return early_response;
    }

    if(!OGL::Core::runProgressStage(
           request, progress_callback, 0.55, "Building scene graph for render frame...",
           "Render frame construction was cancelled.",
           [&]() { scene_graph = OGL::Scene::buildSceneGraph(*geometry_model); }, early_response)) {
        return early_response;
    }

    if(!OGL::Core::runProgressStage(
           request, progress_callback, 0.85, "Building render draw items...",
           "Render frame construction was cancelled.",
           [&]() {
               render_frame = buildRenderFrame(*scene_graph, request.param);
               OGL_RENDER_LOG_INFO("Built render frame frameId={} drawItemCount={}",
                                   render_frame->frameId(), render_frame->drawItems().size());
           },
           early_response)) {
        return early_response;
    }

    OGL::Core::reportProgress(progress_callback, 0.95, "Render frame completed.");

    return {.success = true,
            .module = request.module,
            .action = request.action,
            .message = "Render frame assembled from scene graph.",
            .payload = {
                {"sceneGraph", scene_graph->toJson()},
                {"renderFrame", render_frame->toJson()},
                {"summary", render_frame->summary()},
                {"equivalentPython",
                 OGL::Core::buildEquivalentPythonSnippet({.module = request.module,
                                                          .action = BuildFrameAction::actionName(),
                                                          .param = request.param})},
            }};
}

} // namespace OGL::Render
