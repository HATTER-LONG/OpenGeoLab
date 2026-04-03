/**
 * @file add_label_action.cpp
 * @brief AddLabelAction implementation
 */

#include <opengeolab/scene/add_label_action.hpp>

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/core/label_colors.hpp>
#include <opengeolab/scene/label_manager.hpp>

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

} // namespace

AddLabelAction::AddLabelAction(LabelManager& manager) : m_manager(manager) {}
AddLabelAction::~AddLabelAction() = default;

nlohmann::json AddLabelAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Create or replace a label for one topology entity."},
        {"params",
         {{"shapeId",
           {{"type", "integer"}, {"required", true}, {"description", "Owning shape id."}}},
          {"entityType",
           {{"type", "string"},
            {"required", true},
            {"description", "Entity type: GeoVertex, GeoEdge, GeoWire, GeoFace, or GeoSolid."}}},
          {"localId",
           {{"type", "integer"}, {"required", true}, {"description", "Topology-local id."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"text", {{"type", "string"}, {"description", "Generated label text."}}}}}};
}

nlohmann::json AddLabelAction::execute(const nlohmann::json& param,
                                       const Core::ProgressCallback& progress) {
    const auto entity_type = parseEntityType(param.value("entityType", std::string{}));
    if(!entity_type.has_value()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Unknown 'entityType'"}};
    }

    const auto shape_id = param.value("shapeId", static_cast<uint32_t>(0));
    const auto local_id = param.value("localId", static_cast<uint32_t>(0));
    const Core::EntityRef entity{shape_id, *entity_type, local_id};
    const std::string text = Core::formatLabelText(shape_id, *entity_type, local_id);

    m_manager.addLabel({entity, text, Core::labelColor(*entity_type), Core::K_LABEL_BG_COLOR});

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}, {"text", text}};
}

} // namespace OpenGeoLab::Scene
