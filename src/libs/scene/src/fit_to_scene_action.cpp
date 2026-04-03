/**
 * @file fit_to_scene_action.cpp
 * @brief FitToSceneAction implementation
 */

#include <opengeolab/scene/fit_to_scene_action.hpp>

#include <opengeolab/scene/scene_graph.hpp>

namespace OpenGeoLab::Scene {

FitToSceneAction::FitToSceneAction(SceneGraph& graph) : m_graph(graph) {}
FitToSceneAction::~FitToSceneAction() = default;

nlohmann::json FitToSceneAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Fit camera to the scene bounding box."},
        {"params", nlohmann::json::object()},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}}}}};
}

nlohmann::json FitToSceneAction::execute(const nlohmann::json& /*param*/,
                                         const Core::ProgressCallback& progress) {
    const auto bounds = m_graph.sceneBounds();
    m_graph.viewportState().fitToBounds(bounds);

    if(progress) {
        progress(1.0, "Done");
    }
    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
