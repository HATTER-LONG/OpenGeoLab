#include "mesh_render_builder.hpp"

#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <cmath>

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

// 3D element face table definitions
constexpr int TETRA4_FACES[][3] = {{0, 1, 2}, {0, 3, 1}, {1, 3, 2}, {0, 2, 3}};

constexpr int HEXA8_FACES[][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                                  {2, 3, 7, 6}, {0, 4, 7, 3}, {1, 2, 6, 5}};

constexpr int PRISM6_TRI_FACES[][3] = {{0, 1, 2}, {3, 5, 4}};
constexpr int PRISM6_QUAD_FACES[][4] = {{0, 3, 4, 1}, {1, 4, 5, 2}, {0, 2, 5, 3}};

constexpr int PYRAMID5_BASE[] = {0, 3, 2, 1};
constexpr int PYRAMID5_TRI_FACES[][3] = {{0, 1, 4}, {1, 2, 4}, {2, 3, 4}, {0, 4, 3}};

} // namespace

bool MeshRenderBuilder::build(RenderData& render_data, const MeshRenderInput& input) {
    render_data.clearMesh();

    if(input.m_nodes.empty() || input.m_elements.empty()) {
        return true;
    }

    const auto& color_map = Util::ColorMap::instance();

    auto& surface_pass = render_data.m_passData[RenderPassType::Mesh];

    auto nodePos = [&input](Mesh::MeshNodeId nid) -> Vec3f {
        if(nid == Mesh::INVALID_MESH_NODE_ID || nid > input.m_nodes.size()) {
            return {};
        }
        return toVec3f(input.m_nodes[static_cast<size_t>(nid - 1)].position());
    };

    for(const auto& elem : input.m_elements) {
        if(!elem.isValid()) {
            continue;
        }

        const RenderColor elem_color = color_map.getColorForMeshElementId(elem.elementUID());
        const RenderEntityType render_type = toRenderEntityType(elem.elementType());
        const uint64_t pick_id = PickId::encode(render_type, elem.elementUID());

        switch(elem.elementType()) {
        case Mesh::MeshElementType::Triangle: {
            const Vec3f p0 = nodePos(elem.nodeId(0));
            const Vec3f p1 = nodePos(elem.nodeId(1));
            const Vec3f p2 = nodePos(elem.nodeId(2));
            pushTriangle(surface_pass, p0, p1, p2, elem_color, pick_id);
            break;
        }
        case Mesh::MeshElementType::Quad4: {
            const Vec3f p0 = nodePos(elem.nodeId(0));
            const Vec3f p1 = nodePos(elem.nodeId(1));
            const Vec3f p2 = nodePos(elem.nodeId(2));
            const Vec3f p3 = nodePos(elem.nodeId(3));
            pushTriangle(surface_pass, p0, p1, p2, elem_color, pick_id);
            pushTriangle(surface_pass, p0, p2, p3, elem_color, pick_id);
            break;
        }
        case Mesh::MeshElementType::Line: {
            const RenderColor line_color = color_map.getMeshLineColor();
            const Vec3f p0 = nodePos(elem.nodeId(0));
            const Vec3f p1 = nodePos(elem.nodeId(1));
            pushLine(surface_pass, p0, p1, line_color, pick_id);
            break;
        }
        case Mesh::MeshElementType::Tetra4: {
            for(const auto& face : TETRA4_FACES) {
                const Vec3f p0 = nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = nodePos(elem.nodeId(face[2]));
                pushTriangle(surface_pass, p0, p1, p2, elem_color, pick_id);
            }
            break;
        }
        case Mesh::MeshElementType::Hexa8: {
            for(const auto& face : HEXA8_FACES) {
                const Vec3f p0 = nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = nodePos(elem.nodeId(face[2]));
                const Vec3f p3 = nodePos(elem.nodeId(face[3]));
                pushTriangle(surface_pass, p0, p1, p2, elem_color, pick_id);
                pushTriangle(surface_pass, p0, p2, p3, elem_color, pick_id);
            }
            break;
        }
        case Mesh::MeshElementType::Prism6: {
            for(const auto& face : PRISM6_TRI_FACES) {
                const Vec3f p0 = nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = nodePos(elem.nodeId(face[2]));
                pushTriangle(surface_pass, p0, p1, p2, elem_color, pick_id);
            }
            for(const auto& face : PRISM6_QUAD_FACES) {
                const Vec3f p0 = nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = nodePos(elem.nodeId(face[2]));
                const Vec3f p3 = nodePos(elem.nodeId(face[3]));
                pushTriangle(surface_pass, p0, p1, p2, elem_color, pick_id);
                pushTriangle(surface_pass, p0, p2, p3, elem_color, pick_id);
            }
            break;
        }
        case Mesh::MeshElementType::Pyramid5: {
            const Vec3f b0 = nodePos(elem.nodeId(PYRAMID5_BASE[0]));
            const Vec3f b1 = nodePos(elem.nodeId(PYRAMID5_BASE[1]));
            const Vec3f b2 = nodePos(elem.nodeId(PYRAMID5_BASE[2]));
            const Vec3f b3 = nodePos(elem.nodeId(PYRAMID5_BASE[3]));
            pushTriangle(surface_pass, b0, b1, b2, elem_color, pick_id);
            pushTriangle(surface_pass, b0, b2, b3, elem_color, pick_id);
            for(const auto& face : PYRAMID5_TRI_FACES) {
                const Vec3f p0 = nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = nodePos(elem.nodeId(face[2]));
                pushTriangle(surface_pass, p0, p1, p2, elem_color, pick_id);
            }
            break;
        }
        default:
            break;
        }
    }

    // Mesh nodes as points
    const RenderColor node_color = color_map.getMeshNodeColor();
    const Vec3f zero_normal{0.0f, 0.0f, 0.0f};
    for(const auto& node : input.m_nodes) {
        if(node.nodeId() == Mesh::INVALID_MESH_NODE_ID) {
            continue;
        }
        const uint64_t node_pick_id = PickId::encode(RenderEntityType::MeshNode, node.nodeId());
        pushVertex(surface_pass, toVec3f(node.position()), zero_normal, node_color, node_pick_id);
    }

    // Build a single mesh root node
    if(!surface_pass.m_vertices.empty()) {
        RenderNode mesh_root;
        mesh_root.m_key = {RenderEntityType::MeshTriangle, 0};
        mesh_root.m_visible = true;

        DrawRange range;
        range.m_vertexOffset = 0;
        range.m_vertexCount = static_cast<uint32_t>(surface_pass.m_vertices.size());
        range.m_topology = PrimitiveTopology::Triangles;
        mesh_root.m_drawRanges[RenderPassType::Mesh].push_back(range);

        for(const auto& node : input.m_nodes) {
            if(node.nodeId() != Mesh::INVALID_MESH_NODE_ID) {
                mesh_root.m_bbox.expand(node.position());
            }
        }

        render_data.m_sceneBBox.expand(mesh_root.m_bbox);
        render_data.m_roots.push_back(std::move(mesh_root));
    }

    surface_pass.m_dirty = true;
    render_data.m_meshDirty = true;

    LOG_DEBUG("MeshRenderBuilder::build: mesh vertices={}, elements={}",
              surface_pass.m_vertices.size(), input.m_elements.size());
    return true;
}

} // namespace OpenGeoLab::Render
