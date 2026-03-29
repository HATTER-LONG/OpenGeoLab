/// @file scene_bridge.h
/// @brief SceneBridge — synchronizes geometry/mesh module data to SceneGraph.
#pragma once

#include <opengeolab/scene/scene_graph.hpp>

#include <QObject>

#include <string>
#include <unordered_map>

namespace OpenGeoLab::Command {
class CommandDispatcher;
}

namespace OpenGeoLab::App {

/// @brief Bridges module data-change events to the scene graph.
///
/// When geometry or mesh data changes, SceneBridge queries the
/// corresponding module store and synchronizes SceneGraph nodes:
/// new entries → addNode, deleted entries → removeNode,
/// existing entries → updateVisual.
class SceneBridge : public QObject {
    Q_OBJECT

public:
    explicit SceneBridge(Command::CommandDispatcher& dispatcher,
                         Scene::SceneGraph& scene_graph,
                         QObject* parent = nullptr);

    /// @brief Toggle visibility of a geometry shape node.
    Q_INVOKABLE void setGeoVisible(int shapeId, bool visible);

    /// @brief Toggle visibility of a mesh node.
    Q_INVOKABLE void setMeshVisible(int meshId, bool visible);

public Q_SLOTS:
    void onGeometryDataChanged();
    void onMeshDataChanged();

Q_SIGNALS:
    void sceneUpdated();

private:
    void syncGeometryToScene();
    void syncMeshToScene();

    Command::CommandDispatcher& m_dispatcher;
    Scene::SceneGraph& m_sceneGraph;

    std::unordered_map<uint32_t, std::string> m_shapeNodeMap;
    std::unordered_map<uint32_t, std::string> m_meshNodeMap;
};

} // namespace OpenGeoLab::App
