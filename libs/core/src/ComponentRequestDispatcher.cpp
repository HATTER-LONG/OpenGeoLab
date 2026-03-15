#include <ogl/core/ComponentDispatcherLogger.hpp>
#include <ogl/core/ComponentRequestDispatcher.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <exception>

namespace OGL::Core {

auto ComponentRequestDispatcher::dispatch(const std::string& module,
                                          const std::string& action,
                                          const nlohmann::json& param,
                                          const ProgressCallback& progress_callback)
    -> ServiceResponse {
    return dispatch(ServiceRequest{.module = module, .action = action, .param = param},
                    progress_callback);
}

auto ComponentRequestDispatcher::dispatch(const ServiceRequest& request,
                                          const ProgressCallback& progress_callback)
    -> ServiceResponse {
    const std::string action_name =
        request.action.empty() ? std::string{"unknown"} : request.action;
    if(request.module.empty()) {
        return {.success = false,
                .module = std::string{},
                .action = action_name,
                .message = "Component request module cannot be empty.",
                .payload = nlohmann::json::object()};
    }

    if(!request.param.is_object()) {
        return {.success = false,
                .module = request.module,
                .action = action_name,
                .message = "Component request param must be a JSON object.",
                .payload = nlohmann::json::object()};
    }

    try {
        OGL_COMPONENT_DISPATCH_LOG_DEBUG("Dispatching component request module={} action={}",
                                         request.module, action_name);

        auto service =
            g_ComponentFactory.getInstanceObjectWithID<IServiceSingletonFactory>(request.module);
        if(!service) {
            return {
                .success = false,
                .module = request.module,
                .action = action_name,
                .message = "Component factory resolved a null service instance.",
                .payload = nlohmann::json::object(),
            };
        }

        return service->processRequest(
            {.module = request.module, .action = action_name, .param = request.param},
            progress_callback);
    } catch(const std::exception& ex) {
        OGL_COMPONENT_DISPATCH_LOG_ERROR(
            "Component dispatch failed for module={} action={} error={}", request.module,
            action_name, ex.what());
        return {
            .success = false,
            .module = request.module,
            .action = action_name,
            .message = ex.what(),
            .payload = nlohmann::json::object(),
        };
    }
}

} // namespace OGL::Core
