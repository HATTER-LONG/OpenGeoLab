/**
 * @file remove_label_action.cpp
 * @brief RemoveLabelAction implementation
 */

#include <opengeolab/scene/remove_label_action.hpp>

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/scene/label_manager.hpp>

#include <optional>
#include <string>

namespace OpenGeoLab::Scene {

RemoveLabelAction::RemoveLabelAction(LabelManager& manager) : m_manager(manager) {}
RemoveLabelAction::~RemoveLabelAction() = default;

nlohmann::json RemoveLabelAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Remove a label by entity reference."},
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
          {"removed",
           {{"type", "boolean"}, {"description", "Whether an existing label was removed."}}}}}};
}

nlohmann::json RemoveLabelAction::execute(const nlohmann::json& param,
                                          const Core::ProgressCallback& progress) {
    const auto entity_type = Core::parseEntityType(param.value("entityType", std::string{}));
    if(!entity_type.has_value()) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "Unknown 'entityType'"},
                {"removed", false}};
    }

    const Core::EntityRef entity{param.value("shapeId", static_cast<uint32_t>(0)), *entity_type,
                                 param.value("localId", static_cast<uint32_t>(0))};
    const auto version_before = m_manager.version();
    m_manager.removeByEntity(entity);
    const bool removed = m_manager.version() != version_before;

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}, {"removed", removed}};
}

} // namespace OpenGeoLab::Scene
