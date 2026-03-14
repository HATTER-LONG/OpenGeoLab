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
     * @param module_name Logical module identifier.
     * @param params Request payload.
     * @return Structured response describing success, summary, and payload.
     */
    static auto dispatch(const std::string& module_name, const nlohmann::json& params)
        -> ServiceResponse;
};

} // namespace OGL::Core