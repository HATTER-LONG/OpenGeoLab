#include <opengeolab/command/command_dispatcher.hpp>

#include <stdexcept>
#include <utility>

namespace OpenGeoLab::Command {

static constexpr std::string_view PROTOCOL_VERSION = "1.0";

// NOLINTNEXTLINE(misc-use-anonymous-namespace,readability-function-size)
static auto makeResponse(const nlohmann::json& request_id,
                         const nlohmann::json& action,
                         bool ok,
                         std::string_view summary,
                         const nlohmann::json& result,
                         const nlohmann::json& errors) -> std::string {
    return nlohmann::json{
        {"protocolVersion", PROTOCOL_VERSION},
        {"requestId", request_id},
        {"ok", ok},
        {"action", action},
        {"summary", summary},
        {"result", result},
        {"errors", errors},
    }
        .dump();
}

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
            return makeResponse(request_id, action, false, "Request action must be a string.",
                                nlohmann::json::object(),
                                nlohmann::json::array({"Request action must be a string."}));
        }

        const std::string action_name = action.get<std::string>();
        const auto command_iterator = m_commands.find(action_name);
        if(command_iterator == m_commands.end()) {
            const std::string summary = "Unknown action: " + action_name;
            return makeResponse(request_id, action_name, false, summary, nlohmann::json::object(),
                                nlohmann::json::array({summary}));
        }

        const nlohmann::json payload =
            request.contains("payload") ? request.at("payload") : nlohmann::json::object();
        const CommandResult command_result = command_iterator->second->execute(payload);

        return makeResponse(request_id, action_name, command_result.ok, command_result.summary,
                            command_result.result, nlohmann::json::array());
    } catch(const std::exception& exception) {
        return makeResponse(request_id, action, false, exception.what(), nlohmann::json::object(),
                            nlohmann::json::array({exception.what()}));
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
