// Stub — replaced in Task 3
#include <opengeolab/mesh/mesh_module.hpp>

#include "mesh_scene_bridge.hpp"

namespace OpenGeoLab::Mesh {

MeshModule::MeshModule(Kangaroo::Util::PluginComponentFactory& factory)
    : Core::ModuleBase("mesh", "Mesh generation and query", factory) {}

MeshModule::~MeshModule() = default;

MeshStore& MeshModule::meshStore() { return m_meshStore; }

const MeshStore& MeshModule::meshStore() const { return m_meshStore; }

void MeshModule::initBridge(Scene::SceneGraph& /*scene*/, Geometry::ShapeStore& /*store*/) {}

} // namespace OpenGeoLab::Mesh
