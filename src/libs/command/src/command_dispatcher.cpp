#include <opengeolab/command/command_dispatcher.hpp>

#include <opengeolab/base/protocol_constants.hpp>
#include <opengeolab/base/response_builder.hpp>

#include <stdexcept>
#include <utility>

namespace OpenGeoLab::Command {

using OpenGeoLab::Base::CommandResult;

void CommandDispatcher::registerCommand(std::unique_ptr<ICommand> command) {
    if(command == nullptr) {
        throw std::invalid_argument("CommandDispatcher::registerCommand requires a valid command");
    }

    std::string action_name{command->actionName()};
    m_commands.insert_or_assign(std::move(action_name), std::move(command));
}

auto CommandDispatcher::dispatch(std::string_view request_json) -> std::string {
    nlohmann::json request_id = nullptr;
    nlohmann::json action = nullptr;

    try {
        const nlohmann::json request = nlohmann::json::parse(request_json);
        request_id =
            request.contains("requestId") ? request.at("requestId") : nlohmann::json(nullptr);
        action = request.contains("action") ? request.at("action") : nlohmann::json(nullptr);

        if(!action.is_string()) {
            return Base::makeErrorResponse(request_id, action, "Request action must be a string.");
        }

        const std::string action_name = action.get<std::string>();
        const auto command_iterator = m_commands.find(action_name);
        if(command_iterator == m_commands.end()) {
            const std::string summary = "Unknown action: " + action_name;
            return Base::makeErrorResponse(request_id, action_name, summary);
        }

        const nlohmann::json payload =
            request.contains("payload") ? request.at("payload") : nlohmann::json::object();
        const CommandResult command_result = command_iterator->second->execute(payload);

        return Base::makeResponse(request_id, action_name, command_result.ok,
                                  command_result.summary, command_result.result,
                                  nlohmann::json::array());
    } catch(const std::exception& exception) {
        return Base::makeErrorResponse(request_id, action, exception.what());
    }
}

auto CommandDispatcher::registeredActions() const -> std::vector<std::string_view> {
    std::vector<std::string_view> actions;
    actions.reserve(m_commands.size());
    for(const auto& [action_name, command] : m_commands) {
        static_cast<void>(command);
        actions.emplace_back(action_name);
    }
    return actions;
}

} // namespace OpenGeoLab::Command
