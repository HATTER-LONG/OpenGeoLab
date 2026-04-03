/**
 * @file set_camera_action.cpp
 * @brief SetCameraAction implementation
 */

#include <opengeolab/scene/set_camera_action.hpp>

#include <opengeolab/scene/viewport_state.hpp>

namespace OpenGeoLab::Scene {

SetCameraAction::SetCameraAction(ViewportState& state) : m_state(state) {}
SetCameraAction::~SetCameraAction() = default;

nlohmann::json SetCameraAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Set camera position, target, and up vectors."},
        {"params",
         {{"position",
           {{"type", "array"}, {"required", true}, {"description", "[x, y, z] eye position"}}},
          {"target",
           {{"type", "array"}, {"required", true}, {"description", "[x, y, z] look-at target"}}},
          {"up",
           {{"type", "array"}, {"required", true}, {"description", "[x, y, z] up direction"}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}}}}};
}

nlohmann::json SetCameraAction::execute(const nlohmann::json& param,
                                        const Core::ProgressCallback& progress) {
    if(!param.contains("position") || !param["position"].is_array() ||
       param["position"].size() != 3) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing or invalid 'position'"}};
    }
    if(!param.contains("target") || !param["target"].is_array() || param["target"].size() != 3) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing or invalid 'target'"}};
    }
    if(!param.contains("up") || !param["up"].is_array() || param["up"].size() != 3) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing or invalid 'up'"}};
    }

    CameraState state;
    state.position = {param["position"][0].get<float>(), param["position"][1].get<float>(),
                      param["position"][2].get<float>()};
    state.target = {param["target"][0].get<float>(), param["target"][1].get<float>(),
                    param["target"][2].get<float>()};
    state.up = {param["up"][0].get<float>(), param["up"][1].get<float>(),
                param["up"][2].get<float>()};
    state.updateClipping();

    m_state.setCamera(state);

    if(progress) {
        progress(1.0, "Done");
    }
    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
