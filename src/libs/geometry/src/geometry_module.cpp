/**
 * @file geometry_module.cpp
 * @brief GeometryModule implementation with action dispatch
 */

#include <opengeolab/geometry/create_box_action.hpp>
#include <opengeolab/geometry/geometry_module.hpp>

#include <opengeolab/core/logger.hpp>

#include <fmt/format.h>

#include <stdexcept>

namespace OpenGeoLab::Geometry {

GeometryModule::GeometryModule() { registerAction(std::make_unique<CreateBoxAction>()); }

GeometryModule::~GeometryModule() = default;

nlohmann::json GeometryModule::describe() const {
    nlohmann::json actions = nlohmann::json::array();
    for(const auto& [name, action] : m_actions) {
        actions.push_back(action->describe());
    }
    return {{"name", MODULE_NAME},
            {"description", "Geometry creation and manipulation module."},
            {"actions", std::move(actions)}};
}

nlohmann::json GeometryModule::process(const nlohmann::json& request,
                                       const Core::ProgressCallback& progress) {
    if(!request.contains("action") || !request["action"].is_string()) {
        throw std::invalid_argument("Geometry request must contain a string \"action\" field");
    }

    const auto action_name = request["action"].get<std::string>();
    auto it = m_actions.find(action_name);
    if(it == m_actions.end()) {
        throw std::invalid_argument(
            fmt::format("Geometry module: unknown action '{}'", action_name));
    }

    const auto param = request.contains("param") ? request["param"] : nlohmann::json::object();
    LOG_INFO("GeometryModule: executing action '{}'", action_name);
    return it->second->execute(param, progress);
}

void GeometryModule::registerAction(std::unique_ptr<Core::IAction> action) {
    if(!action) {
        throw std::invalid_argument("Cannot register a null action");
    }

    auto desc = action->describe();
    if(!desc.contains("name") || !desc["name"].is_string()) {
        throw std::invalid_argument(
            "Action describe() must return a JSON object with a string \"name\" field");
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

} // namespace OpenGeoLab::Geometry
