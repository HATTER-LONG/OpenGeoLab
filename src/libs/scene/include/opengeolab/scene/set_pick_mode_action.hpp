/**
 * @file set_pick_mode_action.hpp
 * @brief SetPickModeAction — update pick configuration on the selection state
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SelectionState;

/**
 * @brief Update pick mask and pick enabled state.
 */
class OPENGEOLAB_SCENE_EXPORT SetPickModeAction final : public Core::IAction {
public:
    explicit SetPickModeAction(SelectionState& state);
    ~SetPickModeAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "set_pick_mode";

private:
    SelectionState& m_state;
};

} // namespace OpenGeoLab::Scene
