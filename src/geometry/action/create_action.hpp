/**
 * @file create_action.hpp
 * @brief Geometry primitive creation action
 *
 * CreateAction handles the creation of basic geometric primitives such as
 * boxes, cylinders, spheres, and cones. Each primitive is added to the
 * current document as a new Part entity.
 */

#pragma once

#include "geometry/geometry_action.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Action for creating basic geometric primitives
 *
 * Supports creating the following shapes:
 * - box: Requires dx, dy, dz (dimensions), optional x, y, z (origin)
 * - cylinder: Requires radius, height, optional x, y, z (base center)
 * - sphere: Requires radius, optional x, y, z (center)
 * - cone: Requires radius1 (base), radius2 (top), height, optional x, y, z (base center)
 *
 * All created shapes are added to the current GeometryDocument as Part entities.
 */
class CreateAction : public GeometryActionBase {
public:
    CreateAction() = default;
    ~CreateAction() override = default;

    /**
     * @brief Get the action identifier
     * @return Action name string "create"
     */
    [[nodiscard]] static std::string actionName() { return "create"; }

    /**
     * @brief Execute the shape creation action
     * @param params JSON with "type" and shape-specific parameters
     * @param progress_callback Progress reporting callback
     * @return true if shape was created successfully
     */
    [[nodiscard]] bool execute(const nlohmann::json& params,
                               Util::ProgressCallback progress_callback) override;
};

/**
 * @brief Factory for creating CreateAction instances
 */
class CreateActionFactory : public GeometryActionFactory {
public:
    CreateActionFactory() = default;
    ~CreateActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<CreateAction>(); }
};

} // namespace OpenGeoLab::Geometry