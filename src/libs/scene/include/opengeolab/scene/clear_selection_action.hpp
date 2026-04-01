/**
 * @file clear_selection_action.hpp
 * @brief ClearSelectionAction — clear the selection state
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SelectionState;

/**
 * @brief Clear all selected entities.
 */
class OPENGEOLAB_SCENE_EXPORT ClearSelectionAction final : public Core::IAction {
public:
    explicit ClearSelectionAction(SelectionState& state);
    ~ClearSelectionAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "clear_selection";

private:
    SelectionState& m_state;
};

} // namespace OpenGeoLab::Scene
