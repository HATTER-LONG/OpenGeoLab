/// @file mesh_entry.cpp
/// @brief ElementLocator implementation — prefix-sum build and binary search locate.

#include <opengeolab/mesh/mesh_entry.hpp>

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace OpenGeoLab::Mesh {

void ElementLocator::build(const std::vector<ElementBlock>& line_blocks,
                           const std::vector<ElementBlock>& surface_blocks,
                           const std::vector<ElementBlock>& volume_blocks) {
    m_prefixSums.clear();
    m_groups.clear();
    m_blockIndices.clear();

    uint32_t cumulative = 0;

    auto append_blocks = [&](const std::vector<ElementBlock>& blocks, Location::Group group) {
        for(size_t i = 0; i < blocks.size(); ++i) {
            const auto count = static_cast<uint32_t>(blocks[i].elementCount());
            cumulative += count;
            m_prefixSums.push_back(cumulative);
            m_groups.push_back(group);
            m_blockIndices.push_back(i);
        }
    };

    append_blocks(line_blocks, Location::Group::Line);
    append_blocks(surface_blocks, Location::Group::Surface);
    append_blocks(volume_blocks, Location::Group::Volume);
}

ElementLocator::Location ElementLocator::locate(uint32_t element_id) const {
    assert(element_id >= 1 && element_id <= totalCount());

    // Binary search: find first prefix sum >= elementId
    auto it = std::lower_bound(m_prefixSums.begin(), m_prefixSums.end(), element_id);
    auto idx = static_cast<size_t>(std::distance(m_prefixSums.begin(), it));

    uint32_t const block_start = (idx == 0) ? 0 : m_prefixSums[idx - 1];
    auto local_index = static_cast<size_t>(element_id - block_start - 1);

    return {m_groups[idx], m_blockIndices[idx], local_index};
}

uint32_t ElementLocator::totalCount() const {
    return m_prefixSums.empty() ? 0 : m_prefixSums.back();
}

} // namespace OpenGeoLab::Mesh
