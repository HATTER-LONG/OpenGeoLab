/**
 * @file command_dispatcher.hpp
 * @brief CommandDispatcher — dispatches JSON requests to named ModuleBase instances
 *
 * Extracts the "module" field from a JSON request, looks up the corresponding
 * ModuleBase in PluginComponentFactory, and forwards the request.
 *
 * Expected request format:
 * @code
 * {
 *   "module": "<module_name>",
 *   "action": "<action_name>",
 *   "param":  { ... }
 * }
 * @endcode
 */

#pragma once

#include <opengeolab/command/command_export.hpp>
#include <opengeolab/core/module.hpp>
#include <opengeolab/core/progress_callback.hpp>

#include <kangaroo/util/noncopyable.hpp>
#include <kangaroo/util/plugin_component_factory.hpp>
#include <nlohmann/json.hpp>

#include <string_view>

namespace OpenGeoLab::Command {

/**
 * @brief Dispatches JSON commands to registered ModuleBase instances.
 *
 * The "module" field in the request JSON selects which ModuleBase to invoke.
 *
 * Example:
 * @code
 * CommandDispatcher dispatcher(g_PluginComponentFactory);
 * nlohmann::json req = {{"module", "io"}, {"action", "read_brep"}, {"param", {{"path",
 * "a.brep"}}}}; auto result = dispatcher.dispatch(req, progress);
 * @endcode
 */
class OPENGEOLAB_COMMAND_EXPORT CommandDispatcher final : public Kangaroo::Util::NonCopyMoveable {
public:
    /**
     * @brief Construct with a reference to the component factory.
     * @param factory Factory containing registered ModuleBase instances
     */
    explicit CommandDispatcher(Kangaroo::Util::PluginComponentFactory& factory);
    ~CommandDispatcher();

    /**
     * @brief Dispatch a request to the module named in request["module"].
     * @param request JSON with "module", "action", "param" fields
     * @param progress Callback for reporting progress
     * @return Response JSON from the module
     * @throws std::invalid_argument if "module" field is missing
     * @throws Kangaroo::Util::ComponentFactoryNotRegisteredEx if module not found
     */
    [[nodiscard]] nlohmann::json dispatch(const nlohmann::json& request,
                                          const Core::ProgressCallback& progress) const;

    /**
     * @brief Check whether a module with the given name is registered.
     * @param module_name Module name to look up
     * @return true if a ModuleBase singleton is registered under that name
     */
    [[nodiscard]] bool hasModule(std::string_view module_name) const;

    /**
     * @brief List all registered module names.
     * @return Vector of (moduleName, lifetime) info structs
     */
    [[nodiscard]] std::vector<Kangaroo::Util::FactoryInfo> listModules() const;

    /**
     * @brief Describe the entire command system for LLM / UI auto-discovery.
     *
     * Returns a JSON object containing:
     * - "request_schema": the expected request format
     * - "modules": array of per-module descriptions (from ModuleBase::describe())
     *
     * @return JSON system description
     */
    [[nodiscard]] nlohmann::json describe() const;

private:
    Kangaroo::Util::PluginComponentFactory& m_factory;
};

} // namespace OpenGeoLab::Command
