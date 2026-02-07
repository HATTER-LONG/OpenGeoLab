/*
 * @file query_entity_info_action.hpp
 * @brief Geometry action for querying detailed information of selected entities
 *
 * QueryEntityInfoAction accepts a list of (uid,type) entity handles and returns
 * detailed information for each entity found in the current geometry document.
 */

#pragma once

#include "geometry/geometry_action.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Action for querying detailed entity information
 *
 * Request parameters:
 * - action: "query_entity_info"
 * - entities: array of objects {"uid": <number>, "type": <string>}
 *
 * Response:
 * - success: boolean
 * - entities: array of info objects for found entities
 * - not_found: array of handles that could not be resolved
 */
class QueryEntityInfoAction : public GeometryActionBase {
public:
    QueryEntityInfoAction() = default;
    ~QueryEntityInfoAction() override = default;

    /**
     * @brief Get action identifier
     * @return Action name string "query_entity_info"
     */
    [[nodiscard]] static std::string actionName() { return "query_entity_info"; }

    /**
     * @brief Execute the query action
     * @param params JSON parameters
     * @param progress_callback Optional progress callback
     * @return JSON response with entity information
     */
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;
};

/**
 * @brief Factory for creating QueryEntityInfoAction instances
 */
class QueryEntityInfoActionFactory : public GeometryActionFactory {
public:
    QueryEntityInfoActionFactory() = default;
    ~QueryEntityInfoActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<QueryEntityInfoAction>(); }
};

} // namespace OpenGeoLab::Geometry
