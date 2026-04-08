/**
 * @file set_display_mode_action.hpp
 * @brief SetDisplayModeAction — control viewport rendering display modes
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class ViewportState;

/**
 * @brief Control viewport rendering display modes (x-ray, tessellation overlay).
 *
 * Both parameters are optional — omitted fields keep their current value.
 * The response always echoes the current state of all fields.
 */
class OPENGEOLAB_SCENE_EXPORT SetDisplayModeAction final : public Core::IAction {
public:
    explicit SetDisplayModeAction(ViewportState& state);
    ~SetDisplayModeAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "set_display_mode";

private:
    ViewportState& m_state;
};

} // namespace OpenGeoLab::Scene
