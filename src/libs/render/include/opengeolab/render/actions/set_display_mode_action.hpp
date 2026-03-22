/**
 * @file set_display_mode_action.hpp
 * @brief Action to change the rendering display mode.
 */

#pragma once

#include <opengeolab/base/action_interface.hpp>
#include <opengeolab/render/render_export.hpp>
#include <opengeolab/render/scene_manager.hpp>

#include <memory>

namespace OpenGeoLab::Render {

class OPENGEOLAB_RENDER_EXPORT SetDisplayModeAction : public Base::IAction {
public:
    explicit SetDisplayModeAction(std::shared_ptr<SceneManager> scene_manager);
    [[nodiscard]] auto execute(const nlohmann::json& payload) -> Base::CommandResult override;
    [[nodiscard]] auto actionName() const noexcept -> std::string_view override;

private:
    std::shared_ptr<SceneManager> scene_manager_;
};

} // namespace OpenGeoLab::Render
