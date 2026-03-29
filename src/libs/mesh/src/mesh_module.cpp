/// @file mesh_module.cpp
/// @brief MeshModule — registers mesh actions and bridges MeshStore signals.

#include <opengeolab/mesh/mesh_module.hpp>

#include <opengeolab/mesh/delete_mesh_action.hpp>
#include <opengeolab/mesh/generate_surface_mesh_action.hpp>
#include <opengeolab/mesh/generate_volume_mesh_action.hpp>
#include <opengeolab/mesh/list_meshes_action.hpp>
#include <opengeolab/mesh/query_mesh_action.hpp>

#include <opengeolab/core/module_data_event.hpp>

namespace OpenGeoLab::Mesh {

MeshModule::MeshModule(Kangaroo::Util::PluginComponentFactory& factory)
    : ModuleBase(MODULE_NAME, "Mesh generation and management module.", factory) {
    // Register actions — generate actions need factory for cross-module GeometryModule access
    registerAction<GenerateSurfaceMeshAction>(std::ref(m_meshStore), std::ref(factory));
    registerAction<GenerateVolumeMeshAction>(std::ref(m_meshStore), std::ref(factory));
    registerAction<DeleteMeshAction>(std::ref(m_meshStore));
    registerAction<QueryMeshAction>(std::ref(m_meshStore));
    registerAction<ListMeshesAction>(std::ref(m_meshStore));

    // Bridge MeshStore signals → ModuleBase::dataChanged
    m_storeConnections.push_back(m_meshStore.meshAdded.connect([this](uint32_t, const MeshEntry&) {
        dataChanged.emit(Core::ModuleDataEvent::ItemAdded);
    }));
    m_storeConnections.push_back(m_meshStore.meshRemoved.connect(
        [this](uint32_t) { dataChanged.emit(Core::ModuleDataEvent::ItemRemoved); }));
    m_storeConnections.push_back(
        m_meshStore.meshUpdated.connect([this](uint32_t, const MeshEntry&) {
            dataChanged.emit(Core::ModuleDataEvent::ItemModified);
        }));
}

MeshModule::~MeshModule() = default;

MeshStore& MeshModule::meshStore() { return m_meshStore; }
const MeshStore& MeshModule::meshStore() const { return m_meshStore; }

} // namespace OpenGeoLab::Mesh
