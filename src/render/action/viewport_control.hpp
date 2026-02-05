/**
 * @file viewport_control.hpp
 * @brief Render action for controlling viewport camera presets and refresh
 */

#pragma once

#include "render/render_action.hpp"

namespace OpenGeoLab::Render {

/**
 * @brief Predefined camera view presets
 */
enum class ViewPreset { Front = 0, Back = 1, Left = 2, Right = 3, Top = 4, Bottom = 5 };

/**
 * @brief Render action that controls camera presets or triggers a refresh
 *
 * Handles view_ctrl action parameters to set camera to predefined views
 * or refresh/fit the scene.
 */
class ViewPortControl : public RenderActionBase {
public:
    ViewPortControl() = default;
    ~ViewPortControl() override = default;

    /**
     * @brief Execute the viewport control action
     * @param params JSON parameters containing view_ctrl object
     * @param progress_callback Progress callback (unused)
     * @return JSON response with status and action name
     */
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;

private:
    /**
     * @brief Apply a camera view preset
     * @param preset The view preset to apply
     */
    void applyPreset(ViewPreset preset);

private:
    ViewPreset m_preset{ViewPreset::Front};
};

class ViewPortControlFactory : public RenderActionFactory {
public:
    ViewPortControlFactory() = default;
    ~ViewPortControlFactory() = default;

    [[nodiscard]] tObjectPtr create() override { return std::make_unique<ViewPortControl>(); }
};
} // namespace OpenGeoLab::Render