/**
 * @file chamfer_action.hpp
 * @brief Chamfer geometry operation action
 *
 * ChamferAction handles creating chamfered (beveled) edges on solids.
 * This action follows the command pattern like CreateAction.
 */

#pragma once

#include "geometry/geometry_action.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Action for creating chamfer (beveled edge) on solids
 *
 * Accepts JSON parameters:
 * - targetEntity: EntityId of the solid to modify
 * - edges: Array of EntityIds for edges to chamfer
 * - distance: Chamfer distance (must be positive)
 *
 * Example:
 * @code{.json}
 * {
 *   "targetEntity": 123,
 *   "edges": [456, 789],
 *   "distance": 1.5
 * }
 * @endcode
 */
class ChamferAction : public GeometryActionBase {
public:
    ChamferAction() = default;
    ~ChamferAction() override = default;

    /**
     * @brief Get the action identifier
     * @return Action name string "chamfer"
     */
    [[nodiscard]] static std::string actionName() { return "chamfer"; }

    /**
     * @brief Execute the chamfer operation
     * @param params JSON with targetEntity, edges array, and distance
     * @param progress_callback Progress reporting callback
     * @return true if chamfer was applied successfully
     */
    [[nodiscard]] bool execute(const nlohmann::json& params,
                               Util::ProgressCallback progress_callback) override;
};

/**
 * @brief Factory for creating ChamferAction instances
 */
class ChamferActionFactory : public GeometryActionFactory {
public:
    ChamferActionFactory() = default;
    ~ChamferActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<ChamferAction>(); }
};

} // namespace OpenGeoLab::Geometry
