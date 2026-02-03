/**
 * @file query_entity_action.hpp
 * @brief Action for querying entity information by ID
 *
 * QueryEntityAction provides detailed information about geometry entities
 * including their type, properties, hierarchy relationships, and bounding box.
 * Supports both single entity and batch queries.
 */

#pragma once

#include "geometry/geometry_action.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Action for querying entity details by ID
 *
 * Retrieves comprehensive information about one or more entities:
 * - Entity type and name
 * - Parent/child relationships
 * - Geometric properties (bounding box, etc.)
 * - Owning part information
 */
class QueryEntityAction : public GeometryActionBase {
public:
    QueryEntityAction() = default;
    ~QueryEntityAction() override = default;

    /**
     * @brief Get the action identifier
     * @return Action name string "query_entity"
     */
    [[nodiscard]] static std::string actionName() { return "query_entity"; }

    /**
     * @brief Execute the query entity action
     * @param params JSON parameters:
     *        - entity_id: single EntityId to query
     *        - entity_ids: array of EntityIds for batch query
     * @param progress_callback Progress reporting callback
     * @return JSON with "success" and entity information
     *
     * Response format for single query:
     * {
     *   "success": true,
     *   "entity": {
     *     "id": 123,
     *     "uid": 45,
     *     "type": "Face",
     *     "type_enum": 4,
     *     "name": "Face_45",
     *     "parent_ids": [100],
     *     "child_ids": [200, 201],
     *     "owning_part_id": 1,
     *     "owning_part_name": "Box-1",
     *     "bounding_box": { "min": [...], "max": [...] }
     *   }
     * }
     *
     * Response format for batch query:
     * {
     *   "success": true,
     *   "entities": [ {...}, {...} ]
     * }
     */
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;
};

/**
 * @brief Factory for creating QueryEntityAction instances
 */
class QueryEntityActionFactory : public GeometryActionFactory {
public:
    QueryEntityActionFactory() = default;
    ~QueryEntityActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<QueryEntityAction>(); }
};

} // namespace OpenGeoLab::Geometry
