/**
 * @file mesh_split_algorithm.cpp
 * @brief MeshSplitAlgorithm implementation
 */

#include <opengeolab/mesh/mesh_split_algorithm.hpp>

#include <opengeolab/core/logger.hpp>
#include <opengeolab/mesh/mesh_element_type.hpp>

#include <algorithm>
#include <array>
#include <stdexcept>

namespace OpenGeoLab::Mesh {

MeshElement MeshSplitAlgorithm::makeTriangle(uint32_t n1, uint32_t n2, uint32_t n3) {
    MeshElement element{};
    element.type = MeshElementType::Triangle;
    element.nodeLocalIds = {n1, n2, n3, 0, 0, 0, 0, 0};
    return element;
}

MeshElement MeshSplitAlgorithm::makeQuad(uint32_t n1, uint32_t n2, uint32_t n3, uint32_t n4) {
    MeshElement element{};
    element.type = MeshElementType::Quad;
    element.nodeLocalIds = {n1, n2, n3, n4, 0, 0, 0, 0};
    return element;
}

uint64_t MeshSplitAlgorithm::packNodePair(uint32_t node_a, uint32_t node_b) {
    const auto lo = std::min(node_a, node_b);
    const auto hi = std::max(node_a, node_b);
    return (static_cast<uint64_t>(hi) << 32U) | static_cast<uint64_t>(lo);
}

void MeshSplitAlgorithm::getNodePosition(
    const SplitContext& ctx, uint32_t local_id, double& out_x, double& out_y, double& out_z) const {
    if(local_id <= ctx.originalNodeCount) {
        const auto& pos = ctx.entry.nodes[local_id - 1U].position;
        out_x = static_cast<double>(pos[0]);
        out_y = static_cast<double>(pos[1]);
        out_z = static_cast<double>(pos[2]);
    } else {
        const auto& node = ctx.result.newNodes[local_id - ctx.originalNodeCount - 1U];
        out_x = node.x;
        out_y = node.y;
        out_z = node.z;
    }
}

uint32_t MeshSplitAlgorithm::getOrCreateMidpointByNodes(SplitContext& ctx,
                                                        uint32_t node_a,
                                                        uint32_t node_b) const {
    const uint64_t key = packNodePair(node_a, node_b);
    const auto existing = ctx.nodePairMidpoints.find(key);
    if(existing != ctx.nodePairMidpoints.end()) {
        return existing->second;
    }

    double ax = 0.0;
    double ay = 0.0;
    double az = 0.0;
    double bx = 0.0;
    double by = 0.0;
    double bz = 0.0;
    getNodePosition(ctx, node_a, ax, ay, az);
    getNodePosition(ctx, node_b, bx, by, bz);

    SplitResult::NewNode midpoint{};
    midpoint.x = (ax + bx) / 2.0;
    midpoint.y = (ay + by) / 2.0;
    midpoint.z = (az + bz) / 2.0;

    ctx.result.newNodes.push_back(midpoint);
    const uint32_t new_local_id = ctx.nextNodeLocalId++;
    ctx.nodePairMidpoints[key] = new_local_id;
    return new_local_id;
}

void MeshSplitAlgorithm::seedMidEdgeNodes(SplitContext& ctx) const {
    for(uint32_t i = 0; i < ctx.entry.elements.size(); ++i) {
        const auto& elem = ctx.entry.elements[i];

        if(elem.type == MeshElementType::Tri6) {
            const uint32_t c0 = elem.nodeLocalIds[0];
            const uint32_t c1 = elem.nodeLocalIds[1];
            const uint32_t c2 = elem.nodeLocalIds[2];
            const uint32_t m01 = elem.nodeLocalIds[3];
            const uint32_t m12 = elem.nodeLocalIds[4];
            const uint32_t m20 = elem.nodeLocalIds[5];

            if(const auto e = ctx.topology.findEdgeIndex(c0, c1)) {
                ctx.edgeMidpointNodes.emplace(e.value(), m01);
            }
            if(const auto e = ctx.topology.findEdgeIndex(c1, c2)) {
                ctx.edgeMidpointNodes.emplace(e.value(), m12);
            }
            if(const auto e = ctx.topology.findEdgeIndex(c2, c0)) {
                ctx.edgeMidpointNodes.emplace(e.value(), m20);
            }

            ctx.nodePairMidpoints.emplace(packNodePair(c0, c1), m01);
            ctx.nodePairMidpoints.emplace(packNodePair(c1, c2), m12);
            ctx.nodePairMidpoints.emplace(packNodePair(c2, c0), m20);
        } else if(elem.type == MeshElementType::Quad8) {
            const uint32_t c0 = elem.nodeLocalIds[0];
            const uint32_t c1 = elem.nodeLocalIds[1];
            const uint32_t c2 = elem.nodeLocalIds[2];
            const uint32_t c3 = elem.nodeLocalIds[3];
            const uint32_t m01 = elem.nodeLocalIds[4];
            const uint32_t m12 = elem.nodeLocalIds[5];
            const uint32_t m23 = elem.nodeLocalIds[6];
            const uint32_t m30 = elem.nodeLocalIds[7];

            if(const auto e = ctx.topology.findEdgeIndex(c0, c1)) {
                ctx.edgeMidpointNodes.emplace(e.value(), m01);
            }
            if(const auto e = ctx.topology.findEdgeIndex(c1, c2)) {
                ctx.edgeMidpointNodes.emplace(e.value(), m12);
            }
            if(const auto e = ctx.topology.findEdgeIndex(c2, c3)) {
                ctx.edgeMidpointNodes.emplace(e.value(), m23);
            }
            if(const auto e = ctx.topology.findEdgeIndex(c3, c0)) {
                ctx.edgeMidpointNodes.emplace(e.value(), m30);
            }

            ctx.nodePairMidpoints.emplace(packNodePair(c0, c1), m01);
            ctx.nodePairMidpoints.emplace(packNodePair(c1, c2), m12);
            ctx.nodePairMidpoints.emplace(packNodePair(c2, c3), m23);
            ctx.nodePairMidpoints.emplace(packNodePair(c3, c0), m30);
        }
    }
}

void MeshSplitAlgorithm::upgradeReplacementToSecondOrder(
    SplitContext& ctx, SplitResult::ElementReplacement& rep) const {
    for(auto& child : rep.newElements) {
        if(child.type == MeshElementType::Triangle) {
            const uint32_t c0 = child.nodeLocalIds[0];
            const uint32_t c1 = child.nodeLocalIds[1];
            const uint32_t c2 = child.nodeLocalIds[2];
            child.nodeLocalIds[3] = getOrCreateMidpointByNodes(ctx, c0, c1);
            child.nodeLocalIds[4] = getOrCreateMidpointByNodes(ctx, c1, c2);
            child.nodeLocalIds[5] = getOrCreateMidpointByNodes(ctx, c2, c0);
            child.type = MeshElementType::Tri6;
        } else if(child.type == MeshElementType::Quad) {
            const uint32_t c0 = child.nodeLocalIds[0];
            const uint32_t c1 = child.nodeLocalIds[1];
            const uint32_t c2 = child.nodeLocalIds[2];
            const uint32_t c3 = child.nodeLocalIds[3];
            child.nodeLocalIds[4] = getOrCreateMidpointByNodes(ctx, c0, c1);
            child.nodeLocalIds[5] = getOrCreateMidpointByNodes(ctx, c1, c2);
            child.nodeLocalIds[6] = getOrCreateMidpointByNodes(ctx, c2, c3);
            child.nodeLocalIds[7] = getOrCreateMidpointByNodes(ctx, c3, c0);
            child.type = MeshElementType::Quad8;
        }
    }
}

uint32_t MeshSplitAlgorithm::getOrCreateMidpoint(SplitContext& ctx, uint32_t edge_index) const {
    const auto existing = ctx.edgeMidpointNodes.find(edge_index);
    if(existing != ctx.edgeMidpointNodes.end()) {
        return existing->second;
    }

    const auto& [node1_index, node2_index] = ctx.topology.edges[edge_index];
    const auto& position1 = ctx.entry.nodes[node1_index].position;
    const auto& position2 = ctx.entry.nodes[node2_index].position;

    SplitResult::NewNode midpoint{};
    midpoint.x = (static_cast<double>(position1[0]) + static_cast<double>(position2[0])) / 2.0;
    midpoint.y = (static_cast<double>(position1[1]) + static_cast<double>(position2[1])) / 2.0;
    midpoint.z = (static_cast<double>(position1[2]) + static_cast<double>(position2[2])) / 2.0;

    ctx.result.newNodes.push_back(midpoint);
    const uint32_t new_local_id = ctx.nextNodeLocalId++;
    ctx.edgeMidpointNodes[edge_index] = new_local_id;
    return new_local_id;
}

uint32_t MeshSplitAlgorithm::createCentroid(SplitContext& ctx, uint32_t elem_index) const {
    const auto& element = ctx.entry.elements[elem_index];
    const auto element_node_count = cornerCount(element.type);

    double center_x = 0.0;
    double center_y = 0.0;
    double center_z = 0.0;
    for(uint8_t node_offset = 0; node_offset < element_node_count; ++node_offset) {
        const auto& position = ctx.entry.nodes[element.nodeLocalIds[node_offset] - 1U].position;
        center_x += static_cast<double>(position[0]);
        center_y += static_cast<double>(position[1]);
        center_z += static_cast<double>(position[2]);
    }

    center_x /= element_node_count;
    center_y /= element_node_count;
    center_z /= element_node_count;

    ctx.result.newNodes.push_back({center_x, center_y, center_z});
    return ctx.nextNodeLocalId++;
}

uint32_t MeshSplitAlgorithm::findOppositeNode(const MeshElement& element,
                                              uint32_t edge_node1_local_id,
                                              uint32_t edge_node2_local_id) {
    const auto element_corner_count = cornerCount(element.type);
    for(uint8_t node_offset = 0; node_offset < element_corner_count; ++node_offset) {
        const auto node_local_id = element.nodeLocalIds[node_offset];
        if(node_local_id != edge_node1_local_id && node_local_id != edge_node2_local_id) {
            return node_local_id;
        }
    }

    return 0;
}

void MeshSplitAlgorithm::processTriangleEdges(SplitContext& ctx,
                                              uint32_t elem_index,
                                              const std::vector<uint32_t>& edge_indices,
                                              SplitMode mode) const {
    const auto& element = ctx.entry.elements[elem_index];
    const uint32_t node1 = element.nodeLocalIds[0];
    const uint32_t node2 = element.nodeLocalIds[1];
    const uint32_t node3 = element.nodeLocalIds[2];

    SplitResult::ElementReplacement replacement;
    replacement.originalIndex = elem_index;

    if(edge_indices.size() == 1) {
        const auto& [edge_node1_index, edge_node2_index] = ctx.topology.edges[edge_indices[0]];
        const uint32_t edge_node1_local_id = edge_node1_index + 1U;
        const uint32_t edge_node2_local_id = edge_node2_index + 1U;
        const uint32_t midpoint = getOrCreateMidpoint(ctx, edge_indices[0]);
        const uint32_t opposite =
            findOppositeNode(element, edge_node1_local_id, edge_node2_local_id);

        replacement.newElements.push_back(makeTriangle(edge_node1_local_id, midpoint, opposite));
        replacement.newElements.push_back(makeTriangle(midpoint, edge_node2_local_id, opposite));
    } else if(edge_indices.size() == 2) {
        const auto& [edge0_node1_index, edge0_node2_index] = ctx.topology.edges[edge_indices[0]];
        const auto& [edge1_node1_index, edge1_node2_index] = ctx.topology.edges[edge_indices[1]];

        uint32_t shared_node = 0;
        for(const uint32_t candidate : {edge0_node1_index + 1U, edge0_node2_index + 1U}) {
            if(candidate == edge1_node1_index + 1U || candidate == edge1_node2_index + 1U) {
                shared_node = candidate;
                break;
            }
        }
        assert(shared_node != 0U);

        const uint32_t node_a = (edge0_node1_index + 1U == shared_node) ? edge0_node2_index + 1U
                                                                        : edge0_node1_index + 1U;
        const uint32_t node_b = (edge1_node1_index + 1U == shared_node) ? edge1_node2_index + 1U
                                                                        : edge1_node1_index + 1U;

        const uint32_t midpoint0 = getOrCreateMidpoint(ctx, edge_indices[0]);
        const uint32_t midpoint1 = getOrCreateMidpoint(ctx, edge_indices[1]);

        replacement.newElements.push_back(makeTriangle(shared_node, midpoint0, midpoint1));
        replacement.newElements.push_back(makeTriangle(midpoint0, node_a, node_b));
        replacement.newElements.push_back(makeTriangle(midpoint0, node_b, midpoint1));
    } else if(edge_indices.size() == 3) {
        const auto edge12 = ctx.topology.findEdgeIndex(node1, node2);
        const auto edge23 = ctx.topology.findEdgeIndex(node2, node3);
        const auto edge31 = ctx.topology.findEdgeIndex(node3, node1);
        if(!edge12.has_value() || !edge23.has_value() || !edge31.has_value()) {
            throw std::runtime_error("Triangle topology is missing expected edges.");
        }

        const uint32_t midpoint12 = getOrCreateMidpoint(ctx, edge12.value());
        const uint32_t midpoint23 = getOrCreateMidpoint(ctx, edge23.value());
        const uint32_t midpoint31 = getOrCreateMidpoint(ctx, edge31.value());

        // Extract triangle-specific mode bits (TriaFour|QuadThree)
        const auto tri_mode = static_cast<SplitMode>(static_cast<uint8_t>(mode) & 24U);
        if(tri_mode == SplitMode::QuadThree) {
            const uint32_t center = createCentroid(ctx, elem_index);
            replacement.newElements.push_back(makeQuad(node1, midpoint12, center, midpoint31));
            replacement.newElements.push_back(makeQuad(node2, midpoint23, center, midpoint12));
            replacement.newElements.push_back(makeQuad(node3, midpoint31, center, midpoint23));
        } else {
            replacement.newElements.push_back(makeTriangle(node1, midpoint12, midpoint31));
            replacement.newElements.push_back(makeTriangle(midpoint12, node2, midpoint23));
            replacement.newElements.push_back(makeTriangle(midpoint31, midpoint23, node3));
            replacement.newElements.push_back(makeTriangle(midpoint12, midpoint23, midpoint31));
        }
    }

    ctx.result.replacements.push_back(std::move(replacement));
}

void MeshSplitAlgorithm::processNeighborCut(SplitContext& ctx,
                                            uint32_t neighbor_elem_index,
                                            uint32_t shared_edge_index) const {
    if(ctx.processedElements.count(neighbor_elem_index) > 0U) {
        return;
    }
    ctx.processedElements.insert(neighbor_elem_index);

    const auto& element = ctx.entry.elements[neighbor_elem_index];
    const auto& [edge_node1_index, edge_node2_index] = ctx.topology.edges[shared_edge_index];
    const uint32_t edge_node1_local_id = edge_node1_index + 1U;
    const uint32_t edge_node2_local_id = edge_node2_index + 1U;
    const uint32_t midpoint = getOrCreateMidpoint(ctx, shared_edge_index);

    const auto linear_type = linearEquivalent(element.type);
    if(linear_type == MeshElementType::Triangle) {
        const uint32_t opposite =
            findOppositeNode(element, edge_node1_local_id, edge_node2_local_id);

        SplitResult::ElementReplacement replacement;
        replacement.originalIndex = neighbor_elem_index;
        replacement.newElements.push_back(makeTriangle(edge_node1_local_id, midpoint, opposite));
        replacement.newElements.push_back(makeTriangle(midpoint, edge_node2_local_id, opposite));
        ctx.result.replacements.push_back(std::move(replacement));
        if(isSecondOrder(element.type)) {
            upgradeReplacementToSecondOrder(ctx, ctx.result.replacements.back());
        }
        return;
    }

    if(linear_type == MeshElementType::Quad) {
        std::array<uint32_t, 4> corners{};
        for(uint8_t corner_index = 0; corner_index < 4; ++corner_index) {
            corners[corner_index] = element.nodeLocalIds[corner_index];
        }

        uint8_t edge_node1_position = 0;
        for(uint8_t corner_index = 0; corner_index < 4; ++corner_index) {
            if(corners[corner_index] == edge_node1_local_id) {
                edge_node1_position = corner_index;
                break;
            }
        }

        const uint32_t next_corner = corners[(edge_node1_position + 1U) % 4U];
        const uint32_t previous_corner = corners[(edge_node1_position + 3U) % 4U];

        SplitResult::ElementReplacement replacement;
        replacement.originalIndex = neighbor_elem_index;
        if(next_corner == edge_node2_local_id) {
            const uint32_t opposite_corner = corners[(edge_node1_position + 2U) % 4U];
            replacement.newElements.push_back(
                makeTriangle(edge_node1_local_id, midpoint, previous_corner));
            replacement.newElements.push_back(
                makeQuad(midpoint, edge_node2_local_id, opposite_corner, previous_corner));
        } else {
            const uint32_t opposite_corner = corners[(edge_node1_position + 2U) % 4U];
            replacement.newElements.push_back(
                makeTriangle(edge_node2_local_id, midpoint, opposite_corner));
            replacement.newElements.push_back(
                makeQuad(midpoint, edge_node1_local_id, next_corner, opposite_corner));
        }
        ctx.result.replacements.push_back(std::move(replacement));
        if(isSecondOrder(element.type)) {
            upgradeReplacementToSecondOrder(ctx, ctx.result.replacements.back());
        }
    }
}

void MeshSplitAlgorithm::processQuadEdges(SplitContext& ctx,
                                          uint32_t elem_index,
                                          const std::vector<uint32_t>& edge_indices,
                                          SplitMode mode) const {
    const auto& element = ctx.entry.elements[elem_index];
    const uint32_t n1 = element.nodeLocalIds[0];
    const uint32_t n2 = element.nodeLocalIds[1];
    const uint32_t n3 = element.nodeLocalIds[2];
    const uint32_t n4 = element.nodeLocalIds[3];

    SplitResult::ElementReplacement replacement;
    replacement.originalIndex = elem_index;

    std::array<std::optional<uint32_t>, 4> side_edge_idx;
    side_edge_idx[0] = ctx.topology.findEdgeIndex(n1, n2);
    side_edge_idx[1] = ctx.topology.findEdgeIndex(n2, n3);
    side_edge_idx[2] = ctx.topology.findEdgeIndex(n3, n4);
    side_edge_idx[3] = ctx.topology.findEdgeIndex(n4, n1);

    std::array<bool, 4> side_selected{false, false, false, false};
    uint32_t selected_count = 0;
    for(uint8_t side = 0; side < 4; ++side) {
        if(side_edge_idx[side].has_value()) {
            for(const auto edge_index : edge_indices) {
                if(edge_index == side_edge_idx[side].value()) {
                    side_selected[side] = true;
                    ++selected_count;
                    break;
                }
            }
        }
    }

    const std::array<uint32_t, 4> corners = {n1, n2, n3, n4};

    if(selected_count == 1) {
        uint8_t sel_side = 0;
        for(uint8_t side = 0; side < 4; ++side) {
            if(side_selected[side]) {
                sel_side = side;
                break;
            }
        }
        const uint32_t mid = getOrCreateMidpoint(ctx, side_edge_idx[sel_side].value());
        const uint32_t ca = corners[sel_side];
        const uint32_t cb = corners[(sel_side + 1) % 4];
        const uint32_t cc = corners[(sel_side + 2) % 4];
        const uint32_t cd = corners[(sel_side + 3) % 4];

        replacement.newElements.push_back(makeTriangle(ca, mid, cd));
        replacement.newElements.push_back(makeQuad(mid, cb, cc, cd));
    } else if(selected_count == 2) {
        uint8_t s0 = 255;
        uint8_t s1 = 255;
        for(uint8_t side = 0; side < 4; ++side) {
            if(side_selected[side]) {
                if(s0 == 255) {
                    s0 = side;
                } else {
                    s1 = side;
                }
            }
        }
        const bool adjacent = ((s1 - s0) == 1) || ((s0 == 0) && (s1 == 3));

        if(!adjacent) {
            const uint32_t mid0 = getOrCreateMidpoint(ctx, side_edge_idx[s0].value());
            const uint32_t mid1 = getOrCreateMidpoint(ctx, side_edge_idx[s1].value());
            const uint32_t ca = corners[s0];
            const uint32_t cb = corners[(s0 + 1) % 4];
            const uint32_t cc = corners[(s0 + 2) % 4];
            const uint32_t cd = corners[(s0 + 3) % 4];

            replacement.newElements.push_back(makeQuad(ca, mid0, mid1, cd));
            replacement.newElements.push_back(makeQuad(mid0, cb, cc, mid1));
        } else {
            if((s0 == 0) && (s1 == 3)) {
                std::swap(s0, s1);
            }
            const uint32_t shared = corners[(s0 + 1) % 4];
            const uint32_t mid0 = getOrCreateMidpoint(ctx, side_edge_idx[s0].value());
            const uint32_t mid1 = getOrCreateMidpoint(ctx, side_edge_idx[s1].value());

            const uint32_t ca = corners[s0];
            const uint32_t cc = corners[(s0 + 2) % 4];
            const uint32_t cd = corners[(s0 + 3) % 4];

            replacement.newElements.push_back(makeTriangle(mid0, shared, mid1));
            replacement.newElements.push_back(makeTriangle(ca, mid0, cd));
            replacement.newElements.push_back(makeQuad(mid0, mid1, cc, cd));
        }
    } else if(selected_count == 3) {
        uint8_t unsel = 0;
        for(uint8_t side = 0; side < 4; ++side) {
            if(!side_selected[side]) {
                unsel = side;
                break;
            }
        }

        const uint32_t ca = corners[(unsel + 1) % 4];
        const uint32_t cb = corners[(unsel + 2) % 4];
        const uint32_t cc = corners[(unsel + 3) % 4];
        const uint32_t cd = corners[unsel];

        const uint32_t mid_ab = getOrCreateMidpoint(ctx, side_edge_idx[(unsel + 1) % 4].value());
        const uint32_t mid_bc = getOrCreateMidpoint(ctx, side_edge_idx[(unsel + 2) % 4].value());
        const uint32_t mid_cd = getOrCreateMidpoint(ctx, side_edge_idx[(unsel + 3) % 4].value());

        // Corner/midpoint mapping to reference diagram (ABCD quad, CD unselected):
        //   D = ca,  A = cb,  B = cc,  C = cd
        //   Ml = mid_ab (D-A), Mt = mid_bc (A-B), Mr = mid_cd (B-C)
        // Extract quad-specific mode bits (TriaOneQuadThree|TriaOneQuadTwo|TriaThreeQuadTwo)
        const auto quad_mode = static_cast<SplitMode>(static_cast<uint8_t>(mode) & 7U);

        if(quad_mode == SplitMode::TriaOneQuadThree) {
            // 3Q+1T: center P connects to Mt, Mr, Ml, D
            const uint32_t center = createCentroid(ctx, elem_index);
            replacement.newElements.push_back(makeQuad(mid_ab, cb, mid_bc, center));
            replacement.newElements.push_back(makeQuad(mid_bc, cc, mid_cd, center));
            replacement.newElements.push_back(makeQuad(mid_cd, cd, ca, center));
            replacement.newElements.push_back(makeTriangle(ca, mid_ab, center));
        } else if(quad_mode == SplitMode::TriaOneQuadTwo) {
            // 2Q+1T: no center, Mt→Mr and Ml→Mr cuts
            replacement.newElements.push_back(makeQuad(mid_ab, cb, mid_bc, mid_cd));
            replacement.newElements.push_back(makeTriangle(mid_bc, cc, mid_cd));
            replacement.newElements.push_back(makeQuad(mid_ab, mid_cd, cd, ca));
        } else {
            // Default: TriaThreeQuadTwo (2Q+3T)
            // center P connects to Mt, Mr, Ml, C, D
            const uint32_t center = createCentroid(ctx, elem_index);
            replacement.newElements.push_back(makeQuad(mid_ab, cb, mid_bc, center));
            replacement.newElements.push_back(makeQuad(mid_bc, cc, mid_cd, center));
            replacement.newElements.push_back(makeTriangle(mid_cd, cd, center));
            replacement.newElements.push_back(makeTriangle(cd, ca, center));
            replacement.newElements.push_back(makeTriangle(ca, mid_ab, center));
        }
    } else if(selected_count == 4) {
        const uint32_t mid01 = getOrCreateMidpoint(ctx, side_edge_idx[0].value());
        const uint32_t mid12 = getOrCreateMidpoint(ctx, side_edge_idx[1].value());
        const uint32_t mid23 = getOrCreateMidpoint(ctx, side_edge_idx[2].value());
        const uint32_t mid30 = getOrCreateMidpoint(ctx, side_edge_idx[3].value());
        const uint32_t center = createCentroid(ctx, elem_index);

        replacement.newElements.push_back(makeQuad(n1, mid01, center, mid30));
        replacement.newElements.push_back(makeQuad(mid01, n2, mid12, center));
        replacement.newElements.push_back(makeQuad(center, mid12, n3, mid23));
        replacement.newElements.push_back(makeQuad(mid30, center, mid23, n4));
    }

    ctx.result.replacements.push_back(std::move(replacement));
}

void MeshSplitAlgorithm::processTriangleNodes(SplitContext& ctx, uint32_t elem_index) const {
    const auto& element = ctx.entry.elements[elem_index];
    const uint32_t node1 = element.nodeLocalIds[0];
    const uint32_t node2 = element.nodeLocalIds[1];
    const uint32_t node3 = element.nodeLocalIds[2];

    const uint32_t center = createCentroid(ctx, elem_index);

    SplitResult::ElementReplacement replacement;
    replacement.originalIndex = elem_index;
    replacement.newElements.push_back(makeTriangle(node1, node2, center));
    replacement.newElements.push_back(makeTriangle(node2, node3, center));
    replacement.newElements.push_back(makeTriangle(node3, node1, center));

    ctx.result.replacements.push_back(std::move(replacement));
}

SplitResult MeshSplitAlgorithm::compute(const MeshEntry& entry,
                                        const MeshTopology& topology,
                                        const std::vector<uint32_t>& selected_edge_local_ids,
                                        const std::vector<uint32_t>& selected_node_local_ids,
                                        SplitMode mode) const {
    SplitResult result;
    if(selected_edge_local_ids.empty() && selected_node_local_ids.empty()) {
        return result;
    }

    SplitContext ctx{
        entry,  topology,
        result, {},
        {},     static_cast<uint32_t>(entry.nodes.size()) + 1U,
        {},     static_cast<uint32_t>(entry.nodes.size()),
    };

    seedMidEdgeNodes(ctx);

    std::unordered_map<uint32_t, std::vector<uint32_t>> element_to_edges;
    for(const auto edge_local_id : selected_edge_local_ids) {
        if(edge_local_id == 0U || edge_local_id > topology.edges.size()) {
            LOG_WARN("SKIPPING edge_local_id={} (out of range, max={})", edge_local_id,
                     topology.edges.size());
            continue;
        }

        const auto edge_index = edge_local_id - 1U;
        for(const auto element_index : topology.edgeToElements[edge_index]) {
            element_to_edges[element_index].push_back(edge_index);
        }
    }

    std::unordered_map<uint32_t, std::vector<uint32_t>> element_to_nodes;
    for(const auto node_local_id : selected_node_local_ids) {
        if(node_local_id == 0U || node_local_id > entry.nodes.size()) {
            continue;
        }

        for(const auto element_index : topology.nodeToElements[node_local_id]) {
            element_to_nodes[element_index].push_back(node_local_id);
        }
    }

    // Sort elements by selected edge count (descending) so elements with more
    // selected edges are processed first. This prevents a neighbor cut from
    // "stealing" the target element before it gets its full multi-edge split.
    std::vector<std::pair<uint32_t, std::vector<uint32_t>>> sorted_elements(
        element_to_edges.begin(), element_to_edges.end());
    std::sort(sorted_elements.begin(), sorted_elements.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.second.size() > rhs.second.size();
    });

    // Pass 1: split all directly selected elements
    for(const auto& [element_index, edge_indices] : sorted_elements) {
        if(ctx.processedElements.count(element_index) > 0U) {
            continue;
        }
        ctx.processedElements.insert(element_index);

        const auto& element = entry.elements[element_index];
        const auto linear_type = linearEquivalent(element.type);
        if(linear_type == MeshElementType::Triangle) {
            processTriangleEdges(ctx, element_index, edge_indices, mode);
            if(isSecondOrder(element.type)) {
                upgradeReplacementToSecondOrder(ctx, ctx.result.replacements.back());
            }
        } else if(linear_type == MeshElementType::Quad) {
            processQuadEdges(ctx, element_index, edge_indices, mode);
            if(isSecondOrder(element.type)) {
                upgradeReplacementToSecondOrder(ctx, ctx.result.replacements.back());
            }
        }
    }

    // Pass 2: propagate neighbor cuts along shared edges
    for(const auto& [element_index, edge_indices] : sorted_elements) {
        for(const auto edge_index : edge_indices) {
            for(const auto neighbor_index : topology.edgeToElements[edge_index]) {
                if(neighbor_index != element_index) {
                    processNeighborCut(ctx, neighbor_index, edge_index);
                }
            }
        }
    }

    for(const auto& [element_index, node_ids] : element_to_nodes) {
        if(ctx.processedElements.count(element_index) > 0U) {
            continue;
        }

        const auto& element = entry.elements[element_index];
        const auto linear_type = linearEquivalent(element.type);
        if(linear_type == MeshElementType::Triangle && node_ids.size() == 3U &&
           (static_cast<uint8_t>(mode) & static_cast<uint8_t>(SplitMode::TriaThree)) != 0U) {
            ctx.processedElements.insert(element_index);
            processTriangleNodes(ctx, element_index);
            if(isSecondOrder(element.type)) {
                upgradeReplacementToSecondOrder(ctx, ctx.result.replacements.back());
            }
        }
    }

    return result;
}

} // namespace OpenGeoLab::Mesh
