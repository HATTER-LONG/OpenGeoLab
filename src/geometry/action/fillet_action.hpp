/**
 * @file fillet_action.hpp
 * @brief Fillet geometry operation action
 *
 * FilletAction handles creating rounded fillets on solid edges.
 * This action follows the command pattern like CreateAction.
 */

#pragma once

#include "geometry/geometry_action.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Action for creating fillet (rounded edge) on solids
 *
 * Accepts JSON parameters:
 * - targetEntity: EntityId of the solid to modify
 * - edges: Array of EntityIds for edges to fillet
 * - radius: Fillet radius (must be positive)
 *
 * Example:
 * @code{.json}
 * {
 *   "targetEntity": 123,
 *   "edges": [456, 789],
 *   "radius": 2.0
 * }
 * @endcode
 */
class FilletAction : public GeometryActionBase {
public:
    FilletAction() = default;
    ~FilletAction() override = default;

    /**
     * @brief Get the action identifier
     * @return Action name string "fillet"
     */
    [[nodiscard]] static std::string actionName() { return "fillet"; }

    /**
     * @brief Execute the fillet operation
     * @param params JSON with targetEntity, edges array, and radius
     * @param progress_callback Progress reporting callback
     * @return true if fillet was applied successfully
     */
    [[nodiscard]] bool execute(const nlohmann::json& params,
                               Util::ProgressCallback progress_callback) override;
};

/**
 * @brief Factory for creating FilletAction instances
 */
class FilletActionFactory : public GeometryActionFactory {
public:
    FilletActionFactory() = default;
    ~FilletActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<FilletAction>(); }
};

} // namespace OpenGeoLab::Geometry
