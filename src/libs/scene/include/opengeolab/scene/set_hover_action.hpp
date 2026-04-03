/**
 * @file set_hover_action.hpp
 * @brief SetHoverAction — update hovered entity in the selection state
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SelectionState;

/**
 * @brief Set or clear the currently hovered entity.
 *
 * Param: {"entity": {"shapeId": <int>, "type": <string>, "localId": <int>}}
 */
class OPENGEOLAB_SCENE_EXPORT SetHoverAction final : public Core::IAction {
public:
    explicit SetHoverAction(SelectionState& state);
    ~SetHoverAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "set_hover";

private:
    SelectionState& m_state;
};

} // namespace OpenGeoLab::Scene
