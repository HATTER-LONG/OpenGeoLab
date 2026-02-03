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
 * - box: Requires nested parameters:
 *        - dimensions: {x, y, z}
 *        - origin: {x, y, z}
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
     * @return JSON with "success", created entity info, or "error" on failure
     */
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
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