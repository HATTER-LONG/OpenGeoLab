/**
 * @file look_at_entity_action.cpp
 * @brief LookAtEntityAction — point camera at a topology entity
 */

#include <opengeolab/scene/look_at_entity_action.hpp>

#include <opengeolab/scene/scene_graph.hpp>

#include "entity_camera_utils.hpp"

namespace OpenGeoLab::Scene {

LookAtEntityAction::LookAtEntityAction(SceneGraph& graph) : m_graph(graph) {}
LookAtEntityAction::~LookAtEntityAction() = default;

nlohmann::json LookAtEntityAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Point the camera at a specific face/edge/vertex, keeping "
                        "the current viewing distance."},
        {"params",
         {{"shapeId",
           {{"type", "integer"}, {"required", true}, {"description", "Shape identifier."}}},
          {"entityType",
           {{"type", "string"},
            {"required", true},
            {"description", "'face', 'edge', or 'vertex'."}}},
          {"localId",
           {{"type", "integer"}, {"required", true}, {"description", "1-based local index."}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}}},
          {"action", {{"type", "string"}}},
          {"camera",
           {{"type", "object"},
            {"description", "Resulting camera state: {position, target, up}."}}}}}};
}

nlohmann::json LookAtEntityAction::execute(const nlohmann::json& param,
                                           const Core::ProgressCallback& progress) {
    if(!param.contains("shapeId") || !param["shapeId"].is_number()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'shapeId'."}};
    }
    if(!param.contains("entityType") || !param["entityType"].is_string()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'entityType'."}};
    }
    if(!param.contains("localId") || !param["localId"].is_number()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'localId'."}};
    }

    auto* store = m_graph.shapeStore();
    if(!store) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "ShapeStore not available (bridge not initialized)."}};
    }

    std::string error;
    auto target_opt = computeEntityCameraTarget(*store, param["shapeId"].get<uint32_t>(),
                                                param["entityType"].get<std::string>(),
                                                param["localId"].get<uint32_t>(), &error);

    if(!target_opt) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", error}};
    }

    if(progress) {
        progress(0.5, "Computing camera...");
    }

    auto cam = m_graph.viewportState().camera();
    const float dist = cam.distance();

    cam.target = target_opt->center;
    cam.position = cam.target + target_opt->direction * dist;
    cam.up = chooseUpVector(target_opt->direction);
    cam.updateClipping();

    m_graph.viewportState().setCamera(cam);

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true},
            {"action", ACTION_NAME},
            {"camera",
             {{"position", {cam.position.x, cam.position.y, cam.position.z}},
              {"target", {cam.target.x, cam.target.y, cam.target.z}},
              {"up", {cam.up.x, cam.up.y, cam.up.z}}}}};
}

} // namespace OpenGeoLab::Scene
