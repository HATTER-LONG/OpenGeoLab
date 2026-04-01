/**
 * @file clear_selection_action.cpp
 * @brief ClearSelectionAction implementation
 */

#include <opengeolab/scene/clear_selection_action.hpp>

#include <opengeolab/scene/selection_state.hpp>

namespace OpenGeoLab::Scene {

ClearSelectionAction::ClearSelectionAction(SelectionState& state) : m_state(state) {}
ClearSelectionAction::~ClearSelectionAction() = default;

nlohmann::json ClearSelectionAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Clear all selected entities."},
        {"params", nlohmann::json::object()},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}}}}};
}

nlohmann::json ClearSelectionAction::execute(const nlohmann::json& param,
                                             const Core::ProgressCallback& progress) {
    static_cast<void>(param);
    m_state.clearSelection();

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
