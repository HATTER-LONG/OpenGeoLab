/**
 * @file module_service_interface.hpp
 * @brief Declares the module-level dispatch contract for grouped command actions.
 */

#pragma once

#include <opengeolab/command/command_result.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>
#include <nlohmann/json_fwd.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace OpenGeoLab::Command {

/**
 * @brief Defines the interface implemented by command modules that route named actions.
 */
class IModuleService {
public:
    virtual ~IModuleService() = default;

    /**
     * @brief Returns the stable module identifier used for registration and lookup.
     * @return Module name exposed to the command runtime.
     */
    [[nodiscard]] virtual auto moduleName() const noexcept -> std::string_view = 0;

    /**
     * @brief Dispatches an action request to the module.
     * @param action Stable action identifier scoped to the module.
     * @param payload JSON payload extracted from the request envelope.
     * @return Structured execution result describing status, summary, and payload data.
     */
    [[nodiscard]] virtual auto dispatch(std::string_view action, const nlohmann::json& payload)
        -> CommandResult = 0;

    /**
     * @brief Lists the actions supported by this module.
     * @return Stable action identifiers available for dispatch.
     */
    [[nodiscard]] virtual auto supportedActions() const -> std::vector<std::string> = 0;
};

} // namespace OpenGeoLab::Command

template <> struct Kangaroo::Util::PluginComponentInterfaceId<OpenGeoLab::Command::IModuleService> {
    static constexpr const char* VALUE = "OpenGeoLab.IModuleService";
};
