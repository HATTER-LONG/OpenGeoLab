/**
 * @file clear_labels_action.cpp
 * @brief ClearLabelsAction implementation
 */

#include <opengeolab/scene/clear_labels_action.hpp>

#include <opengeolab/scene/label_manager.hpp>

namespace OpenGeoLab::Scene {

ClearLabelsAction::ClearLabelsAction(LabelManager& manager) : m_manager(manager) {}
ClearLabelsAction::~ClearLabelsAction() = default;

nlohmann::json ClearLabelsAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Remove all active labels."},
        {"params", nlohmann::json::object()},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"cleared",
           {{"type", "integer"}, {"description", "Number of labels removed by the action."}}}}}};
}

nlohmann::json ClearLabelsAction::execute(const nlohmann::json& /*param*/,
                                          const Core::ProgressCallback& progress) {
    const auto cleared = m_manager.labels().size();
    m_manager.clearLabels();

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}, {"cleared", cleared}};
}

} // namespace OpenGeoLab::Scene
