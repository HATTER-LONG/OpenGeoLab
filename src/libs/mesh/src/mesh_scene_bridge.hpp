/**
 * @file mesh_scene_bridge.hpp
 * @brief MeshSceneBridge — syncs MeshStore changes to SceneGraph
 */

#pragma once

#include <opengeolab/mesh/mesh_store.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <kangaroo/util/signal.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Mesh {

class MeshSceneBridge {
public:
    MeshSceneBridge(Scene::SceneGraph& scene, MeshStore& store);
    ~MeshSceneBridge();

    MeshSceneBridge(const MeshSceneBridge&) = delete;
    MeshSceneBridge& operator=(const MeshSceneBridge&) = delete;
    MeshSceneBridge(MeshSceneBridge&&) = delete;
    MeshSceneBridge& operator=(MeshSceneBridge&&) = delete;

private:
    void onMeshAdded(uint32_t shape_id, const MeshEntry& entry);
    void onMeshRemoved(uint32_t shape_id);
    void onStoreCleared();

    Scene::SceneGraph& m_scene;
    MeshStore& m_store;
    std::unordered_map<uint32_t, Scene::NodeId> m_meshToNode;
    std::vector<Kangaroo::Util::ScopedConnection> m_connections;
};

} // namespace OpenGeoLab::Mesh
