/**
 * @file mesh_topology.hpp
 * @brief MeshTopology — edge and adjacency data derived from MeshEntry
 */

#pragma once

#include <opengeolab/mesh/mesh_entry.hpp>
#include <opengeolab/mesh/mesh_export.hpp>

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace OpenGeoLab::Mesh {

/**
 * @brief Edge topology and adjacency maps derived from a MeshEntry.
 *
 * Uses flat vectors indexed by localId for O(1) lookup.
 * Edge derivation matches MeshRenderBuilder ordering (std::set sorted pairs)
 * to ensure edge localId consistency between rendering and algorithm.
 *
 * Cached in MeshStore — rebuilt automatically on mesh add/modify.
 */
struct OPENGEOLAB_MESH_EXPORT MeshTopology {
    /// edges[i] = sorted (node1, node2) as 0-based indices. Edge localId = i + 1.
    std::vector<std::pair<uint32_t, uint32_t>> edges;

    /// edgeToElements[i] = element indices (0-based) sharing edges[i].
    std::vector<std::vector<uint32_t>> edgeToElements;

    /// nodeToElements[localId] = element indices (0-based) containing this node.
    /// Index 0 is unused (localId is 1-based).
    std::vector<std::vector<uint32_t>> nodeToElements;

    /// nodeToEdges[localId] = edge indices (into edges[]) for edges touching this node.
    /// Index 0 is unused (localId is 1-based).
    std::vector<std::vector<uint32_t>> nodeToEdges;

    /**
     * @brief Find edge index by node localIds (1-based). Order-independent.
     * @return Index into edges[] (0-based), or nullopt if not found.
     * @note O(log E) via binary search on sorted edges vector.
     */
    [[nodiscard]] std::optional<uint32_t> findEdgeIndex(uint32_t node1_local_id,
                                                        uint32_t node2_local_id) const;

    /**
     * @brief Resolve edge localId to its node pair (0-based indices).
     * @param local_id Edge localId (1-based, i.e. edges index + 1).
     * @return Sorted pair of 0-based node indices.
     * @pre local_id >= 1 && local_id <= edges.size()
     */
    [[nodiscard]] std::pair<uint32_t, uint32_t> resolveEdge(uint32_t local_id) const;

    /**
     * @brief Build topology from a MeshEntry.
     *
     * Edge derivation replicates MeshRenderBuilder logic: iterates elements,
     * collects corner-pair edges, inserts into std::set for sorted uniqueness,
     * then converts to flat vector. This ensures edge localId matches pick IDs.
     */
    [[nodiscard]] static MeshTopology build(const MeshEntry& entry);
};

} // namespace OpenGeoLab::Mesh
