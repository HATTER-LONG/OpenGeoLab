/**
 * @file deselect_action.cpp
 * @brief DeselectAction implementation
 */

#include <opengeolab/scene/deselect_action.hpp>

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

DeselectAction::DeselectAction(SelectionState& state) : m_state(state) {}
DeselectAction::~DeselectAction() = default;

nlohmann::json DeselectAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Remove entities from the current selection set."},
        {"params",
         {{"entities",
           {{"type", "array"},
            {"required", true},
            {"description",
             "Array of {shapeId, type, localId} entity references. "
             "type must exactly match the entity type string: GeoVertex, GeoEdge, "
             "GeoWire, GeoFace, GeoSolid, MeshNode, MeshEdge, or MeshElement. "
             "Use query_selection to get the exact type of each selected entity."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"removed",
           {{"type", "integer"},
            {"description", "Number of entities removed from the selection."}}}}}};
}

nlohmann::json DeselectAction::execute(const nlohmann::json& param,
                                       const Core::ProgressCallback& progress) {
    const auto& entities = param.value("entities", nlohmann::json::array());
    uint32_t removed = 0;

    for(const auto& entity_json : entities) {
        const Core::EntityRef entity = parseEntityRef(entity_json);
        if(!entity.isValid()) {
            continue;
        }
        if(m_state.isSelected(entity)) {
            ++removed;
        }
        m_state.removeSelection(entity);
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}, {"removed", removed}};
}

} // namespace OpenGeoLab::Scene
