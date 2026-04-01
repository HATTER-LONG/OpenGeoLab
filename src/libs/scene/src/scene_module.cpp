/**
 * @file scene_module.cpp
 * @brief SceneModule — registers scene actions and bridges signals
 */

#include <opengeolab/scene/scene_module.hpp>

#include <opengeolab/core/module_data_event.hpp>
#include <opengeolab/scene/clear_selection_action.hpp>
#include <opengeolab/scene/deselect_action.hpp>
#include <opengeolab/scene/list_nodes_action.hpp>
#include <opengeolab/scene/query_selection_action.hpp>
#include <opengeolab/scene/select_action.hpp>
#include <opengeolab/scene/set_hover_action.hpp>
#include <opengeolab/scene/set_pick_mode_action.hpp>
#include <opengeolab/scene/set_visibility_action.hpp>

#include <functional>

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

    m_graphConnections.push_back(m_sceneGraph.nodeAdded.connect(
        [this](NodeId) { dataChanged.emit(Core::ModuleDataEvent::ItemAdded); }));
    m_graphConnections.push_back(m_sceneGraph.nodeRemoved.connect(
        [this](NodeId) { dataChanged.emit(Core::ModuleDataEvent::ItemRemoved); }));
    m_graphConnections.push_back(m_sceneGraph.nodeUpdated.connect(
        [this](NodeId) { dataChanged.emit(Core::ModuleDataEvent::ItemModified); }));
}

SceneModule::~SceneModule() = default;

SceneGraph& SceneModule::sceneGraph() { return m_sceneGraph; }
const SceneGraph& SceneModule::sceneGraph() const { return m_sceneGraph; }

} // namespace OpenGeoLab::Scene
