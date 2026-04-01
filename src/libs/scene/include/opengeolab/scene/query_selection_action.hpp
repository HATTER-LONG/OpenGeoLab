/**
 * @file query_selection_action.hpp
 * @brief QuerySelectionAction — return the current selection state
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SelectionState;

/**
 * @brief Return the currently selected entities.
 */
class OPENGEOLAB_SCENE_EXPORT QuerySelectionAction final : public Core::IAction {
public:
    explicit QuerySelectionAction(const SelectionState& state);
    ~QuerySelectionAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "query_selection";

private:
    const SelectionState& m_state;
};

} // namespace OpenGeoLab::Scene
