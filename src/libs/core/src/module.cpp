/**
 * @file module.cpp
 * @brief ModuleBase default implementations
 */

#include <opengeolab/core/module.hpp>

#include <opengeolab/core/logger.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <fmt/format.h>

namespace OpenGeoLab::Core {

ModuleBase::ModuleBase(std::string_view module_name,
                       std::string_view description,
                       Kangaroo::Util::PluginComponentFactory& factory)
    : m_moduleName(module_name), m_description(description), m_factory(factory) {}

ModuleBase::~ModuleBase() {
    // Release cached action shared_ptrs first so factory weak_ptrs expire.
    m_actionCache.clear();

    // Unregister all actions that were registered in the constructor.
    for(const auto& key : m_registeredActionKeys) {
        try {
            m_factory.unregisterFactory(Kangaroo::Util::PluginComponentInterfaceId<IAction>::VALUE,
                                        key);
        } catch(...) {
            LOG_WARN("ModuleBase::~ModuleBase: failed to unregister action '{}'", key);
        }
    }
}

std::string_view ModuleBase::moduleName() const { return m_moduleName; }

Kangaroo::Util::PluginComponentFactory& ModuleBase::factory() const { return m_factory; }

std::shared_ptr<IAction> ModuleBase::getAction(const std::string& key) const {
    const std::lock_guard lk(m_actionCacheMutex);
    if(auto it = m_actionCache.find(key); it != m_actionCache.end()) {
        return it->second;
    }
    auto action = m_factory.getSharedInstance<IAction>(key);
    if(action) {
        m_actionCache.emplace(key, action);
    }
    return action;
}

nlohmann::json ModuleBase::describe() const {
    const std::string prefix = m_moduleName + ".";
    auto all = m_factory.listFactories<IAction>();

    nlohmann::json actions = nlohmann::json::array();
    for(const auto& info : all) {
        if(info.m_moduleName.starts_with(prefix)) {
            auto action = getAction(info.m_moduleName);
            if(action) {
                actions.push_back(action->describe());
            }
        }
    }

    return {
        {"name", m_moduleName}, {"description", m_description}, {"actions", std::move(actions)}};
}

nlohmann::json ModuleBase::process(const nlohmann::json& request,
                                   const ProgressCallback& progress) const {
    if(!request.contains("action") || !request["action"].is_string()) {
        throw std::invalid_argument(
            fmt::format("{} request must contain a string \"action\" field", m_moduleName));
    }

    const auto action_name = request["action"].get<std::string>();
    const std::string key = m_moduleName + "." + action_name;

    std::shared_ptr<IAction> action;
    try {
        action = getAction(key);
    } catch(const Kangaroo::Util::ComponentFactoryNotRegisteredEx&) {
        throw std::invalid_argument(
            fmt::format("{} module: unknown action '{}'", m_moduleName, action_name));
    }

    if(!action) {
        throw std::invalid_argument(
            fmt::format("{} module: failed to create action '{}'", m_moduleName, action_name));
    }

    const auto param = request.contains("param") ? request["param"] : nlohmann::json::object();
    LOG_INFO("{}::process: executing action '{}'", m_moduleName, action_name);
    return action->execute(param, progress);
}

} // namespace OpenGeoLab::Core
