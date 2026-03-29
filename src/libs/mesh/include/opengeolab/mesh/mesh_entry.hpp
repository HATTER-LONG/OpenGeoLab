/// @file mesh_entry.hpp
/// @brief MeshEntry and ElementLocator — mesh data storage and element lookup.

#pragma once

#include <opengeolab/mesh/mesh_export.hpp>
#include <opengeolab/mesh/mesh_types.hpp>

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/core/visual_data.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace OpenGeoLab::Mesh {

/// @brief Fast element ID locator using prefix-sum array.
///
/// Builds a prefix-sum over all element blocks (line → surface → volume order).
/// `locate()` maps a global 1-based element ID to its block and local index
/// in O(log B) time, where B is the total number of blocks (typically < 100).
class OPENGEOLAB_MESH_EXPORT ElementLocator {
public:
    /// @brief Result of locating an element.
    struct Location {
        enum class Group { Line, Surface, Volume };
        Group group;       ///< Which block group the element belongs to
        size_t blockIndex; ///< Index within the group's block vector
        size_t localIndex; ///< Element's index within the block
    };

    /// @brief Builds the prefix-sum array from all element blocks.
    void build(const std::vector<ElementBlock>& line_blocks,
               const std::vector<ElementBlock>& surface_blocks,
               const std::vector<ElementBlock>& volume_blocks);

    /// @brief Locates an element by its global 1-based ID.
    /// @pre elementId >= 1 && elementId <= totalCount()
    [[nodiscard]] Location locate(uint32_t element_id) const;

    /// @brief Returns the total number of elements across all blocks.
    [[nodiscard]] uint32_t totalCount() const;

private:
    /// Cumulative element count at end of each block: m_prefixSums[i] = total
    /// elements in blocks [0..i]. m_prefixSums.back() == totalCount().
    std::vector<uint32_t> m_prefixSums;
    std::vector<Location::Group> m_groups;
    std::vector<size_t> m_blockIndices;
};

/// @brief Complete mesh data entry managed by MeshStore.
struct MeshEntry {
    uint32_t id{0};                        ///< Mesh ID (assigned by MeshStore, 1-based)
    std::string name;                      ///< User-visible name
    std::optional<uint32_t> sourceShapeId; ///< Source geometry shape ID (optional)

    MeshNodeArray nodes; ///< All mesh nodes

    std::vector<ElementBlock> lineBlocks;    ///< 1D line segment elements
    std::vector<ElementBlock> surfaceBlocks; ///< 2D face elements (tri/quad)
    std::vector<ElementBlock> volumeBlocks;  ///< 3D volume elements (tet/hex/etc.)

    ElementLocator elementLocator; ///< Global element ID → block locator

    // Render and pick caches (reserved for future use)
    std::shared_ptr<Core::VisualData> visualData;
    std::vector<Core::EntityTag> nodeTags;    ///< MeshNode entity tags
    std::vector<Core::EntityTag> edgeTags;    ///< MeshEdge entity tags
    std::vector<Core::EntityTag> elementTags; ///< MeshElement entity tags

    /// @brief Total number of nodes.
    [[nodiscard]] size_t nodeCount() const { return nodes.count(); }

    /// @brief Total number of elements across all blocks.
    [[nodiscard]] uint32_t elementCount() const { return elementLocator.totalCount(); }
};

} // namespace OpenGeoLab::Mesh
