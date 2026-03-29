/// @file mesh_module.hpp
/// @brief MeshModule — mesh generation and management module.

#pragma once

#include <opengeolab/core/module.hpp>
#include <opengeolab/mesh/mesh_export.hpp>
#include <opengeolab/mesh/mesh_store.hpp>

#include <kangaroo/util/signal.hpp>

#include <vector>

namespace OpenGeoLab::Mesh {

/// @brief Mesh generation and management module.
///
/// Owns a MeshStore and registers 5 actions:
/// generate_surface_mesh, generate_volume_mesh, delete_mesh, query_mesh, list_meshes.
class OPENGEOLAB_MESH_EXPORT MeshModule final : public Core::ModuleBase {
public:
    static constexpr std::string_view MODULE_NAME{"mesh"};

    explicit MeshModule(Kangaroo::Util::PluginComponentFactory& factory);
    ~MeshModule() override;

    /// @brief Access the mesh store owned by this module.
    [[nodiscard]] MeshStore& meshStore();
    [[nodiscard]] const MeshStore& meshStore() const;

private:
    MeshStore m_meshStore;
    std::vector<Kangaroo::Util::ScopedConnection> m_storeConnections;
};

} // namespace OpenGeoLab::Mesh
