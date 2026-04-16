/**
 * @file mesh_split_algorithm.hpp
 * @brief MeshSplitAlgorithm — mesh element subdivision by edge/node selection
 */

#pragma once

#include <opengeolab/mesh/mesh_entry.hpp>
#include <opengeolab/mesh/mesh_export.hpp>
#include <opengeolab/mesh/mesh_topology.hpp>
#include <opengeolab/mesh/split_mode.hpp>
#include <opengeolab/mesh/split_result.hpp>

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace OpenGeoLab::Mesh {

/**
 * @brief Computes mesh element splits based on edge/node selection.
 *
 * Stateless algorithm class. Method names correspond to imMeshSpliter
 * for cross-referencing with the reference implementation.
 *
 * The compute() method produces a SplitResult that the caller applies
 * to the MeshEntry in a single transaction via MeshStore::modifyMesh().
 */
class OPENGEOLAB_MESH_EXPORT MeshSplitAlgorithm {
public:
    /**
     * @brief Compute split result for the given selections.
     *
     * @param entry Source mesh data (read-only)
     * @param topology Cached topology for the mesh
     * @param selected_edge_local_ids Edge localIds selected by user (1-based)
     * @param selected_node_local_ids Node localIds selected by user (1-based)
     * @param mode Desired split mode (may be overridden per-element by auto-selection)
     * @return SplitResult containing new nodes and element replacements
     */
    [[nodiscard]] SplitResult compute(const MeshEntry& entry,
                                      const MeshTopology& topology,
                                      const std::vector<uint32_t>& selected_edge_local_ids,
                                      const std::vector<uint32_t>& selected_node_local_ids,
                                      SplitMode mode) const;

private:
    /// Context passed through split processing to accumulate results.
    struct SplitContext {
        const MeshEntry& entry;
        const MeshTopology& topology;
        SplitResult& result;
        std::unordered_set<uint32_t> processedElements; ///< Element indices already handled

        /// Map from edge index (in topology.edges) to new node localId (1-based).
        /// Ensures each edge midpoint is created only once.
        std::unordered_map<uint32_t, uint32_t> edgeMidpointNodes;

        /// The next localId for new nodes (= entry.nodes.size() + 1 initially,
        /// incremented as new nodes are added).
        uint32_t nextNodeLocalId{};
    };

    /// Add a midpoint node for the given edge, or return existing one.
    uint32_t getOrCreateMidpoint(SplitContext& ctx, uint32_t edge_index) const;

    /// Add a centroid node for the given element.
    uint32_t createCentroid(SplitContext& ctx, uint32_t elem_index) const;

    /// Process a triangle element with selected edges.
    void processTriangleEdges(SplitContext& ctx,
                              uint32_t elem_index,
                              const std::vector<uint32_t>& edge_indices,
                              SplitMode mode) const;

    /// Process a quad element with selected edges.
    void processQuadEdges(SplitContext& ctx,
                          uint32_t elem_index,
                          const std::vector<uint32_t>& edge_indices,
                          SplitMode mode) const;

    /// Process a triangle element with selected nodes (TriaThree).
    void processTriangleNodes(SplitContext& ctx, uint32_t elem_index) const;

    /// Propagate split to neighbor element sharing an edge.
    void processNeighborCut(SplitContext& ctx,
                            uint32_t neighbor_elem_index,
                            uint32_t shared_edge_index) const;

    /// Find the node in an element that is opposite to a given edge.
    [[nodiscard]] static uint32_t findOppositeNode(const MeshElement& element,
                                                   uint32_t edge_node1_local_id,
                                                   uint32_t edge_node2_local_id);

    /// Create a MeshElement helper.
    [[nodiscard]] static MeshElement makeTriangle(uint32_t n1, uint32_t n2, uint32_t n3);
    [[nodiscard]] static MeshElement makeQuad(uint32_t n1, uint32_t n2, uint32_t n3, uint32_t n4);
};

} // namespace OpenGeoLab::Mesh
