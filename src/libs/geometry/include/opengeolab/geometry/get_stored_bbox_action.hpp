/**
 * @file get_stored_bbox_action.hpp
 * @brief Declares the geometry action that computes a bounding box from stored points.
 */

#pragma once

#include <opengeolab/command/action_interface.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

#include <memory>
#include <string_view>

namespace OpenGeoLab::Geometry {

class PointStore;

/**
 * @brief Computes the bounding box of previously stored point data.
 */
class OPENGEOLAB_GEOMETRY_EXPORT GetStoredBBoxAction : public Command::IAction {
public:
    /// @param store Same shared store used by SetPointsAction.
    explicit GetStoredBBoxAction(std::shared_ptr<PointStore> store);

    /**
     * @brief Reads stored points and computes their bounding box.
     * @param payload Ignored — no input parameters required.
     * @return Bounding box and point count, or error if the store is empty.
     */
    [[nodiscard]] auto execute(const nlohmann::json& payload) -> Command::CommandResult override;

    /// @return The string literal "get_stored_bbox".
    [[nodiscard]] auto actionName() const noexcept -> std::string_view override;

private:
    std::shared_ptr<PointStore> store_;
};

} // namespace OpenGeoLab::Geometry
