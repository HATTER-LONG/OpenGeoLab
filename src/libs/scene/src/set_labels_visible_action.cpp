/**
 * @file set_labels_visible_action.cpp
 * @brief SetLabelsVisibleAction implementation
 */

#include <opengeolab/scene/set_labels_visible_action.hpp>

#include <opengeolab/scene/label_manager.hpp>

namespace OpenGeoLab::Scene {

SetLabelsVisibleAction::SetLabelsVisibleAction(LabelManager& manager) : m_manager(manager) {}
SetLabelsVisibleAction::~SetLabelsVisibleAction() = default;

nlohmann::json SetLabelsVisibleAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Enable or disable viewport label rendering."},
        {"params",
         {{"visible",
           {{"type", "boolean"},
            {"required", true},
            {"description", "True to show labels, false to hide them."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}}}}};
}

nlohmann::json SetLabelsVisibleAction::execute(const nlohmann::json& param,
                                               const Core::ProgressCallback& progress) {
    if(!param.contains("visible") || !param["visible"].is_boolean()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing or invalid 'visible'"}};
    }

    m_manager.setVisible(param["visible"].get<bool>());

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
