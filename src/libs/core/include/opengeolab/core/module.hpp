/**
 * @file module.hpp
 * @brief ModuleBase — base class for all command modules
 *
 * Modules register their actions into the global PluginComponentFactory via
 * the protected registerAction<T>() helper.  The default process() and
 * describe() implementations delegate to factory-managed IAction singletons.
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/core/core_export.hpp>
#include <opengeolab/core/progress_callback.hpp>

#include <kangaroo/util/noncopyable.hpp>
#include <kangaroo/util/plugin_component_factory.hpp>
#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

namespace OpenGeoLab::Core {

/**
 * @brief Base class for all modules in OpenGeoLab.
 *
 * Holds module name, description and a reference to the component factory.
 * Provides default process() and describe() that look up IAction singletons
 * registered under "module.action" keys.  Subclasses may override if they
 * need custom dispatch logic.
 */
class OPENGEOLAB_CORE_EXPORT ModuleBase : public Kangaroo::Util::NonCopyMoveable {
public:
    /**
     * @brief Construct a module.
     * @param module_name  Identifier that matches the "module" field in requests
     * @param description  Human / LLM-readable summary of this module
     * @param factory      Component factory where actions are registered
     */
    explicit ModuleBase(std::string_view module_name,
                        std::string_view description,
                        Kangaroo::Util::PluginComponentFactory& factory);
    virtual ~ModuleBase();

    /// Module name (matches request["module"]).
    [[nodiscard]] std::string_view moduleName() const;

    /**
     * @brief Return a JSON description of this module for LLM / tooling.
     *
     * Default implementation enumerates all IAction entries whose factory key
     * starts with "moduleName." and collects their describe() output.
     */
    [[nodiscard]] virtual nlohmann::json describe() const;

    /**
     * @brief Dispatch a request to the matching action.
     *
     * Default implementation extracts request["action"], constructs the factory
     * key "moduleName.action", and delegates to IAction::execute().
     *
     * @param request  JSON with "action" and "param" fields
     * @param progress Callback for reporting progress
     * @return Response JSON from the action
     * @throws std::invalid_argument if "action" field is missing or unknown
     */
    [[nodiscard]] virtual nlohmann::json process(const nlohmann::json& request,
                                                 const ProgressCallback& progress) const;

protected:
    /**
     * @brief Register an action type into the factory.
     *
     * ActionT must provide `static constexpr std::string_view ACTION_NAME`.
     * The factory key will be "moduleName.ACTION_NAME".
     *
     * @tparam ActionT Concrete action class to register as singleton
     */
    template <class ActionT> void registerAction();

    /// Access the component factory (for subclasses that need custom logic).
    [[nodiscard]] Kangaroo::Util::PluginComponentFactory& factory() const;

private:
    std::string m_moduleName;
    std::string m_description;
    Kangaroo::Util::PluginComponentFactory& m_factory;
};

template <class ActionT> void ModuleBase::registerAction() {
    std::string key = m_moduleName + "." + std::string(ActionT::ACTION_NAME);
    m_factory.bindSingleton<IAction, ActionT>(key);
}

} // namespace OpenGeoLab::Core

template <> struct Kangaroo::Util::PluginComponentInterfaceId<OpenGeoLab::Core::ModuleBase> {
    static constexpr std::string_view VALUE{"opengeolab.core.ModuleBase"};
};