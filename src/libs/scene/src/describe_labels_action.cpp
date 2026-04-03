/**
 * @file describe_labels_action.cpp
 * @brief DescribeLabelsAction implementation
 */

#include <opengeolab/scene/describe_labels_action.hpp>

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/core/label_colors.hpp>
#include <opengeolab/scene/label_manager.hpp>

namespace OpenGeoLab::Scene {

namespace {

std::string_view entityTypeName(Core::EntityType entity_type) {
    switch(entity_type) {
    case Core::EntityType::GeoVertex:
        return "GeoVertex";
    case Core::EntityType::GeoEdge:
        return "GeoEdge";
    case Core::EntityType::GeoWire:
        return "GeoWire";
    case Core::EntityType::GeoFace:
        return "GeoFace";
    case Core::EntityType::GeoSolid:
        return "GeoSolid";
    case Core::EntityType::MeshNode:
        return "MeshNode";
    case Core::EntityType::MeshEdge:
        return "MeshEdge";
    case Core::EntityType::MeshElement:
        return "MeshElement";
    case Core::EntityType::SceneNode:
        return "SceneNode";
    }
    return "Unknown";
}

} // namespace

DescribeLabelsAction::DescribeLabelsAction(const LabelManager& label_manager)
    : m_labelManager(label_manager) {}

DescribeLabelsAction::~DescribeLabelsAction() = default;

nlohmann::json DescribeLabelsAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description",
         "Return active viewport labels and their visual encoding scheme. "
         "Designed for LLM consumption alongside viewport screenshots."},
        {"params", nlohmann::json::object()},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"colorLegend",
           {{"type", "object"},
            {"description", "Entity type → {prefix, color, description} mapping."}}},
          {"labels",
           {{"type", "array"},
            {"description",
             "Array of {text, shapeId, entityType, localId, color} for each active label."}}},
          {"totalLabels",
           {{"type", "integer"}, {"description", "Number of active labels."}}}}}};
}

nlohmann::json DescribeLabelsAction::execute(const nlohmann::json& /*param*/,
                                              const Core::ProgressCallback& progress) {
    auto labels = m_labelManager.labels();

    nlohmann::json labels_json = nlohmann::json::array();
    for(const auto& lbl : labels) {
        labels_json.push_back(
            {{"text", lbl.text},
             {"shapeId", lbl.entity.shapeId},
             {"entityType", std::string(entityTypeName(lbl.entity.entityType))},
             {"localId", lbl.entity.localId},
             {"color", std::string(Core::labelColorHex(lbl.entity.entityType))}});
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true},
            {"action", ACTION_NAME},
            {"colorLegend", buildColorLegend()},
            {"textFormat", "<prefix>:<localId>  (e.g. F:3 = Face #3)"},
            {"occlusionBehavior",
             "Labels behind geometry appear semi-transparent (30% opacity)"},
            {"labels", std::move(labels_json)},
            {"totalLabels", labels.size()}};
}

nlohmann::json DescribeLabelsAction::buildColorLegend() {
    return {{"GeoVertex",
             {{"prefix", "V"},
              {"color", Core::labelColorHex(Core::EntityType::GeoVertex)},
              {"description", "Red label — topological vertex"}}},
            {"GeoEdge",
             {{"prefix", "E"},
              {"color", Core::labelColorHex(Core::EntityType::GeoEdge)},
              {"description", "Blue label — topological edge"}}},
            {"GeoFace",
             {{"prefix", "F"},
              {"color", Core::labelColorHex(Core::EntityType::GeoFace)},
              {"description", "Green label — topological face"}}},
            {"GeoSolid",
             {{"prefix", "S"},
              {"color", Core::labelColorHex(Core::EntityType::GeoSolid)},
              {"description", "Orange label — topological solid"}}}};
}

} // namespace OpenGeoLab::Scene
