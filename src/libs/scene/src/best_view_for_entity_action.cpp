/**
 * @file best_view_for_entity_action.cpp
 * @brief BestViewForEntityAction — optimal camera for viewing an entity
 */

#include <opengeolab/scene/best_view_for_entity_action.hpp>

#include <opengeolab/scene/scene_graph.hpp>

#include "entity_camera_utils.hpp"

#include <algorithm>

namespace OpenGeoLab::Scene {

BestViewForEntityAction::BestViewForEntityAction(SceneGraph& graph) : m_graph(graph) {}
BestViewForEntityAction::~BestViewForEntityAction() = default;

nlohmann::json BestViewForEntityAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Compute and apply the optimal camera to view a face/edge/vertex, "
                        "auto-fitting distance to the entity's bounding box."},
        {"params",
         {{"shapeId",
           {{"type", "integer"}, {"required", true}, {"description", "Shape identifier."}}},
          {"entityType",
           {{"type", "string"},
            {"required", true},
            {"description", "'face', 'edge', or 'vertex'."}}},
          {"localId",
           {{"type", "integer"}, {"required", true}, {"description", "1-based local index."}}},
          {"padding",
           {{"type", "number"},
            {"required", false},
            {"description", "Distance multiplier on bbox diagonal (default 1.5). "
                            "Larger values show more context."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"camera",
           {{"type", "object"},
            {"description", "Resulting camera state: {position, target, up}."}}},
          {"entityBounds",
           {{"type", "object"}, {"description", "Entity AABB: {min: [x,y,z], max: [x,y,z]}."}}}}}};
}

nlohmann::json BestViewForEntityAction::execute(const nlohmann::json& param,
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

    float padding = DEFAULT_PADDING;
    if(param.contains("padding") && param["padding"].is_number()) {
        padding = param["padding"].get<float>();
        padding = std::max(padding, 0.1F);
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

    const float diag =
        target_opt->entityBounds.isValid() ? target_opt->entityBounds.diagonal() : 10.0F;
    const float dist = std::max(diag * padding, 1.0F);

    CameraState cam;
    cam.target = target_opt->center;
    cam.position = cam.target + target_opt->direction * dist;
    cam.up = chooseUpVector(target_opt->direction);
    cam.updateClipping();

    m_graph.viewportState().setCamera(cam);

    if(progress) {
        progress(1.0, "Done");
    }

    nlohmann::json result = {{"ok", true},
                             {"action", ACTION_NAME},
                             {"camera",
                              {{"position", {cam.position.x, cam.position.y, cam.position.z}},
                               {"target", {cam.target.x, cam.target.y, cam.target.z}},
                               {"up", {cam.up.x, cam.up.y, cam.up.z}}}}};

    if(target_opt->entityBounds.isValid()) {
        auto& bb = target_opt->entityBounds;
        result["entityBounds"] = {{"min", {bb.min.x, bb.min.y, bb.min.z}},
                                  {"max", {bb.max.x, bb.max.y, bb.max.z}}};
    }

    return result;
}

} // namespace OpenGeoLab::Scene
