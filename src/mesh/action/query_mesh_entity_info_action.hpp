/**
 * @file query_mesh_entity_info_action.hpp
 * @brief Mesh action for querying detailed information of selected mesh entities
 *
 * QueryMeshEntityInfoAction accepts a list of (uid, type) mesh entity handles
 * and returns detailed information for each entity found in the current mesh document.
 */

#pragma once

#include "mesh/mesh_action.hpp"

namespace OpenGeoLab::Mesh {

/**
 * @brief Action for querying detailed mesh entity information
 *
 * Request parameters:
 * - action: "query_mesh_entity_info"
 * - entities: array of objects {"uid": <number>, "type": <string>}
 *   where type is "MeshNode" or "MeshElement"
 *
 * Response:
 * - success: boolean
 * - entities: array of info objects for found entities
 * - not_found: array of handles that could not be resolved
 */
class QueryMeshEntityInfoAction : public MeshActionBase {
public:
    QueryMeshEntityInfoAction() = default;
    ~QueryMeshEntityInfoAction() override = default;

    /**
     * @brief Get action identifier
     * @return Action name string "query_mesh_entity_info"
     */
    [[nodiscard]] static std::string actionName() { return "query_mesh_entity_info"; }

    /**
     * @brief Execute the query action
     * @param params JSON parameters
     * @param progress_callback Optional progress callback
     * @return JSON response with mesh entity information
     */
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;
};

/**
 * @brief Factory for creating QueryMeshEntityInfoAction instances
 */
class QueryMeshEntityInfoActionFactory : public MeshActionFactory {
public:
    QueryMeshEntityInfoActionFactory() = default;
    ~QueryMeshEntityInfoActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<QueryMeshEntityInfoAction>(); }
};

} // namespace OpenGeoLab::Mesh
