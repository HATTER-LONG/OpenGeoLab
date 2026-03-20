#include <opengeolab/command/module_dispatcher.hpp>

#include <opengeolab/command/module_service_interface.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <exception>
#include <string>
#include <utility>

namespace OpenGeoLab::Command {
namespace {

constexpr std::string_view PROTOCOL_VERSION = "1.0";

auto makeErrorItem(std::string_view message) -> nlohmann::json {
    return nlohmann::json{{"message", message}};
}

auto makeResponse(const nlohmann::json& request_id,
                  const nlohmann::json& module,
                  const nlohmann::json& action,
                  bool ok,
                  std::string_view summary,
                  const nlohmann::json& result,
                  const nlohmann::json& errors) -> std::string {
    return nlohmann::json{{"protocolVersion", PROTOCOL_VERSION},
                          {"requestId", request_id},
                          {"ok", ok},
                          {"module", module},
                          {"action", action},
                          {"summary", summary},
                          {"result", result},
                          {"errors", errors}}
        .dump();
}

auto makeErrorResponse(const nlohmann::json& request_id,
                       const nlohmann::json& module,
                       const nlohmann::json& action,
                       std::string_view message) -> std::string {
    return makeResponse(request_id, module, action, false, message, nlohmann::json::object(),
                        nlohmann::json::array({makeErrorItem(message)}));
}

} // namespace

ModuleDispatcher::ModuleDispatcher(Kangaroo::Util::PluginComponentFactory& factory) : m_factory(factory) {}

auto ModuleDispatcher::dispatch(std::string_view request_json) -> std::string {
    nlohmann::json request_id = nullptr;
    nlohmann::json module = nullptr;
    nlohmann::json action = nullptr;

    try {
        const nlohmann::json request = nlohmann::json::parse(request_json);
        if(!request.is_object()) {
            return makeErrorResponse(request_id, module, action,
                                     "Request envelope must be a JSON object.");
        }

        request_id =
            request.contains("requestId") ? request.at("requestId") : nlohmann::json(nullptr);
        module = request.contains("module") ? request.at("module") : nlohmann::json(nullptr);
        action = request.contains("action") ? request.at("action") : nlohmann::json(nullptr);

        if(!module.is_string()) {
            if(module.is_null()) {
                return makeErrorResponse(request_id, module, action, "Missing 'module' field");
            }
            return makeErrorResponse(request_id, module, action, "Request module must be a string.");
        }

        if(!action.is_string()) {
            return makeErrorResponse(request_id, module, action, "Request action must be a string.");
        }

        const auto payload =
            request.contains("payload") ? request.at("payload") : nlohmann::json::object();
        const std::string module_name = module.get<std::string>();
        const std::string action_name = action.get<std::string>();

        std::shared_ptr<IModuleService> module_service;
        try {
            module_service = m_factory.getSharedInstance<IModuleService>(module_name);
        } catch(const Kangaroo::Util::ComponentFactoryNotRegisteredEx&) {
            const std::string message = "Unknown module: " + module_name;
            return makeErrorResponse(request_id, module_name, action_name, message);
        }

        if(module_service == nullptr) {
            const std::string message = "Unknown module: " + module_name;
            return makeErrorResponse(request_id, module_name, action_name, message);
        }

        const CommandResult command_result = module_service->dispatch(action_name, payload);
        rememberModuleName(module_name);

        return makeResponse(request_id, module_name, action_name, command_result.ok,
                            command_result.summary, command_result.result,
                            nlohmann::json::array());
    } catch(const nlohmann::json::exception& exception) {
        return makeErrorResponse(request_id, module, action, exception.what());
    } catch(const std::exception& exception) {
        return makeErrorResponse(request_id, module, action, exception.what());
    }
}

auto ModuleDispatcher::registeredModules() const -> std::vector<std::string> {
    return m_registeredModules;
}

void ModuleDispatcher::rememberModuleName(std::string_view module_name) {
    if(std::ranges::find(m_registeredModules, module_name) == m_registeredModules.end()) {
        m_registeredModules.emplace_back(module_name);
    }
}

} // namespace OpenGeoLab::Command
