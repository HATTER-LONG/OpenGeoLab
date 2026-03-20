/**
 * @file set_points_command.hpp
 * @brief Declares the command that writes explicit point data into a shared store.
 */

#pragma once

#include <opengeolab/command/command_export.hpp>
#include <opengeolab/command/command_interface.hpp>
#include <opengeolab/geometry/point_store.hpp>

#include <memory>
#include <string_view>

namespace OpenGeoLab::Command {

/**
 * @brief Stores caller-provided 3-D points and returns the resulting bounding box.
 *
 * Demonstrates the PySide6 → Python → C++ write path: the UI can push data
 * into the main process via this command, and later read it back through
 * GetStoredBBoxCommand.
 */
class OPENGEOLAB_COMMAND_EXPORT SetPointsCommand : public ICommand {
public:
    /// @param store Shared point store that persists across requests.
    explicit SetPointsCommand(std::shared_ptr<Geometry::PointStore> store);

    /**
     * @brief Parses point data from the payload and stores it.
     * @param payload JSON object containing a "points" array of {x, y, z} objects.
     * @return Stored point count and computed bounding box.
     */
    [[nodiscard]] auto execute(const nlohmann::json& payload) -> CommandResult override;

    /// @return The string literal "geometry.set_points".
    [[nodiscard]] auto actionName() const noexcept -> std::string_view override;

private:
    std::shared_ptr<Geometry::PointStore> store_;
};

} // namespace OpenGeoLab::Command
