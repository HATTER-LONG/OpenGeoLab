/**
 * @file command_interface.hpp
 * @brief Declares the command execution contract for JSON-driven operations.
 */

#pragma once

#include <opengeolab/base/command_result.hpp>

#include <string_view>

namespace OpenGeoLab::Command {

using OpenGeoLab::Base::CommandResult;

/**
 * @brief Defines the interface implemented by JSON-dispatchable commands.
 */
class ICommand {
public:
    virtual ~ICommand() = default;

    /**
     * @brief Executes the command with the provided payload.
     * @param payload JSON payload extracted from the request envelope.
     * @return Structured execution result containing status, summary, and payload data.
     */
    [[nodiscard]] virtual auto execute(const nlohmann::json& payload) -> CommandResult = 0;

    /**
     * @brief Returns the protocol action name handled by this command.
     * @return Stable action identifier used during dispatch.
     */
    [[nodiscard]] virtual auto actionName() const noexcept -> std::string_view = 0;
};

} // namespace OpenGeoLab::Command
