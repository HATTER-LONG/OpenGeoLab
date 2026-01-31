/**
 * @file trim_action.hpp
 * @brief Trim geometry operation action
 *
 * TrimAction handles trimming geometry by another shape.
 * This action follows the command pattern like CreateAction.
 */

#pragma once

#include "geometry/geometry_action.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Action for trimming geometry
 *
 * Accepts JSON parameters:
 * - targetEntity: EntityId of the geometry to trim
 * - toolEntity: EntityId of the trimming tool
 * - keepInside: (optional) Keep inside portion (true) or outside (false)
 * - keepOriginal: (optional) Keep original geometry after trim
 *
 * Example:
 * @code{.json}
 * {
 *   "targetEntity": 123,
 *   "toolEntity": 456,
 *   "keepInside": false,
 *   "keepOriginal": false
 * }
 * @endcode
 */
class TrimAction : public GeometryActionBase {
public:
    TrimAction() = default;
    ~TrimAction() override = default;

    /**
     * @brief Get the action identifier
     * @return Action name string "trim"
     */
    [[nodiscard]] static std::string actionName() { return "trim"; }

    /**
     * @brief Execute the trim operation
     * @param params JSON with targetEntity, toolEntity, and options
     * @param progress_callback Progress reporting callback
     * @return true if trim was applied successfully
     */
    [[nodiscard]] bool execute(const nlohmann::json& params,
                               Util::ProgressCallback progress_callback) override;
};

/**
 * @brief Factory for creating TrimAction instances
 */
class TrimActionFactory : public GeometryActionFactory {
public:
    TrimActionFactory() = default;
    ~TrimActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<TrimAction>(); }
};

} // namespace OpenGeoLab::Geometry
