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

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

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
     * @return Response JSON; on error returns {"ok": false, "summary": "...", "errors": [...]}
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

    /**
     * @brief Look up a registered module by name.
     * @param module_name Module name to look up
     * @return Shared pointer to the module; nullptr if not registered
     */
    [[nodiscard]] std::shared_ptr<Core::ModuleBase>
    findModule(const std::string& module_name) const;

private:
    /// Retrieve or cache a module singleton, keeping it alive across dispatches.
    [[nodiscard]] std::shared_ptr<Core::ModuleBase> getModule(const std::string& name) const;

    Kangaroo::Util::PluginComponentFactory& m_factory;
    mutable std::mutex m_cacheMutex;
    mutable std::unordered_map<std::string, std::shared_ptr<Core::ModuleBase>> m_moduleCache;
};

} // namespace OpenGeoLab::Command
