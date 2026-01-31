/**
 * @file offset_action.hpp
 * @brief Offset geometry operation action
 *
 * OffsetAction handles creating offset surfaces from faces or shells.
 * This action follows the command pattern like CreateAction.
 */

#pragma once

#include "geometry/geometry_action.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Action for creating offset geometry
 *
 * Accepts JSON parameters:
 * - sourceEntity: EntityId of the geometry to offset
 * - distance: Offset distance (positive = outward, negative = inward)
 * - keepOriginal: (optional) Keep original geometry after offset
 *
 * Example:
 * @code{.json}
 * {
 *   "sourceEntity": 123,
 *   "distance": 2.0,
 *   "keepOriginal": true
 * }
 * @endcode
 */
class OffsetAction : public GeometryActionBase {
public:
    OffsetAction() = default;
    ~OffsetAction() override = default;

    /**
     * @brief Get the action identifier
     * @return Action name string "offset"
     */
    [[nodiscard]] static std::string actionName() { return "offset"; }

    /**
     * @brief Execute the offset operation
     * @param params JSON with sourceEntity, distance, and options
     * @param progress_callback Progress reporting callback
     * @return true if offset was applied successfully
     */
    [[nodiscard]] bool execute(const nlohmann::json& params,
                               Util::ProgressCallback progress_callback) override;
};

/**
 * @brief Factory for creating OffsetAction instances
 */
class OffsetActionFactory : public GeometryActionFactory {
public:
    OffsetActionFactory() = default;
    ~OffsetActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<OffsetAction>(); }
};

} // namespace OpenGeoLab::Geometry
