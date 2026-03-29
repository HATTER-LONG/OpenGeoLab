/// @file mesh_visual_builder.hpp
/// @brief Converts MeshEntry data to Core::VisualData for GPU rendering.

#pragma once

#include <opengeolab/mesh/mesh_entry.hpp>
#include <opengeolab/mesh/mesh_export.hpp>

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/core/visual_data.hpp>

#include <vector>

namespace OpenGeoLab::Mesh::MeshVisualBuilder {

/// @brief Build render-ready VisualData from a MeshEntry.
///
/// For 2D surface elements → SurfaceMesh (direct face rendering with normals).
/// For 3D volume elements → SurfaceMesh (boundary faces only, internal faces culled).
/// Element wireframe → EdgeMesh.
/// All nodes → PointSet.
OPENGEOLAB_MESH_EXPORT
Core::VisualData buildVisualData(const MeshEntry& entry);

/// @brief Entity tag collections for pick support.
struct MeshTags {
    std::vector<Core::EntityTag> nodeTags;    ///< One per node
    std::vector<Core::EntityTag> edgeTags;    ///< One per wireframe line segment
    std::vector<Core::EntityTag> elementTags; ///< One per rendered triangle/quad
};

/// @brief Build entity tags for future pick support.
OPENGEOLAB_MESH_EXPORT
MeshTags buildEntityTags(const MeshEntry& entry);

} // namespace OpenGeoLab::Mesh::MeshVisualBuilder
