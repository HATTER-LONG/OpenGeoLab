/**
 * @file module_dispatcher.hpp
 * @brief Declares the dispatcher that routes protocol requests to module services.
 */

#pragma once

#include <opengeolab/base/module_service_interface.hpp>
#include <opengeolab/command/command_export.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Command {

using OpenGeoLab::Base::IModuleService;

/**
 * @brief Routes protocol requests to registered module services.
 *
 * Caches resolved module services to keep Kangaroo singletons alive
 * across dispatch calls (the factory uses weak_ptr internally).
 */
class OPENGEOLAB_COMMAND_EXPORT ModuleDispatcher {
public:
    /**
     * @brief Creates a dispatcher backed by the provided plugin component factory.
     * @param factory Factory used to resolve module services by module name.
     */
    explicit ModuleDispatcher(Kangaroo::Util::PluginComponentFactory& factory);

    /**
     * @brief Dispatches a JSON request string and returns a JSON response string.
     * @param request_json Request envelope containing module, action, payload, and optional
     * requestId.
     * @return Serialized JSON response following the module command protocol envelope.
     */
    [[nodiscard]] auto dispatch(std::string_view request_json) -> std::string;

    /**
     * @brief Returns the module names successfully resolved by this dispatcher instance.
     * @return Module names cached from successful dispatch calls in first-seen order.
     */
    [[nodiscard]] auto registeredModules() const -> std::vector<std::string>;

private:
    [[nodiscard]] auto resolveModule(const std::string& module_name)
        -> std::shared_ptr<IModuleService>;

    Kangaroo::Util::PluginComponentFactory& m_factory;
    std::vector<std::string> m_registeredModules;
    std::unordered_map<std::string, std::shared_ptr<IModuleService>> m_moduleCache;
};

} // namespace OpenGeoLab::Command
