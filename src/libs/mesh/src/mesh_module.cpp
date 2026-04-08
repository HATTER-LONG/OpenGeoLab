/**
 * @file mesh_module.cpp
 * @brief MeshModule implementation — action registration and bridge wiring
 */

#include <opengeolab/mesh/mesh_module.hpp>

#include "action/clear_mesh_action.hpp"
#ifdef OPENGEOLAB_USE_GMSH
#include "action/generate_mesh_action.hpp"
#endif
#include "action/query_mesh_info_action.hpp"
#include "mesh_scene_bridge.hpp"

#include <opengeolab/core/logger.hpp>

#include <functional>

namespace OpenGeoLab::Mesh {

MeshModule::MeshModule(Kangaroo::Util::PluginComponentFactory& factory)
    : Core::ModuleBase("mesh", "Mesh generation, query and management", factory) {
    registerAction<ClearMeshAction>(std::ref(m_meshStore));
    registerAction<QueryMeshInfoAction>(std::cref(m_meshStore));
}

MeshModule::~MeshModule() = default;

MeshStore& MeshModule::meshStore() { return m_meshStore; }

const MeshStore& MeshModule::meshStore() const { return m_meshStore; }

void MeshModule::initBridge(Scene::SceneGraph& scene, Geometry::ShapeStore& store) {
    if(m_shapeStore == nullptr) {
        m_shapeStore = &store;
#ifdef OPENGEOLAB_USE_GMSH
        registerAction<GenerateMeshAction>(std::ref(m_meshStore), std::ref(*m_shapeStore));
#endif
    }

    if(m_bridge) {
        return; // Already initialized.
    }
    m_bridge = std::make_unique<MeshSceneBridge>(scene, m_meshStore);
    LOG_INFO("MeshModule: bridge initialized");
}

} // namespace OpenGeoLab::Mesh
