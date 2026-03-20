/**
 * @file action_interface.hpp
 * @brief Declares the plugin-facing action contract for command modules.
 */

#pragma once

#include <opengeolab/command/command_result.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>
#include <nlohmann/json_fwd.hpp>

#include <string_view>

namespace OpenGeoLab::Command {

/**
 * @brief Defines the interface implemented by command actions exposed from a module.
 */
class IAction {
public:
    virtual ~IAction() = default;

    /**
     * @brief Returns the stable action identifier handled by this component.
     * @return Action name used by module dispatch and plugin discovery.
     */
    [[nodiscard]] virtual auto actionName() const noexcept -> std::string_view = 0;

    /**
     * @brief Executes the action with the provided JSON payload.
     * @param payload JSON payload extracted from the request envelope.
     * @return Structured execution result describing status, summary, and payload data.
     */
    [[nodiscard]] virtual auto execute(const nlohmann::json& payload) -> CommandResult = 0;
};

} // namespace OpenGeoLab::Command

template <> struct Kangaroo::Util::PluginComponentInterfaceId<OpenGeoLab::Command::IAction> {
    static constexpr const char* VALUE = "OpenGeoLab.IAction";
};
