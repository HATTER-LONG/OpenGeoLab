#include <ogl/python_wrapper/OpenGeoLabPythonBridge.hpp>

#include <ogl/command/CommandService.hpp>

#include <stdexcept>

namespace OGL::PythonWrapper {

namespace {

auto parseCommandRequest(const nlohmann::json& request_json) -> OGL::Command::CommandRequest {
    if(!request_json.is_object()) {
        throw std::invalid_argument("Request payload must be a JSON object.");
    }

    const std::string module = request_json.value("module", std::string{});
    if(module.empty()) {
        throw std::invalid_argument("Request module cannot be empty.");
    }

    const std::string action = request_json.value("action", std::string{});
    if(action.empty()) {
        throw std::invalid_argument("Request action cannot be empty.");
    }

    nlohmann::json param =
        request_json.contains("param") ? request_json.at("param") : nlohmann::json::object();
    if(param.is_null()) {
        param = nlohmann::json::object();
    }
    if(!param.is_object()) {
        throw std::invalid_argument("Request param must be a JSON object.");
    }

    return {.module = module, .action = action, .param = std::move(param)};
}

} // namespace

OpenGeoLabPythonBridge::OpenGeoLabPythonBridge() = default;

auto OpenGeoLabPythonBridge::process(const nlohmann::json& request_json) const -> nlohmann::json {
    const auto request = parseCommandRequest(request_json);
    const OGL::Command::CommandService command_service;
    return command_service
        .execute({.module = request.module, .action = request.action, .param = request.param})
        .toJson();
}

} // namespace OGL::PythonWrapper
