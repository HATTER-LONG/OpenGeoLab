/**
 * @file query_mesh_info_action.hpp
 * @brief Mesh action for querying detailed information of selected mesh entities
 */

#pragma once

#include "mesh/mesh_action.hpp"

namespace OpenGeoLab::Mesh {

/**
 * @brief Action for querying detailed mesh node/element information
 *
 * Request parameters:
 * - action: "query_mesh_info"
 * - entities: array of objects {"uid": <number>, "type": <string>}
 *
 * Response:
 * - success: boolean
 * - entities: array of info objects for found entities
 * - not_found: array of handles that could not be resolved
 */
class QueryMeshInfoAction final : public MeshActionBase {
public:
    QueryMeshInfoAction() = default;
    ~QueryMeshInfoAction() override = default;

    [[nodiscard]] static std::string actionName() { return "query_mesh_info"; }

    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;
};

class QueryMeshInfoActionFactory : public MeshActionFactory {
public:
    QueryMeshInfoActionFactory() = default;
    ~QueryMeshInfoActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<QueryMeshInfoAction>(); }
};

} // namespace OpenGeoLab::Mesh
