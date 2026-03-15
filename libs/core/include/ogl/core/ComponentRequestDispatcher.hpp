/**
 * @file ComponentRequestDispatcher.hpp
 * @brief Dispatches OpenGeoLab requests through Kangaroo ComponentFactory.
 */

#pragma once

#include <ogl/core/IService.hpp>
#include <ogl/core/export.hpp>

namespace OGL::Core {

/**
 * @brief Central entry point for component-based service requests.
 */
class OGL_CORE_EXPORT ComponentRequestDispatcher {
public:
    /**
     * @brief Resolve a module service and execute its request.
     * @param request Structured request containing module, action, and param.
     * @param progress_callback Optional callback for intermediate progress updates.
     * @return Structured response describing success, summary, and payload.
     */
    static auto dispatch(const ServiceRequest& request,
                         const ProgressCallback& progress_callback = {}) -> ServiceResponse;
    static auto dispatch(const std::string& module,
                         const std::string& action,
                         const nlohmann::json& param,
                         const ProgressCallback& progress_callback = {}) -> ServiceResponse;
};

} // namespace OGL::Core
