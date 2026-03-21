/**
 * @file render_module.hpp
 * @brief Declares the render module service for 3D scene and camera management.
 */

#pragma once

#include <opengeolab/base/module_service_interface.hpp>
#include <opengeolab/render/render_export.hpp>
#include <opengeolab/render/scene_manager.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Routes render actions (camera, scene) through the plugin component factory.
 *
 * Owns a shared SceneManager instance that is injected into all render actions.
 * CoinQuickItem accesses the SceneManager via this module for GL rendering.
 */
class OPENGEOLAB_RENDER_EXPORT RenderModule : public Base::IModuleService {
public:
    explicit RenderModule(Kangaroo::Util::PluginComponentFactory& factory);
    ~RenderModule() override;

    [[nodiscard]] auto moduleName() const noexcept -> std::string_view override;
    [[nodiscard]] auto dispatch(std::string_view action, const nlohmann::json& payload)
        -> Base::CommandResult override;
    [[nodiscard]] auto supportedActions() const -> std::vector<std::string> override;

    /// @return Shared SceneManager for CoinQuickItem access.
    [[nodiscard]] auto scene_manager() const -> std::shared_ptr<SceneManager>;

private:
    Kangaroo::Util::PluginComponentFactory& factory_;
    std::shared_ptr<SceneManager> scene_manager_;
};

} // namespace OpenGeoLab::Render
