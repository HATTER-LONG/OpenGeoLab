#include <ogl/python_wrapper/OpenGeoLabPythonBridge.hpp>

#include <ogl/command/CommandService.hpp>

namespace OGL::PythonWrapper {

OpenGeoLabPythonBridge::OpenGeoLabPythonBridge() = default;

auto OpenGeoLabPythonBridge::call(const std::string& module_name,
                                  const nlohmann::json& params) const -> nlohmann::json {
    const OGL::Command::CommandService command_service;
    return command_service
        .execute(OGL::Command::CommandRequest{.moduleName = module_name, .params = params})
        .toJson();
}

auto OpenGeoLabPythonBridge::suggestPlaceholderGeometryScript(const std::string& model_name,
                                                              int body_count) const -> std::string {
    const nlohmann::json request = {
        {"operation", "placeholderModel"},
        {"modelName", model_name},
        {"bodyCount", body_count},
        {"source", "python"},
    };

    return call("geometry", request)
        .value("payload", nlohmann::json::object())
        .value("equivalentPython", std::string{});
}

} // namespace OGL::PythonWrapper