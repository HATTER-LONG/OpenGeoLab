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

nlohmann::json CommandDispatcher::dispatch(const nlohmann::json& request,
                                           const Core::ProgressCallback& progress) const {
    if(!request.contains("module") || !request["module"].is_string()) {
        throw std::invalid_argument("request must contain a string \"module\" field");
    }

    const auto module_name = request["module"].get<std::string>();
    LOG_INFO("CommandDispatcher: dispatching to module '{}'", module_name);

    auto module = m_factory.getSharedInstance<Core::ModuleBase>(module_name);
    return module->process(request, progress);
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
        auto module = m_factory.getSharedInstance<Core::ModuleBase>(info.m_moduleName);
        modules.push_back(module->describe());
    }

    return {{"request_schema", std::move(request_schema)}, {"modules", std::move(modules)}};
}

} // namespace OpenGeoLab::Command
