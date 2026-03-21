#include <opengeolab/render/actions/add_box_action.hpp>

#include <nlohmann/json.hpp>

#include <utility>

namespace OpenGeoLab::Render {

AddBoxAction::AddBoxAction(std::shared_ptr<SceneManager> scene_manager)
    : scene_manager_(std::move(scene_manager)) {}

auto AddBoxAction::execute(const nlohmann::json& payload) -> Base::CommandResult {
    const float size_x = payload.value("sizeX", 1.0F);
    const float size_y = payload.value("sizeY", 1.0F);
    const float size_z = payload.value("sizeZ", 1.0F);
    auto node_id = scene_manager_->add_box(size_x, size_y, size_z);
    return Base::CommandResult{
        .ok = true,
        .summary = "Box added: " + node_id,
        .result = {{"nodeId", node_id}},
    };
}

auto AddBoxAction::actionName() const noexcept -> std::string_view { return "scene.add_box"; }

} // namespace OpenGeoLab::Render
