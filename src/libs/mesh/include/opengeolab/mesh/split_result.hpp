/**
 * @file split_result.hpp
 * @brief SplitResult — output of MeshSplitAlgorithm::compute()
 */

#pragma once

#include <opengeolab/mesh/mesh_element.hpp>

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Mesh {

/**
 * @brief Result of a mesh split computation.
 *
 * Contains new nodes to append and element replacements to apply.
 * The caller applies these to the MeshEntry in a single transaction.
 */
struct SplitResult {
    /// A new node to insert (double precision for computation accuracy).
    struct NewNode {
        double x{};
        double y{};
        double z{};
    };

    /// One original element replaced by one or more new elements.
    struct ElementReplacement {
        uint32_t originalIndex{};             ///< Index in MeshEntry::elements (0-based)
        std::vector<MeshElement> newElements; ///< Replacement elements
    };

    std::vector<NewNode> newNodes;                ///< Nodes to append to MeshEntry::nodes
    std::vector<ElementReplacement> replacements; ///< Elements to replace
};

} // namespace OpenGeoLab::Mesh
