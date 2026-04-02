/**
 * @file set_camera_action.hpp
 * @brief SetCameraAction — directly set camera position/target/up
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class ViewportState;

/**
 * @brief Set camera position, target, and up vectors directly.
 *
 * Param: {"position": [x,y,z], "target": [x,y,z], "up": [x,y,z]}
 */
class OPENGEOLAB_SCENE_EXPORT SetCameraAction final : public Core::IAction {
public:
    explicit SetCameraAction(ViewportState& state);
    ~SetCameraAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "set_camera";

private:
    ViewportState& m_state;
};

} // namespace OpenGeoLab::Scene
