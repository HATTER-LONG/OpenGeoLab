/**
 * @file highlight_action.cpp
 * @brief Implementation of highlight action
 */

#include "highlight_action.hpp"
#include "render/render_scene_controller.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Geometry {

namespace {

/**
 * @brief Convert string to HighlightState enum
 * @param state_str State string ("none", "preview", "selected")
 * @return Corresponding HighlightState
 */
[[nodiscard]] Render::HighlightState parseHighlightState(const std::string& state_str) {
    if(state_str == "preview") {
        return Render::HighlightState::Preview;
    } else if(state_str == "selected") {
        return Render::HighlightState::Selected;
    }
    return Render::HighlightState::None;
}

/**
 * @brief Convert HighlightState enum to string
 * @param state Highlight state
 * @return String representation
 */
[[nodiscard]] const char* highlightStateToString(Render::HighlightState state) {
    switch(state) {
    case Render::HighlightState::Preview:
        return "preview";
    case Render::HighlightState::Selected:
        return "selected";
    default:
        return "none";
    }
}

} // anonymous namespace

[[nodiscard]] nlohmann::json HighlightAction::execute(const nlohmann::json& params,
                                                      Util::ProgressCallback progress_callback) {
    nlohmann::json response;

    if(!params.contains("operation")) {
        LOG_ERROR("HighlightAction: Missing 'operation' parameter");
        response["success"] = false;
        response["error"] = "Missing 'operation' parameter";
        return response;
    }

    std::string operation = params["operation"].get<std::string>();
    auto& controller = Render::RenderSceneController::instance();

    if(progress_callback && !progress_callback(0.1, "Processing highlight...")) {
        response["success"] = false;
        response["error"] = "Operation cancelled";
        return response;
    }

    // -------------------------------------------------------------------------
    // Operation: set
    // -------------------------------------------------------------------------
    if(operation == "set") {
        Render::HighlightState state = Render::HighlightState::None;
        if(params.contains("state")) {
            state = parseHighlightState(params["state"].get<std::string>());
        }

        // Batch operation
        if(params.contains("entity_ids") && params["entity_ids"].is_array()) {
            std::vector<EntityId> ids;
            for(const auto& id_json : params["entity_ids"]) {
                if(id_json.is_number_unsigned()) {
                    ids.push_back(id_json.get<EntityId>());
                }
            }

            controller.setHighlight(ids, state);

            LOG_DEBUG("HighlightAction: Set {} entities to state '{}'", ids.size(),
                      highlightStateToString(state));

            response["success"] = true;
            response["affected_count"] = ids.size();
            response["state"] = highlightStateToString(state);
            return response;
        }

        // Single entity operation
        if(params.contains("entity_id")) {
            EntityId entity_id = params["entity_id"].get<EntityId>();
            controller.setHighlight(entity_id, state);

            LOG_DEBUG("HighlightAction: Set entity {} to state '{}'", entity_id,
                      highlightStateToString(state));

            response["success"] = true;
            response["entity_id"] = entity_id;
            response["state"] = highlightStateToString(state);
            return response;
        }

        response["success"] = false;
        response["error"] = "Missing 'entity_id' or 'entity_ids' for set operation";
        return response;
    }

    // -------------------------------------------------------------------------
    // Operation: clear
    // -------------------------------------------------------------------------
    if(operation == "clear") {
        if(params.contains("entity_ids") && params["entity_ids"].is_array()) {
            std::vector<EntityId> ids;
            for(const auto& id_json : params["entity_ids"]) {
                if(id_json.is_number_unsigned()) {
                    ids.push_back(id_json.get<EntityId>());
                }
            }

            controller.clearHighlight(ids);

            LOG_DEBUG("HighlightAction: Cleared highlight for {} entities", ids.size());

            response["success"] = true;
            response["cleared_count"] = ids.size();
            return response;
        }

        if(params.contains("entity_id")) {
            EntityId entity_id = params["entity_id"].get<EntityId>();
            controller.clearHighlight({entity_id});

            response["success"] = true;
            response["entity_id"] = entity_id;
            return response;
        }

        response["success"] = false;
        response["error"] = "Missing 'entity_id' or 'entity_ids' for clear operation";
        return response;
    }

    // -------------------------------------------------------------------------
    // Operation: clear_all
    // -------------------------------------------------------------------------
    if(operation == "clear_all") {
        controller.clearAllHighlights();

        LOG_DEBUG("HighlightAction: Cleared all highlights");

        response["success"] = true;
        return response;
    }

    // -------------------------------------------------------------------------
    // Operation: get
    // -------------------------------------------------------------------------
    if(operation == "get") {
        const auto& highlights = controller.allHighlights();

        nlohmann::json preview_ids = nlohmann::json::array();
        nlohmann::json selected_ids = nlohmann::json::array();

        for(const auto& [id, state] : highlights) {
            if(state == Render::HighlightState::Preview) {
                preview_ids.push_back(id);
            } else if(state == Render::HighlightState::Selected) {
                selected_ids.push_back(id);
            }
        }

        response["success"] = true;
        response["preview_ids"] = preview_ids;
        response["selected_ids"] = selected_ids;
        response["total_highlighted"] = highlights.size();
        return response;
    }

    // Unknown operation
    LOG_ERROR("HighlightAction: Unknown operation '{}'", operation);
    response["success"] = false;
    response["error"] = "Unknown operation: " + operation;
    return response;
}

} // namespace OpenGeoLab::Geometry
