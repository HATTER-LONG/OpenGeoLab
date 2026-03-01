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
#include "util/point_vector3d.hpp"

#include <cmath>

namespace OpenGeoLab::Mesh {

namespace {

using Vec3f = Util::Vec3f;

Vec3f toVec3f(const Util::Pt3d& p);

struct PrimitiveStyle {
    Render::RenderColor m_color;
    uint64_t m_pickId{0};
};

struct BuildContext {
    Render::RenderData& m_renderData;
    const Mesh::MeshRenderInput& m_input;
    const Util::ColorMap& m_colorMap;
    Render::RenderPassData& m_meshPass;
    uint32_t m_surfaceVertexCount{0};
    uint32_t m_wireframeVertexCount{0};
    uint32_t m_nodeVertexCount{0};

    BuildContext(Render::RenderData& render_data,
                 const Mesh::MeshRenderInput& input,
                 const Util::ColorMap& color_map,
                 Render::RenderPassData& mesh_pass)
        : m_renderData(render_data), m_input(input), m_colorMap(color_map), m_meshPass(mesh_pass) {}

    Vec3f nodePos(Mesh::MeshNodeId nid) const {
        if(nid == Mesh::INVALID_MESH_NODE_ID || nid > m_input.m_nodes.size()) {
            return {};
        }
        return toVec3f(m_input.m_nodes[static_cast<size_t>(nid - 1)].position());
    }
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

void pushVertex(Render::RenderPassData& pass,
                const Vec3f& pos,
                const Vec3f& normal,
                const PrimitiveStyle& style) {
    Render::RenderVertex v{};
    v.m_position[0] = pos.x;
    v.m_position[1] = pos.y;
    v.m_position[2] = pos.z;
    v.m_normal[0] = normal.x;
    v.m_normal[1] = normal.y;
    v.m_normal[2] = normal.z;
    v.m_color[0] = style.m_color.m_r;
    v.m_color[1] = style.m_color.m_g;
    v.m_color[2] = style.m_color.m_b;
    v.m_color[3] = style.m_color.m_a;
    v.m_pickId = style.m_pickId;
    pass.m_vertices.push_back(v);
}

void pushTriangle(Render::RenderPassData& pass,
                  const Vec3f& a,
                  const Vec3f& b,
                  const Vec3f& c,
                  const PrimitiveStyle& style) {
    const Vec3f n = computeTriangleNormal(a, b, c);
    pushVertex(pass, a, n, style);
    pushVertex(pass, b, n, style);
    pushVertex(pass, c, n, style);
}

void pushLine(Render::RenderPassData& pass,
              const Vec3f& a,
              const Vec3f& b,
              const PrimitiveStyle& style) {
    const Vec3f zero{0.0f, 0.0f, 0.0f};
    pushVertex(pass, a, zero, style);
    pushVertex(pass, b, zero, style);
}

void pushEdge(Render::RenderPassData& pass,
              const Vec3f& a,
              const Vec3f& b,
              const PrimitiveStyle& style) {
    pushLine(pass, a, b, style);
}

// 3D element face table definitions
constexpr int TETRA4_FACES[][3] = {{0, 1, 2}, {0, 3, 1}, {1, 3, 2}, {0, 2, 3}};

constexpr int HEXA8_FACES[][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                                  {2, 3, 7, 6}, {0, 4, 7, 3}, {1, 2, 6, 5}};

constexpr int PRISM6_TRI_FACES[][3] = {{0, 1, 2}, {3, 5, 4}};
constexpr int PRISM6_QUAD_FACES[][4] = {{0, 3, 4, 1}, {1, 4, 5, 2}, {0, 2, 5, 3}};

constexpr int PYRAMID5_BASE[] = {0, 3, 2, 1};
constexpr int PYRAMID5_TRI_FACES[][3] = {{0, 1, 4}, {1, 2, 4}, {2, 3, 4}, {0, 4, 3}};

void appendSurfaceTriangles(BuildContext& ctx) {
    constexpr float darken_factor = 0.8f;
    const Render::RenderColor fallback_color{0.55f, 0.65f, 0.75f, 1.0f};

    for(const auto& elem : ctx.m_input.m_elements) {
        if(!elem.isValid()) {
            continue;
        }

        const Render::RenderEntityType render_type = Render::toRenderEntityType(elem.elementType());

        // Derive surface color from the element's parent Part color
        Render::RenderColor surface_color = fallback_color;
        if(elem.partUid() != 0) {
            const auto& part_color = ctx.m_colorMap.getColorForPartId(elem.partUid());
            surface_color = Util::ColorMap::darkenColor(part_color, darken_factor);
        }

        const PrimitiveStyle style{surface_color,
                                   Render::PickId::encode(render_type, elem.elementUID())};

        switch(elem.elementType()) {
        case Mesh::MeshElementType::Triangle: {
            const Vec3f p0 = ctx.nodePos(elem.nodeId(0));
            const Vec3f p1 = ctx.nodePos(elem.nodeId(1));
            const Vec3f p2 = ctx.nodePos(elem.nodeId(2));
            pushTriangle(ctx.m_meshPass, p0, p1, p2, style);
            break;
        }
        case Mesh::MeshElementType::Quad4: {
            const Vec3f p0 = ctx.nodePos(elem.nodeId(0));
            const Vec3f p1 = ctx.nodePos(elem.nodeId(1));
            const Vec3f p2 = ctx.nodePos(elem.nodeId(2));
            const Vec3f p3 = ctx.nodePos(elem.nodeId(3));
            pushTriangle(ctx.m_meshPass, p0, p1, p2, style);
            pushTriangle(ctx.m_meshPass, p0, p2, p3, style);
            break;
        }
        case Mesh::MeshElementType::Tetra4: {
            for(const auto& face : TETRA4_FACES) {
                const Vec3f p0 = ctx.nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = ctx.nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = ctx.nodePos(elem.nodeId(face[2]));
                pushTriangle(ctx.m_meshPass, p0, p1, p2, style);
            }
            break;
        }
        case Mesh::MeshElementType::Hexa8: {
            for(const auto& face : HEXA8_FACES) {
                const Vec3f p0 = ctx.nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = ctx.nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = ctx.nodePos(elem.nodeId(face[2]));
                const Vec3f p3 = ctx.nodePos(elem.nodeId(face[3]));
                pushTriangle(ctx.m_meshPass, p0, p1, p2, style);
                pushTriangle(ctx.m_meshPass, p0, p2, p3, style);
            }
            break;
        }
        case Mesh::MeshElementType::Prism6: {
            for(const auto& face : PRISM6_TRI_FACES) {
                const Vec3f p0 = ctx.nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = ctx.nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = ctx.nodePos(elem.nodeId(face[2]));
                pushTriangle(ctx.m_meshPass, p0, p1, p2, style);
            }
            for(const auto& face : PRISM6_QUAD_FACES) {
                const Vec3f p0 = ctx.nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = ctx.nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = ctx.nodePos(elem.nodeId(face[2]));
                const Vec3f p3 = ctx.nodePos(elem.nodeId(face[3]));
                pushTriangle(ctx.m_meshPass, p0, p1, p2, style);
                pushTriangle(ctx.m_meshPass, p0, p2, p3, style);
            }
            break;
        }
        case Mesh::MeshElementType::Pyramid5: {
            const Vec3f b0 = ctx.nodePos(elem.nodeId(PYRAMID5_BASE[0]));
            const Vec3f b1 = ctx.nodePos(elem.nodeId(PYRAMID5_BASE[1]));
            const Vec3f b2 = ctx.nodePos(elem.nodeId(PYRAMID5_BASE[2]));
            const Vec3f b3 = ctx.nodePos(elem.nodeId(PYRAMID5_BASE[3]));
            pushTriangle(ctx.m_meshPass, b0, b1, b2, style);
            pushTriangle(ctx.m_meshPass, b0, b2, b3, style);
            for(const auto& face : PYRAMID5_TRI_FACES) {
                const Vec3f p0 = ctx.nodePos(elem.nodeId(face[0]));
                const Vec3f p1 = ctx.nodePos(elem.nodeId(face[1]));
                const Vec3f p2 = ctx.nodePos(elem.nodeId(face[2]));
                pushTriangle(ctx.m_meshPass, p0, p1, p2, style);
            }
            break;
        }
        default:
            break;
        }
    }

    ctx.m_surfaceVertexCount = static_cast<uint32_t>(ctx.m_meshPass.m_vertices.size());
}

/**
 * @brief Append wireframe edges from Line elements in the input.
 *
 * Line elements are pre-built by MeshDocument::buildEdgeElements() with
 * proper MeshElementUIDs. Each Line element is rendered once using its
 * UID as the pick ID.
 */
void appendWireframeEdges(BuildContext& ctx) {
    const Render::RenderColor wire_color = ctx.m_colorMap.getMeshLineColor();

    for(const auto& elem : ctx.m_input.m_elements) {
        if(!elem.isValid() || elem.elementType() != Mesh::MeshElementType::Line) {
            continue;
        }

        const PrimitiveStyle style{
            wire_color,
            Render::PickId::encode(Render::RenderEntityType::MeshLine, elem.elementUID())};
        pushEdge(ctx.m_meshPass, ctx.nodePos(elem.nodeId(0)), ctx.nodePos(elem.nodeId(1)), style);
    }

    ctx.m_wireframeVertexCount =
        static_cast<uint32_t>(ctx.m_meshPass.m_vertices.size()) - ctx.m_surfaceVertexCount;
}

void appendNodePoints(BuildContext& ctx) {
    const Render::RenderColor node_color = ctx.m_colorMap.getMeshNodeColor();
    const Vec3f zero_normal{0.0f, 0.0f, 0.0f};

    for(const auto& node : ctx.m_input.m_nodes) {
        if(node.nodeId() == Mesh::INVALID_MESH_NODE_ID) {
            continue;
        }

        const PrimitiveStyle style{
            node_color, Render::PickId::encode(Render::RenderEntityType::MeshNode, node.nodeId())};
        pushVertex(ctx.m_meshPass, toVec3f(node.position()), zero_normal, style);
    }

    ctx.m_nodeVertexCount = static_cast<uint32_t>(ctx.m_meshPass.m_vertices.size()) -
                            ctx.m_surfaceVertexCount - ctx.m_wireframeVertexCount;
}

void appendMeshRootNode(BuildContext& ctx) {
    if(ctx.m_meshPass.m_vertices.empty()) {
        return;
    }

    Render::RenderNode mesh_root;
    mesh_root.m_key = {Render::RenderEntityType::MeshTriangle, 0};
    mesh_root.m_visible = true;

    if(ctx.m_surfaceVertexCount > 0) {
        Render::DrawRange surface_range;
        surface_range.m_vertexOffset = 0;
        surface_range.m_vertexCount = ctx.m_surfaceVertexCount;
        surface_range.m_topology = Render::PrimitiveTopology::Triangles;
        mesh_root.m_drawRanges[Render::RenderPassType::Mesh].push_back(surface_range);
    }

    if(ctx.m_wireframeVertexCount > 0) {
        Render::DrawRange wire_range;
        wire_range.m_vertexOffset = ctx.m_surfaceVertexCount;
        wire_range.m_vertexCount = ctx.m_wireframeVertexCount;
        wire_range.m_topology = Render::PrimitiveTopology::Lines;
        mesh_root.m_drawRanges[Render::RenderPassType::Mesh].push_back(wire_range);
    }

    if(ctx.m_nodeVertexCount > 0) {
        Render::DrawRange node_range;
        node_range.m_vertexOffset = ctx.m_surfaceVertexCount + ctx.m_wireframeVertexCount;
        node_range.m_vertexCount = ctx.m_nodeVertexCount;
        node_range.m_topology = Render::PrimitiveTopology::Points;
        mesh_root.m_drawRanges[Render::RenderPassType::Mesh].push_back(node_range);
    }

    for(const auto& node : ctx.m_input.m_nodes) {
        if(node.nodeId() != Mesh::INVALID_MESH_NODE_ID) {
            mesh_root.m_bbox.expand(node.position());
        }
    }

    ctx.m_renderData.m_sceneBBox.expand(mesh_root.m_bbox);
    ctx.m_renderData.m_roots.push_back(std::move(mesh_root));
}

} // namespace

bool MeshRenderBuilder::build(Render::RenderData& render_data, const MeshRenderInput& input) {
    render_data.clearMesh();

    if(input.m_nodes.empty() || input.m_elements.empty()) {
        return true;
    }

    auto& mesh_pass = render_data.m_passData[Render::RenderPassType::Mesh];
    BuildContext ctx{render_data, input, Util::ColorMap::instance(), mesh_pass};
    appendSurfaceTriangles(ctx);
    appendWireframeEdges(ctx);
    appendNodePoints(ctx);
    appendMeshRootNode(ctx);

    mesh_pass.markDataUpdated();
    render_data.markMeshUpdated();

    LOG_DEBUG("MeshRenderBuilder::build: surface={}, wireframe={}, nodes={}, elements={}",
              ctx.m_surfaceVertexCount, ctx.m_wireframeVertexCount, ctx.m_nodeVertexCount,
              input.m_elements.size());
    return true;
}

} // namespace OpenGeoLab::Mesh
