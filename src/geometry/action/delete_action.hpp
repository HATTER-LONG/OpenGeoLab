/**
 * @file delete_action.hpp
 * @brief Delete entities action
 *
 * DeleteAction handles removing entities from the geometry document.
 * This action follows the command pattern like CreateAction.
 */

#pragma once

#include "geometry/geometry_action.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Action for deleting geometry entities
 *
 * Accepts JSON parameters:
 * - entities: Array of EntityIds to delete
 * - deleteChildren: (optional) Whether to delete child entities (default: true)
 *
 * Example:
 * @code{.json}
 * {
 *   "entities": [123, 456, 789],
 *   "deleteChildren": true
 * }
 * @endcode
 */
class DeleteAction : public GeometryActionBase {
public:
    DeleteAction() = default;
    ~DeleteAction() override = default;

    /**
     * @brief Get the action identifier
     * @return Action name string "delete"
     */
    [[nodiscard]] static std::string actionName() { return "delete"; }

    /**
     * @brief Execute the delete operation
     * @param params JSON with entities array and options
     * @param progress_callback Progress reporting callback
     * @return true if entities were deleted successfully
     */
    [[nodiscard]] bool execute(const nlohmann::json& params,
                               Util::ProgressCallback progress_callback) override;
};

/**
 * @brief Factory for creating DeleteAction instances
 */
class DeleteActionFactory : public GeometryActionFactory {
public:
    DeleteActionFactory() = default;
    ~DeleteActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<DeleteAction>(); }
};

} // namespace OpenGeoLab::Geometry
