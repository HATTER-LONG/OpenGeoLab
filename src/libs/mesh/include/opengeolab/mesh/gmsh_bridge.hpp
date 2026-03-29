/// @file gmsh_bridge.hpp
/// @brief Gmsh bridge — converts OCC shapes to mesh data via Gmsh.

#pragma once

#include <opengeolab/mesh/mesh_entry.hpp>
#include <opengeolab/mesh/mesh_export.hpp>
#include <opengeolab/mesh/mesh_params.hpp>

#include <opengeolab/core/progress_callback.hpp>

class TopoDS_Shape;

namespace OpenGeoLab::Mesh {

/// @brief RAII guard for Gmsh initialize/finalize lifecycle.
///
/// Calls gmsh::initialize() on construction and gmsh::finalize() on
/// destruction. Thread-safe: acquires a global mutex to serialize all
/// Gmsh operations (Gmsh uses global state internally).
class OPENGEOLAB_MESH_EXPORT GmshSession {
public:
    GmshSession();
    ~GmshSession();
    GmshSession(const GmshSession&) = delete;
    GmshSession& operator=(const GmshSession&) = delete;
};

/// @brief Gmsh bridge functions — convert OCC shapes to mesh data.
namespace GmshBridge {

/// @brief Generate a 2D surface mesh from an OCC shape.
/// @param shape OCC shape containing at least one face
/// @param params Surface meshing parameters
/// @param progress Progress callback (return false to cancel)
/// @return MeshEntry with nodes and surface element blocks (id not yet assigned)
OPENGEOLAB_MESH_EXPORT
MeshEntry generateSurfaceMesh(const TopoDS_Shape& shape,
                              const SurfaceMeshParams& params,
                              const Core::ProgressCallback& progress);

/// @brief Generate a 3D volume mesh from an OCC shape.
/// @param shape OCC shape containing at least one solid
/// @param params Volume meshing parameters
/// @param progress Progress callback (return false to cancel)
/// @return MeshEntry with nodes, surface and volume element blocks (id not yet assigned)
OPENGEOLAB_MESH_EXPORT
MeshEntry generateVolumeMesh(const TopoDS_Shape& shape,
                             const VolumeMeshParams& params,
                             const Core::ProgressCallback& progress);

} // namespace GmshBridge
} // namespace OpenGeoLab::Mesh
