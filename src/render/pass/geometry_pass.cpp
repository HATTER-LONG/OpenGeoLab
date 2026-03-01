#include "geometry_pass.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Render {
namespace {
const char* surface_vertex_shader = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;
out vec3 v_worldPos;
out vec3 v_normal;
out vec4 v_color;
void main() {
    v_worldPos = a_position;
    v_normal = a_normal;
    v_color = a_color;
    gl_Position = u_projMatrix * u_viewMatrix * vec4(a_position, 1.0);
}
)";

const char* surface_fragment_shader = R"(
#version 330 core
in vec3 v_worldPos;
in vec3 v_normal;
in vec4 v_color;
uniform vec3 u_cameraPos;
uniform vec4 u_highlightColor;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(u_cameraPos - v_worldPos);
    float ambient = 0.35;
    float headlamp = abs(dot(N, V));
    float skyLight = max(dot(N, vec3(0.0, 1.0, 0.0)), 0.0) * 0.15;
    float groundBounce = max(dot(N, vec3(0.0, -1.0, 0.0)), 0.0) * 0.05;
    float lighting = ambient + headlamp * 0.55 + skyLight + groundBounce;
    vec3 litColor = v_color.rgb * min(lighting, 1.0);
    vec3 finalColor = litColor;
    if(u_highlightColor.a > 0.0) {
        finalColor = mix(litColor, u_highlightColor.rgb, u_highlightColor.a);
    }
    // Premultiply alpha for correct Qt Quick scene-graph compositing
    fragColor = vec4(finalColor * u_alpha, u_alpha);
}
)";

const char* flat_vertex_shader = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;
uniform float u_pointSize;
out vec4 v_color;
void main() {
    v_color = a_color;
    gl_PointSize = u_pointSize;
    gl_Position = u_projMatrix * u_viewMatrix * vec4(a_position, 1.0);
}
)";

const char* flat_fragment_shader = R"(
#version 330 core
in vec4 v_color;
uniform vec4 u_highlightColor;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    vec3 color = v_color.rgb;
    if(u_highlightColor.a > 0.0) {
        color = mix(color, u_highlightColor.rgb, u_highlightColor.a);
    }
    // Premultiply alpha for correct Qt Quick scene-graph compositing
    fragColor = vec4(color * u_alpha, v_color.a * u_alpha);
}
)";
/**
 * @brief Recursively collect DrawRangeEx from a visible RenderNode tree
 *        for the Geometry pass, grouped by primitive topology.
 * @param edge_to_wires Edge UID → Wire UID(s) lookup for Wire picking support.
 *        An edge shared between two faces maps to multiple wires.
 */
void collectDrawRangesEx(const RenderNode& node,
                         uint64_t part_uid,
                         const std::unordered_map<uint64_t, std::vector<uint64_t>>& edge_to_wires,
                         std::vector<DrawRangeEx>& tris,
                         std::vector<DrawRangeEx>& lines,
                         std::vector<DrawRangeEx>& points) {
    if(!node.m_visible) {
        return;
    }

    // Track the current part uid
    uint64_t current_part_uid = part_uid;
    if(node.m_key.m_type == RenderEntityType::Part) {
        current_part_uid = node.m_key.m_uid;
    }

    auto it = node.m_drawRanges.find(RenderPassType::Geometry);
    if(it != node.m_drawRanges.end()) {
        for(const auto& range : it->second) {
            DrawRangeEx rangeEx;
            rangeEx.m_range = range;
            rangeEx.m_entityKey = node.m_key;
            rangeEx.m_partUid = current_part_uid;

            // Resolve wire uid for edges (use first wire if multiple exist)
            if(node.m_key.m_type == RenderEntityType::Edge) {
                auto wit = edge_to_wires.find(node.m_key.m_uid);
                if(wit != edge_to_wires.end() && !wit->second.empty()) {
                    rangeEx.m_wireUid = wit->second[0];
                }
            }

            switch(range.m_topology) {
            case PrimitiveTopology::Triangles:
                tris.push_back(rangeEx);
                break;
            case PrimitiveTopology::Lines:
                lines.push_back(rangeEx);
                break;
            case PrimitiveTopology::Points:
                points.push_back(rangeEx);
                break;
            }
        }
    }

    for(const auto& child : node.m_children) {
        collectDrawRangesEx(child, current_part_uid, edge_to_wires, tris, lines, points);
    }
}
} // namespace

void GeometryPass::initialize() {
    if(m_initialized) {
        return;
    }

    if(!m_surfaceShader.compile(surface_vertex_shader, surface_fragment_shader)) {
        LOG_ERROR("GeometryPass: Failed to compile surface shader");
        return;
    }

    if(!m_flatShader.compile(flat_vertex_shader, flat_fragment_shader)) {
        LOG_ERROR("GeometryPass: Failed to compile flat shader");
        return;
    }

    m_gpuBuffer.initialize();
    m_initialized = true;
    LOG_DEBUG("GeometryPass: Initialized");
}

void GeometryPass::cleanup() {
    m_triangleRanges.clear();
    m_lineRanges.clear();
    m_pointRanges.clear();
    m_gpuBuffer.cleanup();
    m_initialized = false;
    LOG_DEBUG("GeometryPass: Cleaned up");
}

// =============================================================================
// Buffer update
// =============================================================================

void GeometryPass::updateBuffers(const RenderData& data) {
    auto geo_pass_iter = data.m_passData.find(RenderPassType::Geometry);
    if(geo_pass_iter == data.m_passData.end()) {
        // No geometry pass data — clear cached ranges
        m_triangleRanges.clear();
        m_lineRanges.clear();
        m_pointRanges.clear();
        return;
    }

    const RenderPassData& pass_data = geo_pass_iter->second;

    if(!m_gpuBuffer.upload(pass_data)) {
        LOG_ERROR("GeometryPass: Failed to upload GPU buffer data");
        return;
    }

    // Rebuild draw range lists by walking the semantic tree
    m_triangleRanges.clear();
    m_lineRanges.clear();
    m_pointRanges.clear();

    for(const auto& root : data.m_roots) {
        if(isGeometryDomain(root.m_key.m_type)) {
            collectDrawRangesEx(root, 0, data.m_pickData.m_edgeToWireUids, m_triangleRanges,
                                m_lineRanges, m_pointRanges);
        }
    }
}
} // namespace OpenGeoLab::Render