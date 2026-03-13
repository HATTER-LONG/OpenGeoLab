#include <ogl/scene/SceneComponentRegistration.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/geometry/PlaceholderGeometryModel.hpp>
#include <ogl/scene/PlaceholderSceneGraph.hpp>

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/logger_factory.hpp>

#include <mutex>
#include <sstream>

namespace {

auto sceneLogger() {
    static auto logger = Kangaroo::Util::LoggerFactory::createLogger("OpenGeoLab.Scene");
    return logger;
}

auto buildSceneEquivalentPython(const nlohmann::json& params) -> std::string {
    std::ostringstream script;
    script << "import opengeolab\n\n";
    script << "bridge = opengeolab.OpenGeoLabPythonBridge()\n";
    script << "result = bridge.call(\"scene\", R\"JSON(" << params.dump(2) << ")JSON\")\n";
    script << "print(result)";
    return script.str();
}

auto buildGeometryModel(const nlohmann::json& params) -> ogl::geometry::PlaceholderGeometryModel {
    return ogl::geometry::PlaceholderGeometryModel(
        {.modelName = params.value("modelName", std::string{"Bracket_A01"}),
         .bodyCount = params.value("bodyCount", 3),
         .source = params.value("source", std::string{"scene-service"})});
}

class PlaceholderSceneService final : public ogl::core::IService {
public:
    auto processRequest(const std::string& module_name, const nlohmann::json& params)
        -> ogl::core::ServiceResponse override {
        const std::string operation_name = params.value("operation", std::string{"unknown"});
        if(module_name != "scene") {
            return {.success = false,
                    .moduleName = module_name,
                    .operationName = operation_name,
                    .message = "Placeholder scene service only accepts the scene module.",
                    .payload = nlohmann::json::object()};
        }

        if(operation_name != "buildPlaceholderScene") {
            return {.success = false,
                    .moduleName = module_name,
                    .operationName = operation_name,
                    .message = "Unsupported scene operation. Use buildPlaceholderScene.",
                    .payload = nlohmann::json::object()};
        }

        const auto geometry_model = buildGeometryModel(params);
        const auto scene_graph = ogl::scene::buildPlaceholderSceneGraph(geometry_model);
        auto logger = sceneLogger();
        logger->info("Built placeholder scene graph sceneId={} nodeCount={}", scene_graph.sceneId(),
                     scene_graph.nodes().size());

        return {.success = true,
                .moduleName = module_name,
                .operationName = operation_name,
                .message = "Placeholder scene graph assembled from geometry model.",
                .payload = {{"componentId", "scene"},
                            {"geometryModel", geometry_model.toJson()},
                            {"sceneGraph", scene_graph.toJson()},
                            {"summary", scene_graph.summary()},
                            {"equivalentPython", buildSceneEquivalentPython(params)}}};
    }
};

class SceneServiceFactory final : public ogl::core::IServiceSingletonFactory {
public:
    auto instance() const -> tObjectSharedPtr override {
        static auto service = std::make_shared<PlaceholderSceneService>();
        return service;
    }
};

} // namespace

namespace ogl::scene {

void registerSceneComponents() {
    static std::once_flag once;
    std::call_once(once, []() {
        auto logger = sceneLogger();
        g_ComponentFactory.registInstanceFactoryWithID<SceneServiceFactory>("scene");
        logger->info("Registered scene component factory for placeholder scene graph pipeline");
    });
}

} // namespace ogl::scene