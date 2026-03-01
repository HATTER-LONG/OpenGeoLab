/**
 * @file mesh_render_builder.cpp
 * @brief MeshRenderBuilder — converts FEM mesh nodes and elements into
 *        RenderData for the Mesh render pass.
 *
 * The vertex buffer is laid out in three contiguous phases:
 *   [0, surfaceCount)        — surface triangles  (GL_TRIANGLES)
 *   [surfaceCount, +wireCount) — wireframe edges   (GL_LINES)
 *   [wireStart, +nodeCount)   — node points        (GL_POINTS)
 * MeshPass relies on this layout to issue separate draw calls per topology.
 */

#include "mesh_render_builder.hpp"

#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <cmath>
#include <unordered_map>

namespace OpenGeoLab::Render {

namespace {

struct Vec3f {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
};

Vec3f toVec3f(const Util::Pt3d& p) {
    return {static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z)};
}

/// Generate a composite key from a sorted pair of node IDs for edge deduplication.
/// Used only as hash-map key; the actual PickId UID is a sequential integer.
uint64_t makeEdgeKey(Mesh::MeshNodeId a, Mesh::MeshNodeId b) {
    const auto lo = std::min(a, b);
    const auto hi = std::max(a, b);
    return (lo << 32u) | (hi & 0xFFFFFFFFu);
}

Vec3f computeTriangleNormal(const Vec3f& a, const Vec3f& b, const Vec3f& c) {
    const float ux = b.x - a.x;
    const float uy = b.y - a.y;
    const float uz = b.z - a.z;
    const float vx = c.x - a.x;
    const float vy = c.y - a.y;
    const float vz = c.z - a.z;
    float nx = uy * vz - uz * vy;
    float ny = uz * vx - ux * vz;
    float nz = ux * vy - uy * vx;
    const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if(len > 1e-8f) {
        nx /= len;
        ny /= len;
        nz /= len;
    }
    return {nx, ny, nz};
}

void pushVertex(RenderPassData& pass,
                const Vec3f& pos,
                const Vec3f& normal,
                const RenderColor& color,
                uint64_t pick_id) {
    RenderVertex v{};
    v.m_position[0] = pos.x;
    v.m_position[1] = pos.y;
    v.m_position[2] = pos.z;
    v.m_normal[0] = normal.x;
    v.m_normal[1] = normal.y;
    v.m_normal[2] = normal.z;
    v.m_color[0] = color.m_r;
    v.m_color[1] = color.m_g;
    v.m_color[2] = color.m_b;
    v.m_color[3] = color.m_a;
    v.m_pickId = pick_id;
    pass.m_vertices.push_back(v);
}

void pushTriangle(RenderPassData& pass,
                  const Vec3f& a,
                  const Vec3f& b,
                  const Vec3f& c,
                  const RenderColor& color,
                  uint64_t pick_id) {
    const Vec3f n = computeTriangleNormal(a, b, c);
    pushVertex(pass, a, n, color, pick_id);
    pushVertex(pass, b, n, color, pick_id);
    pushVertex(pass, c, n, color, pick_id);
}

void pushLine(RenderPassData& pass,
              const Vec3f& a,
              const Vec3f& b,
              const RenderColor& color,
              uint64_t pick_id) {
    const Vec3f zero{0.0f, 0.0f, 0.0f};
    pushVertex(pass, a, zero, color, pick_id);
    pushVertex(pass, b, zero, color, pick_id);
}

void pushEdge(RenderPassData& pass,
              const Vec3f& a,
              const Vec3f& b,
              const RenderColor& color,
              uint64_t pick_id) {
    pushLine(pass, a, b, color, pick_id);
}

// 3D element face table definitions
constexpr int TETRA4_FACES[][3] = {{0, 1, 2}, {0, 3, 1}, {1, 3, 2}, {0, 2, 3}};

constexpr int HEXA8_FACES[][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                                  {2, 3, 7, 6}, {0, 4, 7, 3}, {1, 2, 6, 5}};

constexpr int PRISM6_TRI_FACES[][3] = {{0, 1, 2}, {3, 5, 4}};
constexpr int PRISM6_QUAD_FACES[][4] = {{0, 3, 4, 1}, {1, 4, 5, 2}, {0, 2, 5, 3}};

constexpr int PYRAMID5_BASE[] = {0, 3, 2, 1};
constexpr int PYRAMID5_TRI_FACES[][3] = {{0, 1, 4}, {1, 2, 4}, {2, 3, 4}, {0, 4, 3}};

// Edge tables for wireframe rendering
constexpr int TRIANGLE_EDGES[][2] = {{0, 1}, {1, 2}, {2, 0}};
constexpr int QUAD4_EDGES[][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};
constexpr int TETRA4_EDGES[][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};
constexpr int HEXA8_EDGES[][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                                  {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
constexpr int PRISM6_EDGES[][2] = {{0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 5},
                                   {5, 3}, {0, 3}, {1, 4}, {2, 5}};
constexpr int PYRAMID5_EDGES[][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0},
                                     {0, 4}, {1, 4}, {2, 4}, {3, 4}};

} // namespace

bool MeshRenderBuilder::build(RenderData& render_data, const MeshRenderInput& input) {
    render_data.clearMesh();

    if(input.m_nodes.empty() || input.m_elements.empty()) {
        return true;
    }

    const auto& color_map = Util::ColorMap::instance();

    auto& mesh_pass = render_data.m_passData[RenderPassType::Mesh];

    auto nodePos = [&input](Mesh::MeshNodeId nid) -> Vec3f {
        if(nid == Mesh::INVALID_MESH_NODE_ID || nid > input.m_nodes.size()) {
            return {};
        }
        return toVec3f(input.m_nodes[static_cast<size_t>(nid - 1)].position());
    };

    // ---------- Phase 1: Surface triangles ----------
    // All triangle data comes first in the buffer so MeshPass can draw GL_TRIANGLES
    // over the range [0, surfaceVertexCount).
    const RenderColor surface_color = input.m_surfaceColor;

    for(const auto& elem : input.m_elements) {
        if(!elem.isValid()) {
            continue;
        }

        const RenderEntityType render_type = toRenderEntityType(elem.elementType());
        const uint64_t pick_id = PickId::encode(render_type, elem.elementUID());

        switch(elem.elementType()) {
        case Mesh::MeshElementType::Triangle: {
            const Vec3f p0 = nodePos(elem.nodeId(0));
            const Vec3f p1 = nodePos(elem.nodeId(1));
            const Vec3f p2 = nodePos(elem.nodeId(2));
            pushTriangle(mesh_pass, p0, p1, p2, surface_color, pick_id);
            break;
        }
        case Mesh::MeshElementType::Quad4: {
            const Vec3f p0 = nodePos(elem.nodeId(0));
            const Vec3f p1 = nodePos(elem.nodeId(1));
            const Vec3f p2 = nodePos(elem.nodeId(2));
            const Vec3f p3 = nodePos(elem.nodeId(3));
            pushTriangle(mesh_pass, p0, p1, p2, surface_color, pick_id);
            pushTriangle(mesh_pass, p0, p2, p3, surface_color, pick_id);
            break;
        }
        case Mesh::MeshElementType::Tetra4: {
            for(const auto& face : TETRA4_FACES) {
                const Vec3f p0 = nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = nodePos(elem.nodeId(face[2]));
                pushTriangle(mesh_pass, p0, p1, p2, surface_color, pick_id);
            }
            break;
        }
        case Mesh::MeshElementType::Hexa8: {
            for(const auto& face : HEXA8_FACES) {
                const Vec3f p0 = nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = nodePos(elem.nodeId(face[2]));
                const Vec3f p3 = nodePos(elem.nodeId(face[3]));
                pushTriangle(mesh_pass, p0, p1, p2, surface_color, pick_id);
                pushTriangle(mesh_pass, p0, p2, p3, surface_color, pick_id);
            }
            break;
        }
        case Mesh::MeshElementType::Prism6: {
            for(const auto& face : PRISM6_TRI_FACES) {
                const Vec3f p0 = nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = nodePos(elem.nodeId(face[2]));
                pushTriangle(mesh_pass, p0, p1, p2, surface_color, pick_id);
            }
            for(const auto& face : PRISM6_QUAD_FACES) {
                const Vec3f p0 = nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = nodePos(elem.nodeId(face[2]));
                const Vec3f p3 = nodePos(elem.nodeId(face[3]));
                pushTriangle(mesh_pass, p0, p1, p2, surface_color, pick_id);
                pushTriangle(mesh_pass, p0, p2, p3, surface_color, pick_id);
            }
            break;
        }
        case Mesh::MeshElementType::Pyramid5: {
            const Vec3f b0 = nodePos(elem.nodeId(PYRAMID5_BASE[0]));
            const Vec3f b1 = nodePos(elem.nodeId(PYRAMID5_BASE[1]));
            const Vec3f b2 = nodePos(elem.nodeId(PYRAMID5_BASE[2]));
            const Vec3f b3 = nodePos(elem.nodeId(PYRAMID5_BASE[3]));
            pushTriangle(mesh_pass, b0, b1, b2, surface_color, pick_id);
            pushTriangle(mesh_pass, b0, b2, b3, surface_color, pick_id);
            for(const auto& face : PYRAMID5_TRI_FACES) {
                const Vec3f p0 = nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = nodePos(elem.nodeId(face[2]));
                pushTriangle(mesh_pass, p0, p1, p2, surface_color, pick_id);
            }
            break;
        }
        default:
            break;
        }
    }

    const uint32_t surfaceVertexCount = static_cast<uint32_t>(mesh_pass.m_vertices.size());

    // ---------- Phase 2: Wireframe edges ----------
    // Element outline edges appended after surface triangles.
    // MeshPass draws these as GL_LINES over [surfaceVertexCount, surfaceVertexCount +
    // wireframeCount).
    // Each unique edge gets a small sequential ID (1, 2, 3…) for PickId encoding.
    // Shared edges between elements produce the same ID via the edgeKey dedup map.
    const RenderColor wire_color = color_map.getMeshLineColor();

    // Map composite edge key → sequential ID for deduplication and small IDs.
    std::unordered_map<uint64_t, uint64_t> edgeKeyToSeqId;
    uint64_t nextEdgeId = 1;

    auto getOrCreateEdgeId = [&](Mesh::MeshNodeId n0, Mesh::MeshNodeId n1) -> uint64_t {
        const uint64_t key = makeEdgeKey(n0, n1);
        auto it = edgeKeyToSeqId.find(key);
        if(it != edgeKeyToSeqId.end()) {
            return it->second;
        }
        const uint64_t id = nextEdgeId++;
        edgeKeyToSeqId[key] = id;
        render_data.m_pickData.m_meshLineNodes[id] = {std::min(n0, n1), std::max(n0, n1)};
        return id;
    };

    auto emitEdge = [&](Mesh::MeshNodeId n0, Mesh::MeshNodeId n1) {
        const uint64_t edge_id = getOrCreateEdgeId(n0, n1);
        const uint64_t line_pick_id = PickId::encode(RenderEntityType::MeshLine, edge_id);
        pushEdge(mesh_pass, nodePos(n0), nodePos(n1), wire_color, line_pick_id);
    };

    for(const auto& elem : input.m_elements) {
        if(!elem.isValid()) {
            continue;
        }

        switch(elem.elementType()) {
        case Mesh::MeshElementType::Triangle:
            for(const auto& edge : TRIANGLE_EDGES) {
                emitEdge(elem.nodeId(edge[0]), elem.nodeId(edge[1]));
            }
            break;
        case Mesh::MeshElementType::Quad4:
            for(const auto& edge : QUAD4_EDGES) {
                emitEdge(elem.nodeId(edge[0]), elem.nodeId(edge[1]));
            }
            break;
        case Mesh::MeshElementType::Line:
            emitEdge(elem.nodeId(0), elem.nodeId(1));
            break;
        case Mesh::MeshElementType::Tetra4:
            for(const auto& edge : TETRA4_EDGES) {
                emitEdge(elem.nodeId(edge[0]), elem.nodeId(edge[1]));
            }
            break;
        case Mesh::MeshElementType::Hexa8:
            for(const auto& edge : HEXA8_EDGES) {
                emitEdge(elem.nodeId(edge[0]), elem.nodeId(edge[1]));
            }
            break;
        case Mesh::MeshElementType::Prism6:
            for(const auto& edge : PRISM6_EDGES) {
                emitEdge(elem.nodeId(edge[0]), elem.nodeId(edge[1]));
            }
            break;
        case Mesh::MeshElementType::Pyramid5:
            for(const auto& edge : PYRAMID5_EDGES) {
                emitEdge(elem.nodeId(edge[0]), elem.nodeId(edge[1]));
            }
            break;
        default:
            break;
        }
    }

    const uint32_t wireframeVertexCount =
        static_cast<uint32_t>(mesh_pass.m_vertices.size()) - surfaceVertexCount;

    // ---------- Phase 3: Mesh nodes as points ----------
    // Appended after wireframe data.
    const RenderColor node_color = color_map.getMeshNodeColor();
    const Vec3f zero_normal{0.0f, 0.0f, 0.0f};
    for(const auto& node : input.m_nodes) {
        if(node.nodeId() == Mesh::INVALID_MESH_NODE_ID) {
            continue;
        }
        const uint64_t node_pick_id = PickId::encode(RenderEntityType::MeshNode, node.nodeId());
        pushVertex(mesh_pass, toVec3f(node.position()), zero_normal, node_color, node_pick_id);
    }

    const uint32_t nodeVertexCount = static_cast<uint32_t>(mesh_pass.m_vertices.size()) -
                                     surfaceVertexCount - wireframeVertexCount;

    // ---------- Build mesh root node ----------
    if(!mesh_pass.m_vertices.empty()) {
        RenderNode mesh_root;
        mesh_root.m_key = {RenderEntityType::MeshTriangle, 0};
        mesh_root.m_visible = true;

        // Surface draw range
        if(surfaceVertexCount > 0) {
            DrawRange surface_range;
            surface_range.m_vertexOffset = 0;
            surface_range.m_vertexCount = surfaceVertexCount;
            surface_range.m_topology = PrimitiveTopology::Triangles;
            mesh_root.m_drawRanges[RenderPassType::Mesh].push_back(surface_range);
        }

        // Wireframe draw range
        if(wireframeVertexCount > 0) {
            DrawRange wire_range;
            wire_range.m_vertexOffset = surfaceVertexCount;
            wire_range.m_vertexCount = wireframeVertexCount;
            wire_range.m_topology = PrimitiveTopology::Lines;
            mesh_root.m_drawRanges[RenderPassType::Mesh].push_back(wire_range);
        }

        // Node points draw range
        if(nodeVertexCount > 0) {
            DrawRange node_range;
            node_range.m_vertexOffset = surfaceVertexCount + wireframeVertexCount;
            node_range.m_vertexCount = nodeVertexCount;
            node_range.m_topology = PrimitiveTopology::Points;
            mesh_root.m_drawRanges[RenderPassType::Mesh].push_back(node_range);
        }

        for(const auto& node : input.m_nodes) {
            if(node.nodeId() != Mesh::INVALID_MESH_NODE_ID) {
                mesh_root.m_bbox.expand(node.position());
            }
        }

        render_data.m_sceneBBox.expand(mesh_root.m_bbox);
        render_data.m_roots.push_back(std::move(mesh_root));
    }

    mesh_pass.markDataUpdated();

    LOG_DEBUG("MeshRenderBuilder::build: surface={}, wireframe={}, nodes={}, elements={}",
              surfaceVertexCount, wireframeVertexCount, nodeVertexCount, input.m_elements.size());
    return true;
}

} // namespace OpenGeoLab::Render
