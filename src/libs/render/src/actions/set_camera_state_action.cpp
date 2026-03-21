#include <opengeolab/render/actions/set_camera_state_action.hpp>

#include <opengeolab/render/camera_state.hpp>

#include <utility>

namespace OpenGeoLab::Render {

SetCameraStateAction::SetCameraStateAction(std::shared_ptr<SceneManager> scene_manager)
    : scene_manager_(std::move(scene_manager)) {}

auto SetCameraStateAction::execute(const nlohmann::json& payload) -> Base::CommandResult {
    auto state = CameraState::from_json(payload);
    scene_manager_->restore_camera_state(state);
    return Base::CommandResult{
        .ok = true,
        .summary = "Camera state restored.",
        .result = scene_manager_->camera_state().to_json(),
    };
}

auto SetCameraStateAction::actionName() const noexcept -> std::string_view {
    return "camera.set_state";
}

} // namespace OpenGeoLab::Render
