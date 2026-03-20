/**
 * @file get_stored_bbox_command.hpp
 * @brief Declares the command that reads back the bounding box of stored points.
 */

#pragma once

#include <opengeolab/command/command_export.hpp>
#include <opengeolab/command/command_interface.hpp>
#include <opengeolab/geometry/point_store.hpp>

#include <memory>
#include <string_view>

namespace OpenGeoLab::Command {

/**
 * @brief Computes the bounding box of previously stored point data.
 *
 * Complements SetPointsCommand: after the UI writes points into the store,
 * this command lets the UI verify that the data persists in the C++ process.
 */
class OPENGEOLAB_COMMAND_EXPORT GetStoredBBoxCommand : public ICommand {
public:
    /// @param store Same shared store used by SetPointsCommand.
    explicit GetStoredBBoxCommand(std::shared_ptr<Geometry::PointStore> store);

    /**
     * @brief Reads stored points and computes their bounding box.
     * @param payload Ignored — no input parameters required.
     * @return Bounding box and point count, or error if the store is empty.
     */
    [[nodiscard]] auto execute(const nlohmann::json& payload) -> CommandResult override;

    /// @return The string literal "geometry.get_stored_bbox".
    [[nodiscard]] auto actionName() const noexcept -> std::string_view override;

private:
    std::shared_ptr<Geometry::PointStore> store_;
};

} // namespace OpenGeoLab::Command
