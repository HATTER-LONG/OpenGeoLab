/**
 * @file render_module.cpp
 * @brief RenderModule implementation routing actions to SceneManager.
 */

#include <opengeolab/render/render_module.hpp>

#include <opengeolab/base/action_interface.hpp>

#include <nlohmann/json.hpp>

#include <string>

namespace OpenGeoLab::Render {

RenderModule::RenderModule(Kangaroo::Util::PluginComponentFactory& factory)
    : factory_(factory), scene_manager_(std::make_shared<SceneManager>()) {
    scene_manager_->initialize();
}

RenderModule::~RenderModule() = default;

auto RenderModule::moduleName() const noexcept -> std::string_view { return "render"; }

auto RenderModule::dispatch(std::string_view action, const nlohmann::json& payload)
    -> Base::CommandResult {
    const auto factory_key = std::string(moduleName()) + "." + std::string(action);
    const auto request = Kangaroo::Util::ComponentCreateRequest::from(scene_manager_);

    try {
        auto action_ptr = factory_.create<Base::IAction>(factory_key, request);
        return action_ptr->execute(payload);
    } catch(const Kangaroo::Util::ComponentFactoryNotRegisteredEx&) {
        return Base::CommandResult{
            .ok = false,
            .summary = "Unknown action: " + std::string(action),
            .result = nlohmann::json::object(),
        };
    }
}

auto RenderModule::supportedActions() const -> std::vector<std::string> {
    return {"camera.get_state", "camera.set_state", "camera.view_all", "scene.add_box",
            "scene.describe", "display.set_mode"};
}

auto RenderModule::scene_manager() const -> std::shared_ptr<SceneManager> { return scene_manager_; }

} // namespace OpenGeoLab::Render
