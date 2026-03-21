/**
 * @file bounding_box_action.hpp
 * @brief Declares the geometry action that computes a bounding box from generated points.
 */

#pragma once

#include <opengeolab/base/action_interface.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

#include <string_view>

namespace OpenGeoLab::Geometry {

/**
 * @brief Computes a bounding box from generated pseudo-random points.
 */
class OPENGEOLAB_GEOMETRY_EXPORT BoundingBoxAction : public Base::IAction {
public:
    /**
     * @brief Executes the bounding box workflow for the provided payload.
     * @param payload Optional JSON payload with pointCount and seed values.
     * @return Bounding box result and execution metadata.
     */
    [[nodiscard]] auto execute(const nlohmann::json& payload) -> Base::CommandResult override;

    /**
     * @brief Returns the protocol action handled by this action.
     * @return The string literal "bounding_box".
     */
    [[nodiscard]] auto actionName() const noexcept -> std::string_view override;
};

} // namespace OpenGeoLab::Geometry
