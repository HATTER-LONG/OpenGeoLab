/**
 * @file set_pick_mode_action.cpp
 * @brief SetPickModeAction implementation
 */

#include <opengeolab/scene/set_pick_mode_action.hpp>

#include <opengeolab/core/pick_mask.hpp>
#include <opengeolab/scene/selection_state.hpp>

namespace OpenGeoLab::Scene {

SetPickModeAction::SetPickModeAction(SelectionState& state) : m_state(state) {}
SetPickModeAction::~SetPickModeAction() = default;

nlohmann::json SetPickModeAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Update pick mask and pick enabled state."},
        {"params",
         {{"pickMask",
           {{"type", "integer"},
            {"required", false},
            {"description", "Optional uint32 pick mask to apply."}}},
          {"enabled",
           {{"type", "boolean"},
            {"required", false},
            {"description", "Optional flag that enables or disables picking."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}}}}};
}

nlohmann::json SetPickModeAction::execute(const nlohmann::json& param,
                                          const Core::ProgressCallback& progress) {
    if(param.contains("pickMask")) {
        m_state.setPickMask(static_cast<Core::PickMask>(param["pickMask"].get<uint32_t>()));
    }
    if(param.contains("enabled")) {
        m_state.setPickEnabled(param["enabled"].get<bool>());
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
