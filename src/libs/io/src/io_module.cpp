/**
 * @file io_module.cpp
 * @brief IOModule implementation with action dispatch
 */

#include <opengeolab/io/io_module.hpp>
#include <opengeolab/io/read_brep_action.hpp>

#include <opengeolab/core/logger.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <fmt/format.h>

#include <stdexcept>

namespace OpenGeoLab::IO {

IOModule::IOModule() { registerAction(std::make_unique<ReadBrepAction>()); }

IOModule::~IOModule() = default;

nlohmann::json IOModule::describe() const {
    nlohmann::json actions = nlohmann::json::array();
    for(const auto& [name, action] : m_actions) {
        actions.push_back(action->describe());
    }
    return {{"name", MODULE_NAME},
            {"description", "I/O module for reading and writing geometry files."},
            {"actions", std::move(actions)}};
}

nlohmann::json IOModule::process(const nlohmann::json& request,
                                 const Core::ProgressCallback& progress) {
    if(!request.contains("action") || !request["action"].is_string()) {
        throw std::invalid_argument("IO request must contain a string \"action\" field");
    }

    const auto action_name = request["action"].get<std::string>();
    auto it = m_actions.find(action_name);
    if(it == m_actions.end()) {
        throw std::invalid_argument(fmt::format("IO module: unknown action '{}'", action_name));
    }

    const auto param = request.contains("param") ? request["param"] : nlohmann::json::object();
    LOG_INFO("IOModule: executing action '{}'", action_name);
    return it->second->execute(param, progress);
}

void IOModule::registerAction(std::unique_ptr<Core::IAction> action) {
    if(!action) {
        throw std::invalid_argument("Cannot register a null action");
    }

    auto desc = action->describe();
    if(!desc.contains("name") || !desc["name"].is_string()) {
        throw std::invalid_argument("Action describe() must return a JSON object with a string "
                                    "\"name\" field");
    }

    auto name = desc["name"].get<std::string>();
    if(name.empty()) {
        throw std::invalid_argument("Action name must not be empty");
    }

    auto [it, inserted] = m_actions.emplace(std::move(name), std::move(action));
    if(!inserted) {
        throw std::invalid_argument(fmt::format("Duplicate action name '{}'", it->first));
    }
}

void IOModule::registerModule() {
    g_PluginComponentFactory.bindSingleton<Core::ModuleBase, IOModule>(MODULE_NAME);
}

} // namespace OpenGeoLab::IO
