/**
 * @file set_points_action.hpp
 * @brief Declares the geometry action that stores explicit point data in a shared store.
 */

#pragma once

#include <opengeolab/base/action_interface.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

#include <memory>
#include <string_view>

namespace OpenGeoLab::Geometry {

class PointStore;

/**
 * @brief Stores caller-provided 3-D points and returns the resulting bounding box.
 */
class OPENGEOLAB_GEOMETRY_EXPORT SetPointsAction : public Base::IAction {
public:
    /// @param store Shared point store that persists across requests.
    explicit SetPointsAction(std::shared_ptr<PointStore> store);

    /**
     * @brief Parses point data from the payload and stores it.
     * @param payload JSON object containing a "points" array of {x, y, z} objects.
     * @return Stored point count and computed bounding box.
     */
    [[nodiscard]] auto execute(const nlohmann::json& payload) -> Base::CommandResult override;

    /// @return The string literal "set_points".
    [[nodiscard]] auto actionName() const noexcept -> std::string_view override;

private:
    std::shared_ptr<PointStore> store_;
};

} // namespace OpenGeoLab::Geometry
