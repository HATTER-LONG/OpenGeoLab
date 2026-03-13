#include <ogl/core/ComponentRequestDispatcher.hpp>

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/logger_factory.hpp>

#include <exception>

namespace {

auto componentDispatcherLogger() {
    static auto logger =
        Kangaroo::Util::LoggerFactory::createLogger("OpenGeoLab.ComponentDispatcher");
    return logger;
}

} // namespace

namespace ogl::core {

auto ComponentRequestDispatcher::dispatch(const std::string& module_name,
                                          const nlohmann::json& params) -> ServiceResponse {
    auto logger = componentDispatcherLogger();
    const std::string operation_name = params.value("operation", std::string{"unknown"});

    try {
        logger->info("Dispatching component request module={} operation={}", module_name,
                     operation_name);

        auto service =
            g_ComponentFactory.getInstanceObjectWithID<IServiceSingletonFactory>(module_name);
        if(!service) {
            return {
                .success = false,
                .moduleName = module_name,
                .operationName = operation_name,
                .message = "Component factory resolved a null service instance.",
                .payload = nlohmann::json::object(),
            };
        }

        return service->processRequest(module_name, params);
    } catch(const std::exception& ex) {
        logger->error("Component dispatch failed for module={} operation={} error={}", module_name,
                      operation_name, ex.what());
        return {
            .success = false,
            .moduleName = module_name,
            .operationName = operation_name,
            .message = ex.what(),
            .payload = nlohmann::json::object(),
        };
    }
}

} // namespace ogl::core