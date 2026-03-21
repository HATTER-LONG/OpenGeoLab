/**
 * @file view_all_action.hpp
 * @brief Action to fit the camera view to all scene objects.
 */

#pragma once

#include <opengeolab/base/action_interface.hpp>
#include <opengeolab/render/render_export.hpp>
#include <opengeolab/render/scene_manager.hpp>

#include <memory>

namespace OpenGeoLab::Render {

class OPENGEOLAB_RENDER_EXPORT ViewAllAction : public Base::IAction {
public:
    explicit ViewAllAction(std::shared_ptr<SceneManager> scene_manager);
    [[nodiscard]] auto execute(const nlohmann::json& payload) -> Base::CommandResult override;
    [[nodiscard]] auto actionName() const noexcept -> std::string_view override;

private:
    std::shared_ptr<SceneManager> scene_manager_;
};

} // namespace OpenGeoLab::Render
