#include "geometry_pass.hpp"
#include "util/color_map.hpp"
#include "util/logger.hpp"

#include "render/render_select_manager.hpp"

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
out vec4 fragColor;
void main() {
    vec3 color = v_color.rgb;
    if(u_highlightColor.a > 0.0) {
        color = u_highlightColor.rgb;
    }
    fragColor = vec4(color, v_color.a);
}
)";
/**
 * @brief Recursively collect DrawRangeEx from a visible RenderNode tree
 *        for the Geometry pass, grouped by primitive topology.
 * @param edge_to_wires Edge UID → Wire UID(s) lookup for Wire picking support.
 *        An edge shared between two faces maps to multiple wires.
 */
void collectDrawRangesEx(const RenderNode& node, // NOLINT
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
            DrawRangeEx range_ex;
            range_ex.m_range = range;
            range_ex.m_entityKey = node.m_key;
            range_ex.m_partUid = current_part_uid;

            // Resolve wire uid for edges (use first wire if multiple exist)
            // if(node.m_key.m_type == RenderEntityType::Edge) {
            //     auto wit = edge_to_wires.find(node.m_key.m_uid);
            //     if(wit != edge_to_wires.end() && !wit->second.empty()) {
            //         range_ex.m_wireUid = wit->second[0];
            //     }
            // }

            switch(range.m_topology) {
            case PrimitiveTopology::Triangles:
                tris.push_back(range_ex);
                break;
            case PrimitiveTopology::Lines:
                lines.push_back(range_ex);
                break;
            case PrimitiveTopology::Points:
                points.push_back(range_ex);
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
    if(data.m_geometryVersion == m_uploadedVertexVersion) {
        return;
    }
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

    m_uploadedVertexVersion = data.m_geometryVersion;
}

void GeometryPass::renderTriangles(QOpenGLFunctions* f,
                                   const QMatrix4x4& view,
                                   const QMatrix4x4& projection,
                                   const QVector3D& camera_pos,
                                   bool x_ray_mode) {
    if(m_triangleRanges.empty()) {
        return;
    }

    const auto& select_mgr = RenderSelectManager::instance();
    const auto& color_map = Util::ColorMap::instance();
    const bool part_mode = select_mgr.isTypePickable(RenderEntityType::Part);
    const auto& face_hover = color_map.getFaceHoverColor();
    const auto& face_select = color_map.getFaceSelectionColor();
    const float surface_alpha = x_ray_mode ? 0.25f : 1.0f;

    // Enable blending for X-ray mode
    if(x_ray_mode) {
        f->glEnable(GL_BLEND);
        f->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        f->glDepthMask(GL_FALSE);
    }

    m_surfaceShader.bind();
    m_surfaceShader.setUniformMatrix4("u_viewMatrix", view);
    m_surfaceShader.setUniformMatrix4("u_projMatrix", projection);
    m_surfaceShader.setUniformVec3("u_cameraPos", camera_pos);
    m_surfaceShader.setUniformFloat("u_alpha", surface_alpha);

    for(const auto& range_ex : m_triangleRanges) {
        if(select_mgr.isSelected(range_ex.m_entityKey) ||
           (part_mode && select_mgr.isPartSelected(range_ex.m_partUid))) {
            m_surfaceShader.setUniformVec4("u_highlightColor", face_select.m_r, face_select.m_g,
                                           face_select.m_b, 0.5f);
        } else if(select_mgr.isEntityHovered(range_ex.m_entityKey) ||
                  (part_mode && select_mgr.isPartHovered(range_ex.m_partUid))) {
            m_surfaceShader.setUniformVec4("u_highlightColor", face_hover.m_r, face_hover.m_g,
                                           face_hover.m_b, 0.4f);
        } else {
            m_surfaceShader.setUniformVec4("u_highlightColor", 0.0f, 0.0f, 0.0f, 0.0f);
        }

        const auto& range = range_ex.m_range;
        f->glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(range.m_indexCount), GL_UNSIGNED_INT,
                          reinterpret_cast<const void*>(
                              static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
    }

    m_surfaceShader.release();

    if(x_ray_mode) {
        f->glDepthMask(GL_TRUE);
        f->glDisable(GL_BLEND);
    }
}

void GeometryPass::renderLines(QOpenGLFunctions* f,
                               const QMatrix4x4& view,
                               const QMatrix4x4& projection) {
    if(m_lineRanges.empty()) {
        return;
    }

    const auto& select_mgr = RenderSelectManager::instance();
    const auto& color_map = Util::ColorMap::instance();
    const bool part_mode = select_mgr.isTypePickable(RenderEntityType::Part);
    const bool wire_mode = select_mgr.isTypePickable(RenderEntityType::Wire);
    const auto& ev_hover = color_map.getEdgeVertexHoverColor();
    const auto& ev_select = color_map.getEdgeVertexSelectionColor();

    const float edge_width_default = Util::RenderStyle::EDGE_LINE_WIDTH;
    const float edge_width_hover = Util::RenderStyle::EDGE_LINE_WIDTH_HOVER;
    const float edge_width_selected = Util::RenderStyle::EDGE_LINE_WIDTH_SELECTED;

    m_flatShader.bind();
    m_flatShader.setUniformMatrix4("u_viewMatrix", view);
    m_flatShader.setUniformMatrix4("u_projMatrix", projection);

    for(const auto& range_ex : m_lineRanges) {
        float line_width = edge_width_default;

        if(select_mgr.isSelected(range_ex.m_entityKey) ||
           (part_mode && select_mgr.isPartSelected(range_ex.m_partUid)) ||
           (wire_mode && select_mgr.isEdgeInSelectedWire(range_ex.m_entityKey.m_uid))) {
            m_flatShader.setUniformVec4("u_highlightColor", ev_select.m_r, ev_select.m_g,
                                        ev_select.m_b, 1.0f);
            line_width = edge_width_selected;
        } else if(select_mgr.isEntityHovered(range_ex.m_entityKey) ||
                  (part_mode && select_mgr.isPartHovered(range_ex.m_partUid)) ||
                  (wire_mode && select_mgr.isEdgeInHoveredWire(range_ex.m_entityKey.m_uid))) {
            m_flatShader.setUniformVec4("u_highlightColor", ev_hover.m_r, ev_hover.m_g,
                                        ev_hover.m_b, 1.0f);
            line_width = edge_width_hover;
        } else {
            m_flatShader.setUniformVec4("u_highlightColor", 0.0f, 0.0f, 0.0f, 0.0f);
        }

        f->glLineWidth(line_width);
        const auto& range = range_ex.m_range;
        f->glDrawElements(GL_LINES, static_cast<GLsizei>(range.m_indexCount), GL_UNSIGNED_INT,
                          reinterpret_cast<const void*>(
                              static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
    }

    f->glLineWidth(1.0f);
    m_flatShader.release();
}

void GeometryPass::renderPoints(QOpenGLFunctions* f,
                                const QMatrix4x4& view,
                                const QMatrix4x4& projection) {
    if(m_pointRanges.empty()) {
        return;
    }

    const auto& select_mgr = RenderSelectManager::instance();
    const auto& color_map = Util::ColorMap::instance();
    const bool part_mode = select_mgr.isTypePickable(RenderEntityType::Part);
    const auto& ev_hover = color_map.getEdgeVertexHoverColor();
    const auto& ev_select = color_map.getEdgeVertexSelectionColor();

    const float vtx_size_base = Util::RenderStyle::VERTEX_POINT_SIZE;
    const float vtx_scale_hover = Util::RenderStyle::VERTEX_SCALE_HOVER;
    const float vtx_scale_selected = Util::RenderStyle::VERTEX_SCALE_SELECTED;

    m_flatShader.bind();
    m_flatShader.setUniformMatrix4("u_viewMatrix", view);
    m_flatShader.setUniformMatrix4("u_projMatrix", projection);
    f->glEnable(GL_PROGRAM_POINT_SIZE);

    for(const auto& range_ex : m_pointRanges) {
        float point_size = vtx_size_base;

        if(select_mgr.isSelected(range_ex.m_entityKey) ||
           (part_mode && select_mgr.isPartSelected(range_ex.m_partUid))) {
            m_flatShader.setUniformVec4("u_highlightColor", ev_select.m_r, ev_select.m_g,
                                        ev_select.m_b, 1.0f);
            point_size = vtx_size_base * vtx_scale_selected;
        } else if(select_mgr.isEntityHovered(range_ex.m_entityKey) ||
                  (part_mode && select_mgr.isPartHovered(range_ex.m_partUid))) {
            m_flatShader.setUniformVec4("u_highlightColor", ev_hover.m_r, ev_hover.m_g,
                                        ev_hover.m_b, 1.0f);
            point_size = vtx_size_base * vtx_scale_hover;
        } else {
            m_flatShader.setUniformVec4("u_highlightColor", 0.0f, 0.0f, 0.0f, 0.0f);
        }

        m_flatShader.setUniformFloat("u_pointSize", point_size);
        const auto& range = range_ex.m_range;
        f->glDrawArrays(GL_POINTS, static_cast<GLint>(range.m_vertexOffset),
                        static_cast<GLsizei>(range.m_vertexCount));
    }

    m_flatShader.release();
}

void GeometryPass::render(const QMatrix4x4& view,
                          const QMatrix4x4& projection,
                          const QVector3D& camera_pos,
                          bool x_ray_mode) {
    if(!m_initialized || m_gpuBuffer.vertexCount() == 0) {
        return;
    }
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();

    m_gpuBuffer.bindForDraw();
    f->glEnable(GL_DEPTH_TEST);
    // Delegated rendering for clarity
    renderTriangles(f, view, projection, camera_pos, x_ray_mode);
    renderLines(f, view, projection);
    renderPoints(f, view, projection);

    m_gpuBuffer.unbind();
}
} // namespace OpenGeoLab::Render