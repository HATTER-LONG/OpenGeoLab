/**
 * @file set_hover_action.cpp
 * @brief SetHoverAction implementation
 */

#include <opengeolab/scene/set_hover_action.hpp>

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/scene/selection_state.hpp>

#include <optional>
#include <string>

namespace OpenGeoLab::Scene {

static Core::EntityRef parseEntityRef(const nlohmann::json& entity_json) {
    const auto entity_type = Core::parseEntityType(entity_json.value("type", std::string{}));
    if(!entity_type.has_value()) {
        return {};
    }
    return {
        entity_json.value("shapeId", static_cast<uint32_t>(0)),
        *entity_type,
        entity_json.value("localId", static_cast<uint32_t>(0)),
    };
}

SetHoverAction::SetHoverAction(SelectionState& state) : m_state(state) {}
SetHoverAction::~SetHoverAction() = default;

nlohmann::json SetHoverAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Set or clear the currently hovered entity."},
        {"params",
         {{"entity",
           {{"type", "object"},
            {"required", false},
            {"description",
             "Optional {shapeId, type, localId} entity reference. Null clears. "
             "type must be one of: GeoVertex, GeoEdge, GeoWire, GeoFace, "
             "GeoSolid, MeshNode, MeshEdge, or MeshElement."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}}}}};
}

nlohmann::json SetHoverAction::execute(const nlohmann::json& param,
                                       const Core::ProgressCallback& progress) {
    if(!param.contains("entity") || param["entity"].is_null()) {
        m_state.clearHover();
    } else {
        const Core::EntityRef entity = parseEntityRef(param["entity"]);
        if(entity.isValid()) {
            m_state.setHovered(entity);
        } else {
            m_state.clearHover();
        }
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
