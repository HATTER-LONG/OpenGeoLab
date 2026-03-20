/**
 * @file command_dispatcher.hpp
 * @brief Declares the dispatcher responsible for routing JSON requests to commands.
 */

#pragma once

#include <opengeolab/command/command_export.hpp>
#include <opengeolab/command/command_interface.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Command {

/**
 * @brief Routes protocol requests to registered command handlers.
 */
class OPENGEOLAB_COMMAND_EXPORT CommandDispatcher {
public:
    CommandDispatcher() = default;
    ~CommandDispatcher() = default;
    CommandDispatcher(const CommandDispatcher&) = delete;
    auto operator=(const CommandDispatcher&) -> CommandDispatcher& = delete;
    CommandDispatcher(CommandDispatcher&&) noexcept = default;
    auto operator=(CommandDispatcher&&) noexcept -> CommandDispatcher& = default;

    /**
     * @brief Registers a command handler using its action name as the lookup key.
     * @param command Owning pointer to the command implementation.
     * @warning Passing a null command is invalid and results in std::invalid_argument.
     */
    void registerCommand(std::unique_ptr<ICommand> command);

    /**
     * @brief Dispatches a JSON request string and returns a JSON response string.
     * @param request_json Request envelope containing action, payload, and optional requestId.
     * @return Serialized JSON response following the command protocol envelope.
     */
    [[nodiscard]] auto dispatch(std::string_view request_json) -> std::string;

    /**
     * @brief Returns the action names of all currently registered commands.
     * @return Views into the internally stored action keys. The views remain valid while the
     *         dispatcher and its command registry remain unchanged.
     */
    [[nodiscard]] auto registeredActions() const -> std::vector<std::string_view>;

private:
    std::unordered_map<std::string, std::unique_ptr<ICommand>> m_commands;
};

} // namespace OpenGeoLab::Command
