#include <ogl/python_wrapper/OpenGeoLabPythonBridge.hpp>

#include <ogl/command/RequestProtocol.hpp>
#include <ogl/command/CommandService.hpp>

namespace OGL::PythonWrapper {

OpenGeoLabPythonBridge::OpenGeoLabPythonBridge() = default;

auto OpenGeoLabPythonBridge::process(const nlohmann::json& request_json) const -> nlohmann::json {
    const auto request = OGL::Command::parseCommandRequest(request_json);
    const OGL::Command::CommandService command_service;
    return command_service
        .execute({.module = request.module, .action = request.action, .param = request.param})
        .toJson();
}

} // namespace OGL::PythonWrapper
