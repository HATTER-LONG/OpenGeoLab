#include "mesh_scene_bridge.hpp"

namespace OpenGeoLab::Mesh {

MeshSceneBridge::MeshSceneBridge(Scene::SceneGraph& scene, MeshStore& store)
    : m_scene(scene), m_store(store) {}

MeshSceneBridge::~MeshSceneBridge() = default;

} // namespace OpenGeoLab::Mesh
