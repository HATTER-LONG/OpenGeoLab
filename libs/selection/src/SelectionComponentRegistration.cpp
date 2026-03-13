#include <ogl/selection/SelectionComponentRegistration.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/geometry/PlaceholderGeometryModel.hpp>
#include <ogl/render/PlaceholderRenderFrame.hpp>
#include <ogl/scene/PlaceholderSceneGraph.hpp>
#include <ogl/selection/PlaceholderSelectionResult.hpp>

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/logger_factory.hpp>

#include <mutex>
#include <sstream>

namespace {

auto selectionLogger() {
    static auto logger = Kangaroo::Util::LoggerFactory::createLogger("OpenGeoLab.Selection");
    return logger;
}

auto buildSelectionEquivalentPython(const nlohmann::json& params) -> std::string {
    std::ostringstream script;
    script << "import opengeolab\n\n";
    script << "bridge = opengeolab.OpenGeoLabPythonBridge()\n";
    script << "result = bridge.call(\"selection\", R\"JSON(" << params.dump(2) << ")JSON\")\n";
    script << "print(result)";
    return script.str();
}

auto buildGeometryModel(const nlohmann::json& params) -> ogl::geometry::PlaceholderGeometryModel {
    return ogl::geometry::PlaceholderGeometryModel(
        {.modelName = params.value("modelName", std::string{"Bracket_A01"}),
         .bodyCount = params.value("bodyCount", 3),
         .source = params.value("source", std::string{"selection-service"})});
}

auto normalizeSelectionParams(const std::string& operation_name, const nlohmann::json& params)
    -> nlohmann::json {
    nlohmann::json normalized = params;
    if(operation_name == "boxSelectPlaceholder") {
        normalized["mode"] = "box";
        if(!normalized.contains("selectionCount")) {
            normalized["selectionCount"] = 2;
        }
    } else {
        normalized["mode"] = "pick";
    }
    return normalized;
}

class PlaceholderSelectionService final : public ogl::core::IService {
public:
    auto processRequest(const std::string& module_name, const nlohmann::json& params)
        -> ogl::core::ServiceResponse override {
        const std::string operation_name = params.value("operation", std::string{"unknown"});
        if(module_name != "selection") {
            return {.success = false,
                    .moduleName = module_name,
                    .operationName = operation_name,
                    .message = "Placeholder selection service only accepts the selection module.",
                    .payload = nlohmann::json::object()};
        }

        if(operation_name != "pickPlaceholderEntity" && operation_name != "boxSelectPlaceholder") {
            return {.success = false,
                    .moduleName = module_name,
                    .operationName = operation_name,
                    .message = "Unsupported selection operation. Use pickPlaceholderEntity or "
                               "boxSelectPlaceholder.",
                    .payload = nlohmann::json::object()};
        }

        const auto normalized_params = normalizeSelectionParams(operation_name, params);
        const auto geometry_model = buildGeometryModel(normalized_params);
        const auto scene_graph = ogl::scene::buildPlaceholderSceneGraph(geometry_model);
        const auto render_frame =
            ogl::render::buildPlaceholderRenderFrame(scene_graph, normalized_params);
        const auto selection_result = ogl::selection::evaluatePlaceholderSelection(
            scene_graph, render_frame, normalized_params);

        auto logger = selectionLogger();
        logger->info("Resolved placeholder selection mode={} hitCount={}", selection_result.mode(),
                     selection_result.hits().size());

        return {
            .success = true,
            .moduleName = module_name,
            .operationName = operation_name,
            .message = "Placeholder selection completed through geometry, scene, and render data.",
            .payload = {{"componentId", "selection"},
                        {"geometryModel", geometry_model.toJson()},
                        {"sceneGraph", scene_graph.toJson()},
                        {"renderFrame", render_frame.toJson()},
                        {"selectionResult", selection_result.toJson()},
                        {"summary", selection_result.summary()},
                        {"equivalentPython", buildSelectionEquivalentPython(normalized_params)}}};
    }
};

class SelectionServiceFactory final : public ogl::core::IServiceSingletonFactory {
public:
    auto instance() const -> tObjectSharedPtr override {
        static auto service = std::make_shared<PlaceholderSelectionService>();
        return service;
    }
};

} // namespace

namespace ogl::selection {

void registerSelectionComponents() {
    static std::once_flag once;
    std::call_once(once, []() {
        auto logger = selectionLogger();
        g_ComponentFactory.registInstanceFactoryWithID<SelectionServiceFactory>("selection");
        logger->info("Registered selection component factory for placeholder interaction pipeline");
    });
}

} // namespace ogl::selection