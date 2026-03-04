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

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

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

    auto normalize_part_uid = [](Geometry::EntityUID uid) -> Geometry::EntityUID {
        return uid == Geometry::INVALID_ENTITY_UID ? 0 : uid;
    };

    std::unordered_map<Geometry::EntityUID, std::vector<const Mesh::MeshElement*>> part_elements;
    std::unordered_map<Geometry::EntityUID, std::unordered_set<Mesh::MeshNodeId>> part_nodes;
    std::vector<Geometry::EntityUID> part_order;

    for(const auto& elem : input.m_elements) {
        if(!elem.isValid()) {
            continue;
        }
        const auto part_uid = normalize_part_uid(elem.sourcePartUid());
        auto [it, inserted] = part_elements.try_emplace(part_uid);
        if(inserted) {
            part_order.push_back(part_uid);
        }
        it->second.push_back(&elem);

        auto& node_set = part_nodes[part_uid];
        for(uint8_t i = 0; i < elem.nodeCount(); ++i) {
            node_set.insert(elem.nodeId(i));
        }

        const auto render_type = toRenderEntityType(elem.elementType());
        render_data.m_pickData.m_entityToPartUid[PickId::encode(render_type, elem.elementUID())] =
            part_uid;
    }

    auto resolve_surface_color = [&](Geometry::EntityUID part_uid) -> RenderColor {
        const RenderColor fallback =
            part_uid == 0 ? input.m_defaultSurfaceColor : color_map.getColorForPartId(part_uid);
        auto it = input.m_partSurfaceColors.find(part_uid);
        const RenderColor base = (it != input.m_partSurfaceColors.end()) ? it->second : fallback;
        return Util::ColorMap::darkenColor(base, input.m_partDarkenFactor);
    };

    // ---------- Phase 1: Surface triangles (per-part + per-element-type batches) ----------
    for(const auto part_uid : part_order) {
        const auto eit = part_elements.find(part_uid);
        if(eit == part_elements.end()) {
            continue;
        }

        const RenderColor surface_color = resolve_surface_color(part_uid);
        RenderEntityType current_type = RenderEntityType::None;
        uint32_t current_batch_start = 0;

        auto flush_surface_batch = [&]() {
            if(current_type == RenderEntityType::None) {
                return;
            }
            const uint32_t vertex_end = static_cast<uint32_t>(mesh_pass.m_vertices.size());
            if(vertex_end <= current_batch_start) {
                return;
            }

            DrawRange range;
            range.m_entityKey = {current_type, part_uid};
            range.m_partUid = part_uid;
            range.m_vertexOffset = current_batch_start;
            range.m_vertexCount = vertex_end - current_batch_start;
            range.m_topology = PrimitiveTopology::Triangles;

            DrawRangeEx range_ex;
            range_ex.m_range = range;
            range_ex.m_entityKey = range.m_entityKey;
            range_ex.m_partUid = part_uid;
            render_data.m_meshTriangleRanges.push_back(range_ex);
        };

        for(const auto* elem_ptr : eit->second) {
            const auto& elem = *elem_ptr;
            const RenderEntityType render_type = toRenderEntityType(elem.elementType());
            const uint64_t pick_id = PickId::encode(render_type, elem.elementUID());

            if(current_type != render_type) {
                flush_surface_batch();
                current_type = render_type;
                current_batch_start = static_cast<uint32_t>(mesh_pass.m_vertices.size());
            }

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

        flush_surface_batch();
    }

    const uint32_t surfaceVertexCount = static_cast<uint32_t>(mesh_pass.m_vertices.size());

    // ---------- Phase 2: Wireframe edges (per-part batches) ----------
    const RenderColor wire_color = color_map.getMeshLineColor();

    struct PartEdgeKey {
        Geometry::EntityUID m_partUid{0};
        Mesh::MeshNodeId m_nodeLo{0};
        Mesh::MeshNodeId m_nodeHi{0};

        bool operator==(const PartEdgeKey& other) const {
            return m_partUid == other.m_partUid && m_nodeLo == other.m_nodeLo &&
                   m_nodeHi == other.m_nodeHi;
        }
    };

    struct PartEdgeKeyHash {
        size_t operator()(const PartEdgeKey& k) const noexcept {
            size_t seed = std::hash<uint64_t>{}(k.m_partUid);
            seed ^= std::hash<uint64_t>{}(k.m_nodeLo) + 0x9e3779b97f4a7c15ULL + (seed << 6) +
                    (seed >> 2);
            seed ^= std::hash<uint64_t>{}(k.m_nodeHi) + 0x9e3779b97f4a7c15ULL + (seed << 6) +
                    (seed >> 2);
            return seed;
        }
    };

    std::unordered_map<PartEdgeKey, uint64_t, PartEdgeKeyHash> edge_key_to_seq_id;
    uint64_t next_edge_id = 1;

    auto emit_edge = [&](Geometry::EntityUID part_uid, Mesh::MeshNodeId n0, Mesh::MeshNodeId n1) {
        const auto lo = std::min(n0, n1);
        const auto hi = std::max(n0, n1);
        const PartEdgeKey key{part_uid, lo, hi};

        auto it = edge_key_to_seq_id.find(key);
        uint64_t edge_id = 0;
        if(it != edge_key_to_seq_id.end()) {
            edge_id = it->second;
        } else {
            edge_id = next_edge_id++;
            edge_key_to_seq_id.emplace(key, edge_id);
            render_data.m_pickData.m_meshLineNodes[edge_id] = {lo, hi};
            render_data.m_pickData
                .m_entityToPartUid[PickId::encode(RenderEntityType::MeshLine, edge_id)] = part_uid;
        }

        const uint64_t line_pick_id = PickId::encode(RenderEntityType::MeshLine, edge_id);
        pushEdge(mesh_pass, nodePos(n0), nodePos(n1), wire_color, line_pick_id);
    };

    for(const auto part_uid : part_order) {
        const auto eit = part_elements.find(part_uid);
        if(eit == part_elements.end()) {
            continue;
        }

        const uint32_t part_wire_start = static_cast<uint32_t>(mesh_pass.m_vertices.size());

        for(const auto* elem_ptr : eit->second) {
            const auto& elem = *elem_ptr;
            switch(elem.elementType()) {
            case Mesh::MeshElementType::Triangle:
                for(const auto& edge : TRIANGLE_EDGES) {
                    emit_edge(part_uid, elem.nodeId(edge[0]), elem.nodeId(edge[1]));
                }
                break;
            case Mesh::MeshElementType::Quad4:
                for(const auto& edge : QUAD4_EDGES) {
                    emit_edge(part_uid, elem.nodeId(edge[0]), elem.nodeId(edge[1]));
                }
                break;
            case Mesh::MeshElementType::Line:
                emit_edge(part_uid, elem.nodeId(0), elem.nodeId(1));
                break;
            case Mesh::MeshElementType::Tetra4:
                for(const auto& edge : TETRA4_EDGES) {
                    emit_edge(part_uid, elem.nodeId(edge[0]), elem.nodeId(edge[1]));
                }
                break;
            case Mesh::MeshElementType::Hexa8:
                for(const auto& edge : HEXA8_EDGES) {
                    emit_edge(part_uid, elem.nodeId(edge[0]), elem.nodeId(edge[1]));
                }
                break;
            case Mesh::MeshElementType::Prism6:
                for(const auto& edge : PRISM6_EDGES) {
                    emit_edge(part_uid, elem.nodeId(edge[0]), elem.nodeId(edge[1]));
                }
                break;
            case Mesh::MeshElementType::Pyramid5:
                for(const auto& edge : PYRAMID5_EDGES) {
                    emit_edge(part_uid, elem.nodeId(edge[0]), elem.nodeId(edge[1]));
                }
                break;
            default:
                break;
            }
        }

        const uint32_t part_wire_end = static_cast<uint32_t>(mesh_pass.m_vertices.size());
        if(part_wire_end > part_wire_start) {
            DrawRange range;
            range.m_entityKey = {RenderEntityType::MeshLine, part_uid};
            range.m_partUid = part_uid;
            range.m_vertexOffset = part_wire_start;
            range.m_vertexCount = part_wire_end - part_wire_start;
            range.m_topology = PrimitiveTopology::Lines;

            DrawRangeEx range_ex;
            range_ex.m_range = range;
            range_ex.m_entityKey = range.m_entityKey;
            range_ex.m_partUid = part_uid;
            render_data.m_meshLineRanges.push_back(range_ex);
        }
    }

    const uint32_t wireframeVertexCount =
        static_cast<uint32_t>(mesh_pass.m_vertices.size()) - surfaceVertexCount;

    // ---------- Phase 3: Mesh nodes as points (per-part batches) ----------
    const RenderColor node_color = color_map.getMeshNodeColor();
    const Vec3f zero_normal{0.0f, 0.0f, 0.0f};

    for(const auto part_uid : part_order) {
        const auto nit = part_nodes.find(part_uid);
        if(nit == part_nodes.end() || nit->second.empty()) {
            continue;
        }

        std::vector<Mesh::MeshNodeId> ordered_nodes(nit->second.begin(), nit->second.end());
        std::sort(ordered_nodes.begin(), ordered_nodes.end());

        const uint32_t part_node_start = static_cast<uint32_t>(mesh_pass.m_vertices.size());
        for(const auto node_id : ordered_nodes) {
            if(node_id == Mesh::INVALID_MESH_NODE_ID) {
                continue;
            }
            const uint64_t node_pick_id = PickId::encode(RenderEntityType::MeshNode, node_id);
            pushVertex(mesh_pass, nodePos(node_id), zero_normal, node_color, node_pick_id);

            const uint64_t part_key = PickId::encode(RenderEntityType::MeshNode, node_id);
            render_data.m_pickData.m_entityToPartUid.try_emplace(part_key, part_uid);
        }

        const uint32_t part_node_end = static_cast<uint32_t>(mesh_pass.m_vertices.size());
        if(part_node_end > part_node_start) {
            DrawRange range;
            range.m_entityKey = {RenderEntityType::MeshNode, part_uid};
            range.m_partUid = part_uid;
            range.m_vertexOffset = part_node_start;
            range.m_vertexCount = part_node_end - part_node_start;
            range.m_topology = PrimitiveTopology::Points;

            DrawRangeEx range_ex;
            range_ex.m_range = range;
            range_ex.m_entityKey = range.m_entityKey;
            range_ex.m_partUid = part_uid;
            render_data.m_meshPointRanges.push_back(range_ex);
        }
    }

    const uint32_t nodeVertexCount = static_cast<uint32_t>(mesh_pass.m_vertices.size()) -
                                     surfaceVertexCount - wireframeVertexCount;

    // ---------- Build mesh root node ----------
    if(!mesh_pass.m_vertices.empty()) {
        RenderNode mesh_root;
        mesh_root.m_key = {RenderEntityType::MeshTriangle, 0};
        mesh_root.m_visible = true;

        for(const auto& range_ex : render_data.m_meshTriangleRanges) {
            mesh_root.m_drawRanges.push_back(range_ex.m_range);
        }
        for(const auto& range_ex : render_data.m_meshLineRanges) {
            mesh_root.m_drawRanges.push_back(range_ex.m_range);
        }
        for(const auto& range_ex : render_data.m_meshPointRanges) {
            mesh_root.m_drawRanges.push_back(range_ex.m_range);
        }

        for(const auto& node : input.m_nodes) {
            if(node.nodeId() != Mesh::INVALID_MESH_NODE_ID) {
                mesh_root.m_bbox.expand(node.position());
            }
        }

        render_data.m_sceneBBox.expand(mesh_root.m_bbox);
        render_data.m_meshRoots.push_back(std::move(mesh_root));
    }

    mesh_pass.markDataUpdated();

    LOG_DEBUG("MeshRenderBuilder::build: surface={}, wireframe={}, nodes={}, elements={}, parts={}",
              surfaceVertexCount, wireframeVertexCount, nodeVertexCount, input.m_elements.size(),
              part_order.size());
    return true;
}

} // namespace OpenGeoLab::Render
