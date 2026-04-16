/**
 * @file mesh_topology.cpp
 * @brief MeshTopology implementation — edge derivation and adjacency building
 */

#include <opengeolab/mesh/mesh_topology.hpp>

#include <opengeolab/mesh/mesh_element_type.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <set>
#include <utility>

namespace OpenGeoLab::Mesh {

namespace {

using EdgeCorners = std::array<uint8_t, 2>;

void appendElementEdges(MeshElementType type, std::vector<EdgeCorners>& out) {
    switch(type) {
    case MeshElementType::Triangle:
        out.insert(out.end(), {{0, 1}, {1, 2}, {2, 0}});
        return;
    case MeshElementType::Quad:
        out.insert(out.end(), {{0, 1}, {1, 2}, {2, 3}, {3, 0}});
        return;
    case MeshElementType::Tetra:
        out.insert(out.end(), {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}});
        return;
    case MeshElementType::Hexa:
        out.insert(out.end(), {{0, 1},
                               {1, 2},
                               {2, 3},
                               {3, 0},
                               {4, 5},
                               {5, 6},
                               {6, 7},
                               {7, 4},
                               {0, 4},
                               {1, 5},
                               {2, 6},
                               {3, 7}});
        return;
    case MeshElementType::Prism:
        out.insert(out.end(),
                   {{0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 5}, {5, 3}, {0, 3}, {1, 4}, {2, 5}});
        return;
    case MeshElementType::Pyramid:
        out.insert(out.end(), {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 4}, {2, 4}, {3, 4}});
        return;
    case MeshElementType::Tri6:
        out.insert(out.end(), {{0, 1}, {1, 2}, {2, 0}});
        return;
    case MeshElementType::Quad8:
    case MeshElementType::Quad9:
        out.insert(out.end(), {{0, 1}, {1, 2}, {2, 3}, {3, 0}});
        return;
    }
}

[[nodiscard]] bool isValidLocalId(uint32_t local_id, std::size_t node_count) {
    return local_id > 0U && local_id <= node_count;
}

[[nodiscard]] bool collectElementNodeIndices(const MeshElement& element,
                                             std::size_t node_count,
                                             std::vector<uint32_t>& node_indices) {
    node_indices.clear();
    node_indices.reserve(nodeCount(element.type));

    for(uint8_t i = 0; i < nodeCount(element.type); ++i) {
        const uint32_t local_id = element.nodeLocalIds[i];
        if(!isValidLocalId(local_id, node_count)) {
            return false;
        }
        node_indices.push_back(local_id - 1U);
    }

    return true;
}

} // namespace

std::optional<uint32_t> MeshTopology::findEdgeIndex(uint32_t node1_local_id,
                                                    uint32_t node2_local_id) const {
    if(node1_local_id == 0U || node2_local_id == 0U) {
        return std::nullopt;
    }

    const uint32_t first = node1_local_id - 1U;
    const uint32_t second = node2_local_id - 1U;
    const auto key = std::make_pair(std::min(first, second), std::max(first, second));
    const auto it = std::lower_bound(edges.begin(), edges.end(), key);
    if(it == edges.end() || *it != key) {
        return std::nullopt;
    }

    return static_cast<uint32_t>(std::distance(edges.begin(), it));
}

std::pair<uint32_t, uint32_t> MeshTopology::resolveEdge(uint32_t local_id) const {
    return edges.at(local_id - 1U);
}

MeshTopology MeshTopology::build(const MeshEntry& entry) {
    MeshTopology topology;
    if(entry.nodes.empty() || entry.elements.empty()) {
        return topology;
    }

    const std::size_t node_count = entry.nodes.size();
    std::set<std::pair<uint32_t, uint32_t>> unique_edges;
    std::vector<uint32_t> node_indices;
    std::vector<EdgeCorners> element_edges;
    element_edges.reserve(12);

    for(const auto& element : entry.elements) {
        if(!collectElementNodeIndices(element, node_count, node_indices)) {
            continue;
        }

        element_edges.clear();
        appendElementEdges(element.type, element_edges);
        for(const auto& edge : element_edges) {
            const uint32_t first = node_indices[edge[0]];
            const uint32_t second = node_indices[edge[1]];
            unique_edges.emplace(std::min(first, second), std::max(first, second));
        }
    }

    topology.edges.assign(unique_edges.begin(), unique_edges.end());
    topology.edgeToElements.resize(topology.edges.size());
    topology.nodeToElements.resize(node_count + 1U);
    topology.nodeToEdges.resize(node_count + 1U);

    for(std::size_t element_index = 0; element_index < entry.elements.size(); ++element_index) {
        const auto& element = entry.elements[element_index];
        if(!collectElementNodeIndices(element, node_count, node_indices)) {
            continue;
        }

        for(uint8_t node_offset = 0; node_offset < nodeCount(element.type); ++node_offset) {
            const uint32_t local_id = element.nodeLocalIds[node_offset];
            topology.nodeToElements[local_id].push_back(static_cast<uint32_t>(element_index));
        }

        element_edges.clear();
        appendElementEdges(element.type, element_edges);
        for(const auto& edge : element_edges) {
            const auto key = std::make_pair(std::min(node_indices[edge[0]], node_indices[edge[1]]),
                                            std::max(node_indices[edge[0]], node_indices[edge[1]]));
            const auto it = std::lower_bound(topology.edges.begin(), topology.edges.end(), key);
            if(it != topology.edges.end() && *it == key) {
                topology
                    .edgeToElements[static_cast<uint32_t>(
                        std::distance(topology.edges.begin(), it))]
                    .push_back(static_cast<uint32_t>(element_index));
            }
        }
    }

    for(uint32_t edge_index = 0; edge_index < topology.edges.size(); ++edge_index) {
        const auto& edge = topology.edges[edge_index];
        topology.nodeToEdges[edge.first + 1U].push_back(edge_index);
        topology.nodeToEdges[edge.second + 1U].push_back(edge_index);
    }

    return topology;
}

} // namespace OpenGeoLab::Mesh
