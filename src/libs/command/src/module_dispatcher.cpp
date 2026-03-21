#include <opengeolab/command/module_dispatcher.hpp>

#include <opengeolab/base/protocol_constants.hpp>
#include <opengeolab/base/response_builder.hpp>

#include <nlohmann/json.hpp>

#include <exception>
#include <string>
#include <utility>

namespace OpenGeoLab::Command {

using OpenGeoLab::Base::CommandResult;

ModuleDispatcher::ModuleDispatcher(Kangaroo::Util::PluginComponentFactory& factory)
    : m_factory(factory) {}

auto ModuleDispatcher::dispatch(std::string_view request_json) -> std::string {
    nlohmann::json request_id = nullptr;
    nlohmann::json module = nullptr;
    nlohmann::json action = nullptr;

    try {
        const nlohmann::json request = nlohmann::json::parse(request_json);
        if(!request.is_object()) {
            return Base::makeErrorResponse(request_id, module, action,
                                           "Request envelope must be a JSON object.");
        }

        request_id =
            request.contains("requestId") ? request.at("requestId") : nlohmann::json(nullptr);
        module = request.contains("module") ? request.at("module") : nlohmann::json(nullptr);
        action = request.contains("action") ? request.at("action") : nlohmann::json(nullptr);

        if(!module.is_string()) {
            if(module.is_null()) {
                return Base::makeErrorResponse(request_id, module, action,
                                               "Missing 'module' field");
            }
            return Base::makeErrorResponse(request_id, module, action,
                                           "Request module must be a string.");
        }

        if(!action.is_string()) {
            return Base::makeErrorResponse(request_id, module, action,
                                           "Request action must be a string.");
        }

        const auto payload =
            request.contains("payload") ? request.at("payload") : nlohmann::json::object();
        const std::string module_name = module.get<std::string>();
        const std::string action_name = action.get<std::string>();

        m_recorder.record(request_json);

        auto module_service = resolveModule(module_name);
        if(module_service == nullptr) {
            const std::string message = "Unknown module: " + module_name;
            return Base::makeErrorResponse(request_id, module_name, action_name, message);
        }

        const CommandResult command_result = module_service->dispatch(action_name, payload);

        return Base::makeResponse(request_id, module_name, action_name, command_result.ok,
                                  command_result.summary, command_result.result,
                                  nlohmann::json::array());
    } catch(const nlohmann::json::exception& exception) {
        return Base::makeErrorResponse(request_id, module, action, exception.what());
    } catch(const std::exception& exception) {
        return Base::makeErrorResponse(request_id, module, action, exception.what());
    }
}

auto ModuleDispatcher::registeredModules() const -> std::vector<std::string> {
    return m_registeredModules;
}

void ModuleDispatcher::startRecording() { m_recorder.start(); }

void ModuleDispatcher::stopRecording() { m_recorder.stop(); }

auto ModuleDispatcher::isRecording() const noexcept -> bool { return m_recorder.isRecording(); }

auto ModuleDispatcher::getRecordedRequests() const -> const std::vector<std::string>& {
    return m_recorder.get();
}

void ModuleDispatcher::clearRecording() { m_recorder.clear(); }

auto ModuleDispatcher::resolveModule(const std::string& module_name)
    -> std::shared_ptr<IModuleService> {
    auto it = m_moduleCache.find(module_name);
    if(it != m_moduleCache.end()) {
        return it->second;
    }

    try {
        auto module_service = m_factory.getSharedInstance<IModuleService>(module_name);
        if(module_service) {
            m_moduleCache.emplace(module_name, module_service);
            m_registeredModules.emplace_back(module_name);
        }
        return module_service;
    } catch(const Kangaroo::Util::ComponentFactoryNotRegisteredEx&) {
        return nullptr;
    }
}

} // namespace OpenGeoLab::Command
