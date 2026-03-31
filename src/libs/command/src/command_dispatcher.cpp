/**
 * @file command_dispatcher.cpp
 * @brief CommandDispatcher implementation
 */

#include <opengeolab/command/command_dispatcher.hpp>

#include <opengeolab/core/logger.hpp>

#include <stdexcept>

namespace OpenGeoLab::Command {

CommandDispatcher::CommandDispatcher(Kangaroo::Util::PluginComponentFactory& factory)
    : m_factory(factory) {}

CommandDispatcher::~CommandDispatcher() = default;

std::shared_ptr<Core::ModuleBase> CommandDispatcher::getModule(const std::string& name) const {
    const std::lock_guard lk(m_cacheMutex);
    if(auto it = m_moduleCache.find(name); it != m_moduleCache.end()) {
        return it->second;
    }
    auto module = m_factory.getSharedInstance<Core::ModuleBase>(name);
    if(module) {
        m_moduleCache.emplace(name, module);
    }
    return module;
}

nlohmann::json CommandDispatcher::dispatch(const nlohmann::json& request,
                                           const Core::ProgressCallback& progress) const {
    if(!request.contains("module") || !request["module"].is_string()) {
        return {{"ok", false},
                {"summary", "Request must contain a string \"module\" field"},
                {"errors", nlohmann::json::array({"Missing or invalid 'module' field"})}};
    }

    const auto module_name = request["module"].get<std::string>();
    LOG_INFO("CommandDispatcher: dispatching to module '{}'", module_name);

    if(!hasModule(module_name)) {
        LOG_WARN("CommandDispatcher: module '{}' is not registered", module_name);
        return {{"ok", false},
                {"summary", "Module '" + module_name + "' is not registered."},
                {"errors",
                 nlohmann::json::array({"No module named '" + module_name + "' is available."})}};
    }

    try {
        auto module = getModule(module_name);
        if(!module) {
            LOG_ERROR("CommandDispatcher: failed to create module '{}'", module_name);
            return {{"ok", false},
                    {"summary", "Module '" + module_name + "' could not be created."},
                    {"errors", nlohmann::json::array(
                                   {"Factory returned null for module '" + module_name + "'."})}};
        }
        return module->process(request, progress);
    } catch(const std::exception& e) {
        LOG_ERROR("CommandDispatcher: module '{}' threw: {}", module_name, e.what());
        return {{"ok", false},
                {"summary", "Module '" + module_name + "' threw an exception."},
                {"errors", nlohmann::json::array({std::string(e.what())})}};
    }
}

bool CommandDispatcher::hasModule(std::string_view module_name) const {
    auto factories = m_factory.listFactories<Core::ModuleBase>();
    for(const auto& info : factories) {
        if(info.m_moduleName == module_name) {
            return true;
        }
    }
    return false;
}

std::vector<Kangaroo::Util::FactoryInfo> CommandDispatcher::listModules() const {
    return m_factory.listFactories<Core::ModuleBase>();
}

nlohmann::json CommandDispatcher::describe() const {
    nlohmann::json request_schema = {
        {"type", "object"},
        {"properties",
         {{"module", {{"type", "string"}, {"description", "Target module name"}}},
          {"action", {{"type", "string"}, {"description", "Action to invoke within the module"}}},
          {"param",
           {{"type", "object"},
            {"description", "Action-specific parameters (see each action's params schema)"}}}}}};

    nlohmann::json modules = nlohmann::json::array();
    for(const auto& info : m_factory.listFactories<Core::ModuleBase>()) {
        auto module = getModule(info.m_moduleName);
        if(module) {
            modules.push_back(module->describe());
        }
    }

    return {{"request_schema", std::move(request_schema)}, {"modules", std::move(modules)}};
}

std::shared_ptr<Core::ModuleBase>
CommandDispatcher::findModule(const std::string& module_name) const {
    if(!hasModule(module_name)) {
        return nullptr;
    }
    return getModule(module_name);
}

Kangaroo::Util::ScopedConnection
CommandDispatcher::onModuleDataChanged(const std::string& module_name,
                                       std::function<void(Core::ModuleDataEvent)> callback) {
    auto module = findModule(module_name);
    if(!module) {
        return {};
    }
    return module->dataChanged.connect(std::move(callback));
}

} // namespace OpenGeoLab::Command
