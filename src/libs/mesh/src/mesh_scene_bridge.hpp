#pragma once

#include <opengeolab/mesh/mesh_store.hpp>

namespace OpenGeoLab::Scene {
class SceneGraph;
}

namespace OpenGeoLab::Mesh {

class MeshSceneBridge {
public:
    MeshSceneBridge(Scene::SceneGraph& scene, MeshStore& store);
    ~MeshSceneBridge();

private:
    Scene::SceneGraph& m_scene;
    MeshStore& m_store;
};

} // namespace OpenGeoLab::Mesh
