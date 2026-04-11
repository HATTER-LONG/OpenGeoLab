/**
 * @file scene_module.cpp
 * @brief SceneModule — registers scene actions and bridges signals
 */

#include <opengeolab/scene/scene_module.hpp>

#include <opengeolab/core/logger.hpp>
#include <opengeolab/core/module_data_event.hpp>
#include <opengeolab/scene/add_label_action.hpp>
#include <opengeolab/scene/best_view_for_entity_action.hpp>
#include <opengeolab/scene/capture_viewport_action.hpp>
#include <opengeolab/scene/clear_labels_action.hpp>
#include <opengeolab/scene/clear_selection_action.hpp>
#include <opengeolab/scene/describe_labels_action.hpp>
#include <opengeolab/scene/deselect_action.hpp>
#include <opengeolab/scene/fit_to_scene_action.hpp>
#include <opengeolab/scene/list_nodes_action.hpp>
#include <opengeolab/scene/look_at_entity_action.hpp>
#include <opengeolab/scene/new_model_action.hpp>
#include <opengeolab/scene/pick_area_action.hpp>
#include <opengeolab/scene/query_selection_action.hpp>
#include <opengeolab/scene/remove_label_action.hpp>
#include <opengeolab/scene/select_action.hpp>
#include <opengeolab/scene/set_auto_label_action.hpp>
#include <opengeolab/scene/set_camera_action.hpp>
#include <opengeolab/scene/set_display_mode_action.hpp>
#include <opengeolab/scene/set_hover_action.hpp>
#include <opengeolab/scene/set_labels_visible_action.hpp>
#include <opengeolab/scene/set_pick_mode_action.hpp>
#include <opengeolab/scene/set_view_preset_action.hpp>
#include <opengeolab/scene/set_visibility_action.hpp>

#include <functional>
#include <optional>

namespace OpenGeoLab::Scene {

SceneModule::SceneModule(Kangaroo::Util::PluginComponentFactory& factory)
    : ModuleBase(MODULE_NAME, "Scene state management module.", factory) {
    registerAction<SetVisibilityAction>(std::ref(m_sceneGraph));
    registerAction<ListNodesAction>(std::cref(m_sceneGraph));
    registerAction<SelectAction>(std::ref(m_sceneGraph.selectionState()));
    registerAction<DeselectAction>(std::ref(m_sceneGraph.selectionState()));
    registerAction<ClearSelectionAction>(std::ref(m_sceneGraph.selectionState()));
    registerAction<QuerySelectionAction>(std::cref(m_sceneGraph.selectionState()));
    registerAction<SetPickModeAction>(std::ref(m_sceneGraph.selectionState()));
    registerAction<SetHoverAction>(std::ref(m_sceneGraph.selectionState()));
    registerAction<FitToSceneAction>(std::ref(m_sceneGraph));
    registerAction<NewModelAction>(std::ref(m_sceneGraph), std::ref(factory));
    registerAction<SetViewPresetAction>(std::ref(m_sceneGraph.viewportState()));
    registerAction<SetCameraAction>(std::ref(m_sceneGraph.viewportState()));
    registerAction<SetDisplayModeAction>(std::ref(m_sceneGraph.viewportState()));
    registerAction<PickAreaAction>(std::ref(m_sceneGraph.viewportState()));
    registerAction<DescribeLabelsAction>(std::cref(m_sceneGraph.labelManager()));
    registerAction<AddLabelAction>(std::ref(m_sceneGraph.labelManager()));
    registerAction<RemoveLabelAction>(std::ref(m_sceneGraph.labelManager()));
    registerAction<ClearLabelsAction>(std::ref(m_sceneGraph.labelManager()));
    registerAction<SetLabelsVisibleAction>(std::ref(m_sceneGraph.labelManager()));
    registerAction<SetAutoLabelAction>(std::ref(m_sceneGraph.labelManager()));
    registerAction<CaptureViewportAction>(std::ref(m_sceneGraph));
    registerAction<LookAtEntityAction>(std::ref(m_sceneGraph));
    registerAction<BestViewForEntityAction>(std::ref(m_sceneGraph));

    m_graphConnections.push_back(m_sceneGraph.nodeAdded.connect(
        [this](NodeId) { dataChanged.emit(Core::ModuleDataEvent::ItemAdded); }));
    m_graphConnections.push_back(m_sceneGraph.nodeRemoved.connect(
        [this](NodeId) { dataChanged.emit(Core::ModuleDataEvent::ItemRemoved); }));
    m_graphConnections.push_back(m_sceneGraph.nodeUpdated.connect(
        [this](NodeId) { dataChanged.emit(Core::ModuleDataEvent::ItemModified); }));
    m_graphConnections.push_back(m_sceneGraph.sceneCleared.connect(
        [this]() { dataChanged.emit(Core::ModuleDataEvent::ItemRemoved); }));

    // Selection/hover/label changes wake the viewport but do NOT trigger scene data refresh
    // (SidebarPanel listens to sceneDataChanged; selection/label state is not node data).
    auto& sel = m_sceneGraph.selectionState();
    m_graphConnections.push_back(
        sel.entitiesSelected.connect([this](const std::vector<Core::EntityRef>&) {
            dataChanged.emit(Core::ModuleDataEvent::ViewportChanged);
        }));
    m_graphConnections.push_back(
        sel.entitiesDeselected.connect([this](const std::vector<Core::EntityRef>&) {
            dataChanged.emit(Core::ModuleDataEvent::ViewportChanged);
        }));
    m_graphConnections.push_back(sel.selectionCleared.connect(
        [this]() { dataChanged.emit(Core::ModuleDataEvent::ViewportChanged); }));
    m_graphConnections.push_back(
        sel.hoverChanged.connect([this](const std::optional<Core::EntityRef>&) {
            dataChanged.emit(Core::ModuleDataEvent::ViewportChanged);
        }));

    auto& lbl = m_sceneGraph.labelManager();
    m_graphConnections.push_back(lbl.labelsChanged.connect(
        [this]() { dataChanged.emit(Core::ModuleDataEvent::ViewportChanged); }));
    m_graphConnections.push_back(lbl.visibleChanged.connect(
        [this]() { dataChanged.emit(Core::ModuleDataEvent::ViewportChanged); }));

    // ViewportState changes trigger viewport re-render only (not scene data refresh).
    auto& vps = m_sceneGraph.viewportState();
    m_graphConnections.push_back(vps.cameraChanged.connect(
        [this]() { dataChanged.emit(Core::ModuleDataEvent::ViewportChanged); }));
    m_graphConnections.push_back(vps.pickAreaRequested.connect(
        [this]() { dataChanged.emit(Core::ModuleDataEvent::ViewportChanged); }));
    m_graphConnections.push_back(vps.captureRequested.connect(
        [this]() { dataChanged.emit(Core::ModuleDataEvent::ViewportChanged); }));
    m_graphConnections.push_back(vps.displayModeChanged.connect(
        [this]() { dataChanged.emit(Core::ModuleDataEvent::ViewportChanged); }));
}

SceneModule::~SceneModule() = default;

void SceneModule::initBridge(Geometry::ShapeStore& store) {
    if(m_bridge) {
        return; // Already initialized.
    }
    m_sceneGraph.setShapeStore(&store);
    m_bridge = std::make_unique<GeometrySceneBridge>(m_sceneGraph, store);
    LOG_INFO("SceneModule: GeometrySceneBridge created successfully");
}

SceneGraph& SceneModule::sceneGraph() { return m_sceneGraph; }
const SceneGraph& SceneModule::sceneGraph() const { return m_sceneGraph; }

} // namespace OpenGeoLab::Scene
