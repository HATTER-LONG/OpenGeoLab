/**
 * @file query_selection_action.cpp
 * @brief QuerySelectionAction implementation
 */

#include <opengeolab/scene/query_selection_action.hpp>

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/scene/selection_state.hpp>

namespace OpenGeoLab::Scene {

QuerySelectionAction::QuerySelectionAction(const SelectionState& state) : m_state(state) {}
QuerySelectionAction::~QuerySelectionAction() = default;

nlohmann::json QuerySelectionAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Return the currently selected entities."},
        {"params", nlohmann::json::object()},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"selections",
           {{"type", "array"},
            {"description",
             "Array of {shapeId, type, localId} selected entities. "
             "type is one of: GeoVertex, GeoEdge, GeoWire, GeoFace, "
             "GeoSolid, MeshNode, MeshEdge, or MeshElement."}}}}}};
}

nlohmann::json QuerySelectionAction::execute(const nlohmann::json& param,
                                             const Core::ProgressCallback& progress) {
    static_cast<void>(param);
    nlohmann::json selections_json = nlohmann::json::array();

    for(const auto& entity : m_state.selections()) {
        selections_json.push_back({{"shapeId", entity.shapeId},
                                   {"type", Core::entityTypeName(entity.entityType)},
                                   {"localId", entity.localId}});
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}, {"selections", std::move(selections_json)}};
}

} // namespace OpenGeoLab::Scene
