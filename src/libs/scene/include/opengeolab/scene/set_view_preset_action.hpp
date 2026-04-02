/**
 * @file set_view_preset_action.hpp
 * @brief SetViewPresetAction — apply a standard camera view preset
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class ViewportState;

/**
 * @brief Apply a named camera view preset.
 *
 * Param: {"preset": "Front"|"Back"|"Top"|"Bottom"|"Left"|"Right"|"Isometric"}
 */
class OPENGEOLAB_SCENE_EXPORT SetViewPresetAction final : public Core::IAction {
public:
    explicit SetViewPresetAction(ViewportState& state);
    ~SetViewPresetAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "set_view_preset";

private:
    ViewportState& m_state;
};

} // namespace OpenGeoLab::Scene
