#include <ogl/render/RenderComponentRegistration.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/geometry/PlaceholderGeometryModel.hpp>
#include <ogl/render/PlaceholderRenderFrame.hpp>
#include <ogl/scene/PlaceholderSceneGraph.hpp>

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/logger_factory.hpp>

#include <mutex>
#include <sstream>

namespace {

auto renderLogger() {
    static auto logger = Kangaroo::Util::LoggerFactory::createLogger("OpenGeoLab.Render");
    return logger;
}

auto buildRenderEquivalentPython(const nlohmann::json& params) -> std::string {
    std::ostringstream script;
    script << "import opengeolab\n\n";
    script << "bridge = opengeolab.OpenGeoLabPythonBridge()\n";
    script << "result = bridge.call(\"render\", R\"JSON(" << params.dump(2) << ")JSON\")\n";
    script << "print(result)";
    return script.str();
}

auto buildGeometryModel(const nlohmann::json& params) -> ogl::geometry::PlaceholderGeometryModel {
    return ogl::geometry::PlaceholderGeometryModel(
        {.modelName = params.value("modelName", std::string{"Bracket_A01"}),
         .bodyCount = params.value("bodyCount", 3),
         .source = params.value("source", std::string{"render-service"})});
}

class PlaceholderRenderService final : public ogl::core::IService {
public:
    auto processRequest(const std::string& module_name, const nlohmann::json& params)
        -> ogl::core::ServiceResponse override {
        const std::string operation_name = params.value("operation", std::string{"unknown"});
        if(module_name != "render") {
            return {.success = false,
                    .moduleName = module_name,
                    .operationName = operation_name,
                    .message = "Placeholder render service only accepts the render module.",
                    .payload = nlohmann::json::object()};
        }

        if(operation_name != "buildPlaceholderFrame") {
            return {.success = false,
                    .moduleName = module_name,
                    .operationName = operation_name,
                    .message = "Unsupported render operation. Use buildPlaceholderFrame.",
                    .payload = nlohmann::json::object()};
        }

        const auto geometry_model = buildGeometryModel(params);
        const auto scene_graph = ogl::scene::buildPlaceholderSceneGraph(geometry_model);
        const auto render_frame = ogl::render::buildPlaceholderRenderFrame(scene_graph, params);

        auto logger = renderLogger();
        logger->info("Built placeholder render frame frameId={} drawItemCount={}",
                     render_frame.frameId(), render_frame.drawItems().size());

        return {.success = true,
                .moduleName = module_name,
                .operationName = operation_name,
                .message = "Placeholder render frame assembled from scene graph.",
                .payload = {{"componentId", "render"},
                            {"geometryModel", geometry_model.toJson()},
                            {"sceneGraph", scene_graph.toJson()},
                            {"renderFrame", render_frame.toJson()},
                            {"summary", render_frame.summary()},
                            {"equivalentPython", buildRenderEquivalentPython(params)}}};
    }
};

class RenderServiceFactory final : public ogl::core::IServiceSingletonFactory {
public:
    auto instance() const -> tObjectSharedPtr override {
        static auto service = std::make_shared<PlaceholderRenderService>();
        return service;
    }
};

} // namespace

namespace ogl::render {

void registerRenderComponents() {
    static std::once_flag once;
    std::call_once(once, []() {
        auto logger = renderLogger();
        g_ComponentFactory.registInstanceFactoryWithID<RenderServiceFactory>("render");
        logger->info("Registered render component factory for placeholder render-frame pipeline");
    });
}

} // namespace ogl::render