/**
 * @file set_auto_label_action.cpp
 * @brief SetAutoLabelAction implementation
 */

#include <opengeolab/scene/set_auto_label_action.hpp>

#include <opengeolab/scene/label_manager.hpp>

namespace OpenGeoLab::Scene {

SetAutoLabelAction::SetAutoLabelAction(LabelManager& manager) : m_manager(manager) {}
SetAutoLabelAction::~SetAutoLabelAction() = default;

nlohmann::json SetAutoLabelAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Enable or disable automatic label creation on selection."},
        {"params",
         {{"enabled",
           {{"type", "boolean"},
            {"required", true},
            {"description", "True to enable auto-label, false to disable it."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}}}}};
}

nlohmann::json SetAutoLabelAction::execute(const nlohmann::json& param,
                                           const Core::ProgressCallback& progress) {
    if(!param.contains("enabled") || !param["enabled"].is_boolean()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing or invalid 'enabled'"}};
    }

    m_manager.setAutoLabel(param["enabled"].get<bool>());

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
