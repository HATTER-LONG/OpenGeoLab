/**
 * @file mesh_entry.hpp
 * @brief MeshEntry — per-shape mesh data container
 */

#pragma once

#include <opengeolab/mesh/mesh_element.hpp>
#include <opengeolab/mesh/mesh_node.hpp>

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Mesh {

/**
 * @brief All mesh data for one geometry shape.
 *
 * Nodes and elements are stored in contiguous vectors.
 * localId for any item is `vector_index + 1` (1-based, matching geometry convention).
 *
 * @note Edge data is not stored — extracted on-the-fly by MeshRenderBuilder.
 */
struct MeshEntry {
    uint32_t shapeId{};                ///< Source geometry shape ID
    std::vector<MeshNode> nodes;       ///< Mesh nodes (localId = index + 1)
    std::vector<MeshElement> elements; ///< Mesh elements (localId = index + 1)
    uint64_t version{0};               ///< Monotonic dirty counter

    /// Increment version to signal data change.
    void markUpdated() { ++version; }
};

} // namespace OpenGeoLab::Mesh
