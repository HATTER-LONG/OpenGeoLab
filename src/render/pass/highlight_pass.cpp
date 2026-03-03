/**
 * @file highlight_pass.cpp
 * @brief HighlightPass implementation — selected/hovered entity overlay
 */

#include "highlight_pass.hpp"

#include "render/render_select_manager.hpp"
#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>

namespace OpenGeoLab::Render {
namespace {

const char* highlight_surface_vertex_shader = R"(
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

const char* highlight_surface_fragment_shader = R"(
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
    vec3 finalColor = mix(litColor, u_highlightColor.rgb, u_highlightColor.a);
    // Premultiply alpha for correct Qt Quick scene-graph compositing
    fragColor = vec4(finalColor * u_alpha, u_alpha);
}
)";

const char* highlight_flat_vertex_shader = R"(
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

const char* highlight_flat_fragment_shader = R"(
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

} // namespace

void HighlightPass::initialize() {
    if(m_initialized) {
        return;
    }
    if(!m_surfaceShader.compile(highlight_surface_vertex_shader,
                                highlight_surface_fragment_shader)) {
        LOG_ERROR("HighlightPass: Failed to compile surface shader");
        return;
    }
    if(!m_flatShader.compile(highlight_flat_vertex_shader, highlight_flat_fragment_shader)) {
        LOG_ERROR("HighlightPass: Failed to compile flat shader");
        return;
    }
    m_initialized = true;
    LOG_DEBUG("HighlightPass: Initialized");
}

void HighlightPass::cleanup() {
    m_initialized = false;
    LOG_DEBUG("HighlightPass: Cleaned up");
}

void HighlightPass::render(QOpenGLFunctions* f,
                           GpuBuffer& gpu_buffer,
                           const QMatrix4x4& view,
                           const QMatrix4x4& projection,
                           const QVector3D& camera_pos,
                           const std::vector<DrawRange>& triangle_ranges,
                           const std::vector<DrawRange>& line_ranges,
                           const std::vector<DrawRange>& point_ranges) {
    if(!m_initialized) {
        return;
    }

    // Early exit if nothing is selected or hovered
    const auto& select_mgr = RenderSelectManager::instance();
    if(select_mgr.hoveredEntity().m_type == RenderEntityType::None &&
       select_mgr.selections().empty()) {
        return;
    }

    gpu_buffer.bindForDraw();

    // Use GL_LEQUAL so highlights draw exactly on top of existing geometry
    f->glDepthFunc(GL_LEQUAL);

    renderHighlightedTriangles(f, view, projection, camera_pos, triangle_ranges);
    renderHighlightedLines(f, view, projection, line_ranges);
    renderHighlightedPoints(f, view, projection, point_ranges);

    // Restore default depth function
    f->glDepthFunc(GL_LESS);

    gpu_buffer.unbind();
}

void HighlightPass::renderHighlightedTriangles(QOpenGLFunctions* f,
                                               const QMatrix4x4& view,
                                               const QMatrix4x4& projection,
                                               const QVector3D& camera_pos,
                                               const std::vector<DrawRange>& triangle_ranges) {
    if(triangle_ranges.empty()) {
        return;
    }

    const auto& select_mgr = RenderSelectManager::instance();
    const auto& color_map = Util::ColorMap::instance();
    const bool part_mode = select_mgr.isTypePickable(RenderEntityType::Part);
    const auto& face_hover = color_map.getFaceHoverColor();
    const auto& face_select = color_map.getFaceSelectionColor();

    bool shader_bound = false;

    for(const auto& range : triangle_ranges) {
        bool is_selected = select_mgr.isSelected(range.m_entityKey) ||
                           (part_mode && select_mgr.isPartSelected(range.m_partUid));
        bool is_hovered = select_mgr.isEntityHovered(range.m_entityKey) ||
                          (part_mode && select_mgr.isPartHovered(range.m_partUid));

        if(!is_selected && !is_hovered) {
            continue;
        }

        if(!shader_bound) {
            m_surfaceShader.bind();
            m_surfaceShader.setUniformMatrix4("u_viewMatrix", view);
            m_surfaceShader.setUniformMatrix4("u_projMatrix", projection);
            m_surfaceShader.setUniformVec3("u_cameraPos", camera_pos);
            m_surfaceShader.setUniformFloat("u_alpha", 1.0f);
            shader_bound = true;
        }

        if(is_selected) {
            m_surfaceShader.setUniformVec4("u_highlightColor", face_select.m_r, face_select.m_g,
                                           face_select.m_b, 0.5f);
        } else {
            m_surfaceShader.setUniformVec4("u_highlightColor", face_hover.m_r, face_hover.m_g,
                                           face_hover.m_b, 0.4f);
        }

        f->glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(range.m_indexCount), GL_UNSIGNED_INT,
                          reinterpret_cast<const void*>(
                              static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
    }

    if(shader_bound) {
        m_surfaceShader.release();
    }
}

void HighlightPass::renderHighlightedLines(QOpenGLFunctions* f,
                                           const QMatrix4x4& view,
                                           const QMatrix4x4& projection,
                                           const std::vector<DrawRange>& line_ranges) {
    if(line_ranges.empty()) {
        return;
    }

    const auto& select_mgr = RenderSelectManager::instance();
    const auto& color_map = Util::ColorMap::instance();
    const bool part_mode = select_mgr.isTypePickable(RenderEntityType::Part);
    const bool wire_mode = select_mgr.isTypePickable(RenderEntityType::Wire);
    const auto& ev_hover = color_map.getEdgeVertexHoverColor();
    const auto& ev_select = color_map.getEdgeVertexSelectionColor();

    const float edge_width_hover = Util::RenderStyle::EDGE_LINE_WIDTH_HOVER;
    const float edge_width_selected = Util::RenderStyle::EDGE_LINE_WIDTH_SELECTED;

    bool shader_bound = false;

    for(const auto& range : line_ranges) {
        bool is_selected = select_mgr.isSelected(range.m_entityKey) ||
                           (part_mode && select_mgr.isPartSelected(range.m_partUid)) ||
                           (wire_mode && select_mgr.isEdgeInSelectedWire(range.m_entityKey.m_uid));
        bool is_hovered = select_mgr.isEntityHovered(range.m_entityKey) ||
                          (part_mode && select_mgr.isPartHovered(range.m_partUid)) ||
                          (wire_mode && select_mgr.isEdgeInHoveredWire(range.m_entityKey.m_uid));

        if(!is_selected && !is_hovered) {
            continue;
        }

        if(!shader_bound) {
            m_flatShader.bind();
            m_flatShader.setUniformMatrix4("u_viewMatrix", view);
            m_flatShader.setUniformMatrix4("u_projMatrix", projection);
            shader_bound = true;
        }

        float line_width;
        if(is_selected) {
            m_flatShader.setUniformVec4("u_highlightColor", ev_select.m_r, ev_select.m_g,
                                        ev_select.m_b, 1.0f);
            line_width = edge_width_selected;
        } else {
            m_flatShader.setUniformVec4("u_highlightColor", ev_hover.m_r, ev_hover.m_g,
                                        ev_hover.m_b, 1.0f);
            line_width = edge_width_hover;
        }

        f->glLineWidth(line_width);
        f->glDrawElements(GL_LINES, static_cast<GLsizei>(range.m_indexCount), GL_UNSIGNED_INT,
                          reinterpret_cast<const void*>(
                              static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
    }

    if(shader_bound) {
        f->glLineWidth(1.0f);
        m_flatShader.release();
    }
}

void HighlightPass::renderHighlightedPoints(QOpenGLFunctions* f,
                                            const QMatrix4x4& view,
                                            const QMatrix4x4& projection,
                                            const std::vector<DrawRange>& point_ranges) {
    if(point_ranges.empty()) {
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

    bool shader_bound = false;

    for(const auto& range : point_ranges) {
        bool is_selected = select_mgr.isSelected(range.m_entityKey) ||
                           (part_mode && select_mgr.isPartSelected(range.m_partUid));
        bool is_hovered = select_mgr.isEntityHovered(range.m_entityKey) ||
                          (part_mode && select_mgr.isPartHovered(range.m_partUid));

        if(!is_selected && !is_hovered) {
            continue;
        }

        if(!shader_bound) {
            m_flatShader.bind();
            m_flatShader.setUniformMatrix4("u_viewMatrix", view);
            m_flatShader.setUniformMatrix4("u_projMatrix", projection);
            f->glEnable(GL_PROGRAM_POINT_SIZE);
            shader_bound = true;
        }

        float point_size;
        if(is_selected) {
            m_flatShader.setUniformVec4("u_highlightColor", ev_select.m_r, ev_select.m_g,
                                        ev_select.m_b, 1.0f);
            point_size = vtx_size_base * vtx_scale_selected;
        } else {
            m_flatShader.setUniformVec4("u_highlightColor", ev_hover.m_r, ev_hover.m_g,
                                        ev_hover.m_b, 1.0f);
            point_size = vtx_size_base * vtx_scale_hover;
        }

        m_flatShader.setUniformFloat("u_pointSize", point_size);
        f->glDrawArrays(GL_POINTS, static_cast<GLint>(range.m_vertexOffset),
                        static_cast<GLsizei>(range.m_vertexCount));
    }

    if(shader_bound) {
        m_flatShader.release();
    }
}

} // namespace OpenGeoLab::Render
