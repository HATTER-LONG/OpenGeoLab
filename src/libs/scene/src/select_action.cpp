/**
 * @file select_action.cpp
 * @brief SelectAction implementation
 */

#include <opengeolab/scene/select_action.hpp>

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/scene/selection_state.hpp>

#include <optional>
#include <string>

namespace OpenGeoLab::Scene {

namespace {

std::optional<Core::EntityType> parseEntityType(const std::string& type_name) {
    if(type_name == "GeoVertex") {
        return Core::EntityType::GeoVertex;
    }
    if(type_name == "GeoEdge") {
        return Core::EntityType::GeoEdge;
    }
    if(type_name == "GeoWire") {
        return Core::EntityType::GeoWire;
    }
    if(type_name == "GeoFace") {
        return Core::EntityType::GeoFace;
    }
    if(type_name == "GeoSolid") {
        return Core::EntityType::GeoSolid;
    }
    return std::nullopt;
}

Core::EntityRef parseEntityRef(const nlohmann::json& entity_json) {
    const auto entity_type = parseEntityType(entity_json.value("type", std::string{}));
    if(!entity_type.has_value()) {
        return {};
    }
    return {
        entity_json.value("shapeId", static_cast<uint32_t>(0)),
        *entity_type,
        entity_json.value("localId", static_cast<uint32_t>(0)),
    };
}

} // namespace

SelectAction::SelectAction(SelectionState& state) : m_state(state) {}
SelectAction::~SelectAction() = default;

nlohmann::json SelectAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Add entities to the current selection set."},
        {"params",
         {{"entities",
           {{"type", "array"},
            {"required", true},
            {"description", "Array of {shapeId, type, localId} entity references."}}},
          {"append",
           {{"type", "boolean"},
            {"required", false},
            {"description", "If false, clears the current selection before adding."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"selected",
           {{"type", "integer"},
            {"description", "Number of entities newly selected by this action."}}}}}};
}

nlohmann::json SelectAction::execute(const nlohmann::json& param,
                                     const Core::ProgressCallback& progress) {
    const bool append = param.value("append", true);
    const auto& entities = param.value("entities", nlohmann::json::array());
    uint32_t selected = 0;

    if(!append) {
        m_state.clearSelection();
    }

    for(const auto& entity_json : entities) {
        const Core::EntityRef entity = parseEntityRef(entity_json);
        if(!entity.isValid()) {
            continue;
        }
        if(!m_state.isSelected(entity)) {
            ++selected;
        }
        m_state.addSelection(entity);
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}, {"selected", selected}};
}

} // namespace OpenGeoLab::Scene
