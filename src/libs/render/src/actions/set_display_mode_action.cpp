#include <opengeolab/render/actions/set_display_mode_action.hpp>

#include <nlohmann/json.hpp>

#include <utility>

namespace OpenGeoLab::Render {

SetDisplayModeAction::SetDisplayModeAction(std::shared_ptr<SceneManager> scene_manager)
    : scene_manager_(std::move(scene_manager)) {}

auto SetDisplayModeAction::execute(const nlohmann::json& payload) -> Base::CommandResult {
    const auto mode_str = payload.value("mode", "flat_lines");
    const DisplayMode mode =
        (mode_str == "wireframe") ? DisplayMode::kWireframe : DisplayMode::kFlatLines;
    scene_manager_->set_display_mode(mode);
    return Base::CommandResult{
        .ok = true,
        .summary = "Display mode set to " + mode_str,
        .result = {{"mode", mode_str}},
    };
}

auto SetDisplayModeAction::actionName() const noexcept -> std::string_view {
    return "display.set_mode";
}

} // namespace OpenGeoLab::Render
