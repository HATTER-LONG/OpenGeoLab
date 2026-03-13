#include <ogl/geometry/GeometryComponentRegistration.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/geometry/PlaceholderGeometryModel.hpp>

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/logger_factory.hpp>

#include <mutex>
#include <sstream>
#include <utility>

namespace {

auto geometryLogger() {
    static auto logger = Kangaroo::Util::LoggerFactory::createLogger("OpenGeoLab.Geometry");
    return logger;
}

auto buildDescriptor(const nlohmann::json& params) -> ogl::geometry::PlaceholderGeometryDescriptor {
    return {
        .modelName = params.value("modelName", std::string{"Bracket_A01"}),
        .bodyCount = params.value("bodyCount", 3),
        .source = params.value("source", std::string{"component"}),
    };
}

auto buildEquivalentPython(const nlohmann::json& params) -> std::string {
    std::ostringstream script;
    script << "import opengeolab\n\n";
    script << "bridge = opengeolab.OpenGeoLabPythonBridge()\n";
    script << "result = bridge.call(\"geometry\", R\"JSON(" << params.dump(2) << ")JSON\")\n";
    script << "print(result)";
    return script.str();
}

class PlaceholderGeometryService final : public ogl::core::IService {
public:
    auto processRequest(const std::string& module_name, const nlohmann::json& params)
        -> ogl::core::ServiceResponse override {
        const std::string operation_name = params.value("operation", std::string{"unknown"});
        auto logger = geometryLogger();

        if(module_name != "geometry") {
            return {
                .success = false,
                .moduleName = module_name,
                .operationName = operation_name,
                .message = "Placeholder geometry service only accepts the geometry module.",
                .payload = nlohmann::json::object(),
            };
        }

        if(operation_name != "placeholderModel") {
            return {
                .success = false,
                .moduleName = module_name,
                .operationName = operation_name,
                .message = "Unsupported geometry operation. Use placeholderModel for the "
                           "scaffolded pipeline.",
                .payload = nlohmann::json::object(),
            };
        }

        const ogl::geometry::PlaceholderGeometryModel model(buildDescriptor(params));
        logger->info("Built placeholder geometry model name={} bodies={} source={}",
                     model.modelName(), model.bodyCount(), model.source());

        return {
            .success = true,
            .moduleName = module_name,
            .operationName = operation_name,
            .message =
                "Placeholder geometry request completed through the modular component pipeline.",
            .payload =
                {
                    {"componentId", "geometry"},
                    {"model", model.toJson()},
                    {"summary", model.summary()},
                    {"equivalentPython", buildEquivalentPython(params)},
                },
        };
    }
};

class GeometryServiceFactory final : public ogl::core::IServiceSingletonFactory {
public:
    auto instance() const -> tObjectSharedPtr override {
        static auto service = std::make_shared<PlaceholderGeometryService>();
        return service;
    }
};

} // namespace

namespace ogl::geometry {

void registerGeometryComponents() {
    static std::once_flag once;
    std::call_once(once, []() {
        auto logger = geometryLogger();
        g_ComponentFactory.registInstanceFactoryWithID<GeometryServiceFactory>("geometry");
        logger->info("Registered geometry component factory for placeholder service pipeline");
    });
}

} // namespace ogl::geometry