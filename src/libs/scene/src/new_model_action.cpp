/**
 * @file new_model_action.cpp
 * @brief NewModelAction implementation
 */

#include <opengeolab/scene/new_model_action.hpp>

#include <opengeolab/core/module.hpp>
#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/scene/camera_state.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

namespace OpenGeoLab::Scene {

NewModelAction::NewModelAction(SceneGraph& graph,
                               Kangaroo::Util::PluginComponentFactory& factory)
    : m_graph(graph), m_factory(factory) {}

NewModelAction::~NewModelAction() = default;

nlohmann::json NewModelAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Clear all data and reset workspace to initial state."},
        {"params", nlohmann::json::object()},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}}}}};
}

nlohmann::json NewModelAction::execute(const nlohmann::json& /*param*/,
                                       const Core::ProgressCallback& progress) {
    // 1. Clear geometry — cascades to bridge which clears its own maps.
    auto geo_base = m_factory.getSharedInstance<Core::ModuleBase>("geometry");
    if(geo_base) {
        auto* geo_module = dynamic_cast<Geometry::GeometryModule*>(geo_base.get());
        if(geo_module != nullptr) {
            geo_module->clearAll();
        }
    }

    // 2. Clear all scene state: nodes, selection, labels, hover.
    m_graph.clear();

    // 3. Reset camera to default position.
    CameraState default_camera;
    default_camera.reset();
    m_graph.viewportState().setCamera(default_camera);

    if(progress) {
        progress(1.0, "Done");
    }
    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
