/**
 * @file bounding_box_command.hpp
 * @brief Declares the command that computes a random point cloud bounding box.
 */

#pragma once

#include <opengeolab/command/command_export.hpp>
#include <opengeolab/command/command_interface.hpp>

#include <string_view>

namespace OpenGeoLab::Command {

/**
 * @brief Computes a bounding box from generated pseudo-random points.
 */
class OPENGEOLAB_COMMAND_EXPORT BoundingBoxCommand : public ICommand {
public:
    /**
     * @brief Executes the bounding box workflow for the provided payload.
     * @param payload Optional JSON payload with pointCount and seed values.
     * @return Bounding box result and execution metadata.
     */
    [[nodiscard]] auto execute(const nlohmann::json& payload) -> CommandResult override;

    /**
     * @brief Returns the protocol action handled by this command.
     * @return The string literal "geometry.bounding_box".
     */
    [[nodiscard]] auto actionName() const noexcept -> std::string_view override;
};

} // namespace OpenGeoLab::Command
