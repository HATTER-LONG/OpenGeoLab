#include <opengeolab/render/actions/get_camera_state_action.hpp>

#include <utility>

namespace OpenGeoLab::Render {

GetCameraStateAction::GetCameraStateAction(std::shared_ptr<SceneManager> scene_manager)
    : scene_manager_(std::move(scene_manager)) {}

auto GetCameraStateAction::execute(const nlohmann::json& /*payload*/) -> Base::CommandResult {
    return Base::CommandResult{
        .ok = true,
        .summary = "Camera state retrieved.",
        .result = scene_manager_->camera_state().to_json(),
    };
}

auto GetCameraStateAction::actionName() const noexcept -> std::string_view {
    return "camera.get_state";
}

} // namespace OpenGeoLab::Render
