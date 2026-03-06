/**
 * @file part_visibility_control.cpp
 * @brief Implementation of per-part visibility control render action.
 */

#include "part_visibility_control.hpp"

#include "render/render_scene_controller.hpp"

namespace OpenGeoLab::Render {
namespace {

[[nodiscard]] nlohmann::json makeErrorResponse(const std::string& error) {
    return nlohmann::json{{"success", false}, {"error", error}};
}

[[nodiscard]] bool appendPartUids(const nlohmann::json& visibility,
                                  std::vector<uint64_t>& part_uids,
                                  std::string& error) {
    if(visibility.contains("part_uid")) {
        if(!visibility["part_uid"].is_number_integer()) {
            error = "'part_visibility.part_uid' must be an integer";
            return false;
        }
        part_uids.push_back(visibility["part_uid"].get<uint64_t>());
    }

    if(visibility.contains("part_uids")) {
        if(!visibility["part_uids"].is_array()) {
            error = "'part_visibility.part_uids' must be an array of integers";
            return false;
        }
        for(const auto& uid : visibility["part_uids"]) {
            if(!uid.is_number_integer()) {
                error = "'part_visibility.part_uids' must contain only integers";
                return false;
            }
            part_uids.push_back(uid.get<uint64_t>());
        }
    }

    if(part_uids.empty()) {
        error = "Missing 'part_visibility.part_uid' or 'part_visibility.part_uids'";
        return false;
    }

    std::sort(part_uids.begin(), part_uids.end());
    part_uids.erase(std::unique(part_uids.begin(), part_uids.end()), part_uids.end());
    return true;
}

} // namespace

nlohmann::json PartVisibilityControl::execute(const nlohmann::json& params,
                                              Util::ProgressCallback /*progress_callback*/) {
    if(!params.is_object() || !params.contains("part_visibility") ||
       !params["part_visibility"].is_object()) {
        return makeErrorResponse("Missing or invalid 'part_visibility' parameter");
    }

    const auto& visibility = params["part_visibility"];
    std::vector<uint64_t> part_uids;
    std::string error;
    if(!appendPartUids(visibility, part_uids, error)) {
        return makeErrorResponse(error);
    }

    bool changed = false;

    if(visibility.contains("geometry_visible")) {
        if(!visibility["geometry_visible"].is_boolean()) {
            return makeErrorResponse("'part_visibility.geometry_visible' must be a boolean");
        }
        const bool geometry_visible = visibility["geometry_visible"].get<bool>();
        for(const auto part_uid : part_uids) {
            RenderSceneController::instance().setPartGeometryVisible(part_uid, geometry_visible);
        }
        changed = true;
    }

    if(visibility.contains("mesh_visible")) {
        if(!visibility["mesh_visible"].is_boolean()) {
            return makeErrorResponse("'part_visibility.mesh_visible' must be a boolean");
        }
        const bool mesh_visible = visibility["mesh_visible"].get<bool>();
        for(const auto part_uid : part_uids) {
            RenderSceneController::instance().setPartMeshVisible(part_uid, mesh_visible);
        }
        changed = true;
    }

    if(!changed) {
        return makeErrorResponse("Missing visibility field to update");
    }

    nlohmann::json response{{"success", true}, {"part_uids", part_uids}};
    if(part_uids.size() == 1) {
        response["part_uid"] = part_uids.front();
    }
    if(visibility.contains("geometry_visible")) {
        response["geometry_visible"] = visibility["geometry_visible"].get<bool>();
    }
    if(visibility.contains("mesh_visible")) {
        response["mesh_visible"] = visibility["mesh_visible"].get<bool>();
    }
    return response;
}

} // namespace OpenGeoLab::Render