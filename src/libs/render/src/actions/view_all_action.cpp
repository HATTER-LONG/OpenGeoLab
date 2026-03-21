#include <opengeolab/render/actions/view_all_action.hpp>

#include <nlohmann/json.hpp>

#include <utility>

namespace OpenGeoLab::Render {

ViewAllAction::ViewAllAction(std::shared_ptr<SceneManager> scene_manager)
    : scene_manager_(std::move(scene_manager)) {}

auto ViewAllAction::execute(const nlohmann::json& payload) -> Base::CommandResult {
    const int width = payload.value("width", 800);
    const int height = payload.value("height", 600);
    scene_manager_->view_all(width, height);
    return Base::CommandResult{
        .ok = true,
        .summary = "View fitted to scene.",
        .result = scene_manager_->camera_state().to_json(),
    };
}

auto ViewAllAction::actionName() const noexcept -> std::string_view { return "camera.view_all"; }

} // namespace OpenGeoLab::Render
