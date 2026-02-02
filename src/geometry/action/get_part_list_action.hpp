/**
 * @file get_part_list_action.hpp
 * @brief Action for retrieving part list information
 *
 * GetPartListAction provides a way to query all parts in the current document
 * along with their entity counts, IDs, and assigned colors.
 */

#pragma once

#include "geometry/geometry_action.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Action for querying part list information
 *
 * Returns a JSON array of all parts in the current document with:
 * - Part entity ID
 * - Part name
 * - Entity counts by type (faces, edges, vertices, etc.)
 * - Assigned color (hex format)
 */
class GetPartListAction : public GeometryActionBase {
public:
    GetPartListAction() = default;
    ~GetPartListAction() override = default;

    /**
     * @brief Get the action identifier
     * @return Action name string "get_part_list"
     */
    [[nodiscard]] static std::string actionName() { return "get_part_list"; }

    /**
     * @brief Execute the get part list action
     * @param params JSON parameters (optional filters)
     * @param progress_callback Progress reporting callback
     * @return JSON with "success" and "parts" array containing part information
     */
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;
};

/**
 * @brief Factory for creating GetPartListAction instances
 */
class GetPartListActionFactory : public GeometryActionFactory {
public:
    GetPartListActionFactory() = default;
    ~GetPartListActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<GetPartListAction>(); }
};

} // namespace OpenGeoLab::Geometry
