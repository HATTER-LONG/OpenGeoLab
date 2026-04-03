/**
 * @file mesh_module.cpp
 * @brief MeshModule implementation — action registration and bridge wiring
 */

#include <opengeolab/mesh/mesh_module.hpp>

#include "mesh_scene_bridge.hpp"

#include <opengeolab/core/logger.hpp>

namespace OpenGeoLab::Mesh {

MeshModule::MeshModule(Kangaroo::Util::PluginComponentFactory& factory)
    : Core::ModuleBase("mesh", "Mesh generation, query and management", factory) {
    // Actions will be registered in Task 4 once they exist.
}

MeshModule::~MeshModule() = default;

MeshStore& MeshModule::meshStore() { return m_meshStore; }

const MeshStore& MeshModule::meshStore() const { return m_meshStore; }

void MeshModule::initBridge(Scene::SceneGraph& scene, Geometry::ShapeStore& /*store*/) {
    if(m_bridge) {
        return; // Already initialized.
    }
    m_bridge = std::make_unique<MeshSceneBridge>(scene, m_meshStore);
    LOG_INFO("MeshModule: bridge initialized");
}

} // namespace OpenGeoLab::Mesh
