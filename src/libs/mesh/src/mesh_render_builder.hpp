/**
 * @file mesh_render_builder.hpp
 * @brief MeshRenderBuilder — converts MeshEntry to RenderMeshData
 */

#pragma once

#include <opengeolab/mesh/mesh_entry.hpp>
#include <opengeolab/mesh/mesh_export.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>

#include <cstdint>

namespace OpenGeoLab::Mesh {

struct OPENGEOLAB_MESH_EXPORT MeshRenderBuilder {
    /**
     * @brief Build GPU-ready render data from a mesh entry.
     *
     * Generates:
     * - triangleRanges: element faces (MeshElement entityType)
     * - lineRanges: unique edges from elements (MeshEdge entityType)
     * - pointRanges: individual nodes (MeshNode entityType)
     */
    [[nodiscard]] static Scene::RenderMeshData build(uint32_t shape_id, const MeshEntry& entry);
};

} // namespace OpenGeoLab::Mesh
