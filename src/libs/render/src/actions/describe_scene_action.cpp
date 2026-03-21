#include <opengeolab/render/actions/describe_scene_action.hpp>

#include <utility>

namespace OpenGeoLab::Render {

DescribeSceneAction::DescribeSceneAction(std::shared_ptr<SceneManager> scene_manager)
    : scene_manager_(std::move(scene_manager)) {}

auto DescribeSceneAction::execute(const nlohmann::json& /*payload*/) -> Base::CommandResult {
    return Base::CommandResult{
        .ok = true,
        .summary = "Scene described.",
        .result = scene_manager_->describe_scene(),
    };
}

auto DescribeSceneAction::actionName() const noexcept -> std::string_view {
    return "scene.describe";
}

} // namespace OpenGeoLab::Render
