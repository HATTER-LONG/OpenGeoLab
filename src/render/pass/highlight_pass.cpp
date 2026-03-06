/**
 * @file highlight_pass.cpp
 * @brief HighlightPass implementation — selected/hovered entity overdraw.
 */

#include "highlight_pass.hpp"

#include "../core/gpu_buffer.hpp"
#include "draw_batch_utils.hpp"
#include "render/render_select_manager.hpp"
#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {

namespace {
// --- Geometry highlight shaders (entity-level uniform-based highlight) ---

const char* geom_surface_vertex_shader = R"(
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

const char* geom_surface_fragment_shader = R"(
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
    fragColor = vec4(finalColor * u_alpha, u_alpha);
}
)";

const char* geom_flat_vertex_shader = R"(
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

const char* geom_flat_fragment_shader = R"(
#version 330 core
in vec4 v_color;
uniform vec4 u_highlightColor;
out vec4 fragColor;
void main() {
    vec3 color = u_highlightColor.rgb;
    fragColor = vec4(color, v_color.a);
}
)";
// --- Mesh highlight shaders (pickId-based per-vertex highlight) ---
// Select pick IDs are stored in a GL_RG32UI texture sampled via texelFetch,
// allowing an unlimited number of selected elements without shader recompilation.
const char* mesh_surface_vertex_shader = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;
layout(location = 3) in uvec2 a_pickId;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;
uniform float u_viewOffset;
out vec3 v_worldPos;
out vec3 v_normal;
out vec4 v_color;
flat out uvec2 v_pickId;
void main() {
    v_worldPos = a_position;
    v_normal = a_normal;
    v_color = a_color;
    v_pickId = a_pickId;
    vec4 viewPos = u_viewMatrix * vec4(a_position, 1.0);
    viewPos.z -= u_viewOffset;
    gl_Position = u_projMatrix * viewPos;
}
)";

const char* mesh_surface_fragment_shader = R"(
#version 330 core
in vec3 v_worldPos;
in vec3 v_normal;
in vec4 v_color;
flat in uvec2 v_pickId;
uniform vec3 u_cameraPos;
uniform float u_alpha;
uniform uvec2 u_hoverPickId;
uniform vec4 u_hoverColor;
uniform usampler2D u_selectPickIds;
uniform int u_selectCount;
uniform vec4 u_selectColor;
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

    bool isSelected = false;
    for(int i = 0; i < u_selectCount; i++) {
        if(v_pickId == texelFetch(u_selectPickIds, ivec2(i, 0), 0).xy) {
            isSelected = true;
            break;
        }
    }
    bool isHovered = (u_hoverPickId != uvec2(0, 0) && v_pickId == u_hoverPickId);

    if(!isSelected && !isHovered) {
        discard;
    }
    vec3 finalColor = isSelected ? mix(litColor, u_selectColor.rgb, u_selectColor.a)
                                 : mix(litColor, u_hoverColor.rgb, u_hoverColor.a);
    fragColor = vec4(finalColor * u_alpha, u_alpha);
}
)";

const char* mesh_flat_vertex_shader = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 2) in vec4 a_color;
layout(location = 3) in uvec2 a_pickId;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;
uniform float u_pointSize;
uniform float u_viewOffset;
out vec4 v_color;
flat out uvec2 v_pickId;
void main() {
    v_color = a_color;
    v_pickId = a_pickId;
    gl_PointSize = u_pointSize;
    vec4 viewPos = u_viewMatrix * vec4(a_position, 1.0);
    viewPos.z -= u_viewOffset;
    gl_Position = u_projMatrix * viewPos;
}
)";

const char* mesh_flat_fragment_shader = R"(
#version 330 core
in vec4 v_color;
flat in uvec2 v_pickId;
uniform uvec2 u_hoverPickId;
uniform vec4 u_hoverColor;
uniform usampler2D u_selectPickIds;
uniform int u_selectCount;
uniform vec4 u_selectColor;
out vec4 fragColor;
void main() {
    bool isSelected = false;
    for(int i = 0; i < u_selectCount; i++) {
        if(v_pickId == texelFetch(u_selectPickIds, ivec2(i, 0), 0).xy) {
            isSelected = true;
            break;
        }
    }
    bool isHovered = (u_hoverPickId != uvec2(0, 0) && v_pickId == u_hoverPickId);

    if(!isSelected && !isHovered) {
        discard;
    }
    vec3 color = isSelected ? u_selectColor.rgb : u_hoverColor.rgb;
    fragColor = vec4(color, v_color.a);
}
)";

[[nodiscard]] constexpr bool hasMode(RenderDisplayModeMask value, RenderDisplayModeMask flag) {
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

/// Split a uint64_t encoded pick ID into two uint32 components for shader uvec2.
void toUvec2(uint64_t encoded, uint32_t& lo, uint32_t& hi) {
    lo = static_cast<uint32_t>(encoded & 0xFFFFFFFFu);
    hi = static_cast<uint32_t>((encoded >> 32u) & 0xFFFFFFFFu);
}
constexpr float K_MESH_LINE_VIEW_OFFSET = -0.05f;
} // namespace

bool HighlightPass::onInitialize() {
    if(!m_surfaceShader.compile(geom_surface_vertex_shader, geom_surface_fragment_shader)) {
        LOG_ERROR("HighlightPass: Failed to compile geometry surface shader");
        return false;
    }

    if(!m_flatShader.compile(geom_flat_vertex_shader, geom_flat_fragment_shader)) {
        LOG_ERROR("HighlightPass: Failed to compile geometry flat shader");
        return false;
    }

    if(!m_meshSurfaceShader.compile(mesh_surface_vertex_shader, mesh_surface_fragment_shader)) {
        LOG_ERROR("HighlightPass: Failed to compile mesh surface shader");
        return false;
    }

    if(!m_meshFlatShader.compile(mesh_flat_vertex_shader, mesh_flat_fragment_shader)) {
        LOG_ERROR("HighlightPass: Failed to compile mesh flat shader");
        return false;
    }

    // Allocate pick-ID lookup textures (one per mesh topology: surface / line / node).
    // Each texture uses GL_RG32UI; width = selection count, height = 1.
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glGenTextures(static_cast<GLsizei>(m_selectPickTex.size()), m_selectPickTex.data());
    const uint32_t dummy[2] = {0u, 0u};
    for(GLuint tex : m_selectPickTex) {
        f->glBindTexture(GL_TEXTURE_2D, tex);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, 1, 1, 0, GL_RG_INTEGER, GL_UNSIGNED_INT,
                        dummy);
    }
    f->glBindTexture(GL_TEXTURE_2D, 0);
    LOG_DEBUG("HighlightPass: Initialized");
    return true;
}

void HighlightPass::onCleanup() {
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glDeleteTextures(static_cast<GLsizei>(m_selectPickTex.size()), m_selectPickTex.data());
    m_selectPickTex.fill(0);
    LOG_DEBUG("HighlightPass: Cleaned up");
}
void HighlightPass::render(RenderPassContext& ctx_data) {
    if(!isInitialized()) {
        return;
    }
    renderGeometry(ctx_data);
    renderMesh(ctx_data);
}
void HighlightPass::renderGeometry(const RenderPassContext& ctx) { // NOLINT
    if(!isInitialized() || !ctx.m_geometry.hasGeometry()) {
        return;
    }

    const auto& select_mgr = RenderSelectManager::instance();
    const auto& color_map = Util::ColorMap::instance();

    const auto& ev_hover_color = color_map.getEdgeVertexHoverColor();
    const auto& ev_select_color = color_map.getEdgeVertexSelectionColor();
    const auto& face_hover_color = color_map.getFaceHoverColor();
    const auto& face_select_color = color_map.getFaceSelectionColor();

    const auto& params = ctx.m_params;
    const auto& triangle_ranges = ctx.m_geometry.m_triangleRanges;
    const auto& line_ranges = ctx.m_geometry.m_lineRanges;
    const auto& point_ranges = ctx.m_geometry.m_pointRanges;

    const bool part_mode = (select_mgr.isTypePickable(RenderEntityType::Part) ||
                            select_mgr.hasSelectionsOfType(RenderEntityType::Part));
    const bool solid_mode = (select_mgr.isTypePickable(RenderEntityType::Solid) ||
                             select_mgr.hasSelectionsOfType(RenderEntityType::Solid));
    const bool wire_mode = (select_mgr.isTypePickable(RenderEntityType::Wire) ||
                            select_mgr.hasSelectionsOfType(RenderEntityType::Wire));

    auto& geom_buf = ctx.m_geometry.m_buffer;
    const float surface_alpha = params.m_xRayMode ? 0.25f : 1.0f;

    // Helper: determine highlight state for a range, returning color+alpha or nothing.
    // Returns false when the range should be skipped (not highlighted).
    struct HighlightInfo {
        RenderColor m_color;
        float m_alpha{1.0f};
        bool m_selected; ///< true = selected state, false = hovered state
    };
    auto resolve_highlight = [&select_mgr, &part_mode, &solid_mode, &wire_mode](
                                 const DrawRange& r, bool check_wire, const RenderColor& sel_color,
                                 float sel_alpha, const RenderColor& hov_color, float hov_alpha,
                                 HighlightInfo& out) -> bool {
        bool sel = select_mgr.isSelected(r.m_entityKey) ||
                   (part_mode && select_mgr.isPartSelected(r.m_partUid)) ||
                   (solid_mode && select_mgr.isSolidSelected(r.m_solidUid));
        bool hov = select_mgr.isEntityHovered(r.m_entityKey) ||
                   (part_mode && select_mgr.isPartHovered(r.m_partUid)) ||
                   (solid_mode && select_mgr.isSolidHovered(r.m_solidUid));
        if(check_wire && wire_mode) {
            sel |= (wire_mode && select_mgr.isEdgeInSelectedWire(r.m_entityKey.m_uid));
            hov |= (wire_mode && select_mgr.isEdgeInHoveredWire(r.m_entityKey.m_uid));
        }
        if(sel) {
            out = {sel_color, sel_alpha, true};
            return true;
        }
        if(hov) {
            out = {hov_color, hov_alpha, false};
            return true;
        }
        return false;
    };

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LEQUAL);

    if(params.m_xRayMode) {
        f->glEnable(GL_BLEND);
        f->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    geom_buf.bindForDraw();
    // --- Highlighted surface triangles ---
    if(!triangle_ranges.empty()) {
        f->glEnable(GL_POLYGON_OFFSET_FILL);
        f->glPolygonOffset(1.0f, 1.0f);

        m_surfaceShader.bind();
        m_surfaceShader.setUniformMatrix4("u_viewMatrix", params.m_viewMatrix);
        m_surfaceShader.setUniformMatrix4("u_projMatrix", params.m_projMatrix);
        m_surfaceShader.setUniformVec3("u_cameraPos", params.m_cameraPos);
        m_surfaceShader.setUniformFloat("u_alpha", surface_alpha);

        for(const auto& range : triangle_ranges) {
            HighlightInfo hi;
            if(!resolve_highlight(range, false, face_select_color, 0.5f, face_hover_color, 0.4f,
                                  hi)) {
                continue;
            }
            m_surfaceShader.setUniformVec4("u_highlightColor", hi.m_color.m_r, hi.m_color.m_g,
                                           hi.m_color.m_b, hi.m_alpha);
            f->glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(range.m_indexCount),
                              GL_UNSIGNED_INT,
                              reinterpret_cast<const void*>(
                                  static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
        }
        m_surfaceShader.release();
        f->glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // --- Highlighted edges and points — share a single flat shader bind ---
    if(!line_ranges.empty() || !point_ranges.empty()) {
        m_flatShader.bind();
        m_flatShader.setUniformMatrix4("u_viewMatrix", params.m_viewMatrix);
        m_flatShader.setUniformMatrix4("u_projMatrix", params.m_projMatrix);

        for(const auto& range : line_ranges) {
            HighlightInfo hi;
            if(!resolve_highlight(range, true, ev_select_color, 1.0f, ev_hover_color, 1.0f, hi)) {
                continue;
            }
            const float line_width = hi.m_selected ? Util::RenderStyle::EDGE_LINE_WIDTH_SELECTED
                                                   : Util::RenderStyle::EDGE_LINE_WIDTH_HOVER;
            m_flatShader.setUniformVec4("u_highlightColor", hi.m_color.m_r, hi.m_color.m_g,
                                        hi.m_color.m_b, hi.m_alpha);
            f->glLineWidth(line_width);
            f->glDrawElements(GL_LINES, static_cast<GLsizei>(range.m_indexCount), GL_UNSIGNED_INT,
                              reinterpret_cast<const void*>(
                                  static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
        }
        f->glLineWidth(1.0f);
        if(!point_ranges.empty()) {
            f->glEnable(GL_PROGRAM_POINT_SIZE);
            const float vtx_base = Util::RenderStyle::VERTEX_POINT_SIZE;
            for(const auto& range : point_ranges) {
                HighlightInfo hi;
                if(!resolve_highlight(range, false, ev_select_color, 1.0f, ev_hover_color, 1.0f,
                                      hi)) {
                    continue;
                }
                const float scale = hi.m_selected ? Util::RenderStyle::VERTEX_SCALE_SELECTED
                                                  : Util::RenderStyle::VERTEX_SCALE_HOVER;
                m_flatShader.setUniformVec4("u_highlightColor", hi.m_color.m_r, hi.m_color.m_g,
                                            hi.m_color.m_b, hi.m_alpha);
                m_flatShader.setUniformFloat("u_pointSize", vtx_base * scale);
                f->glDrawArrays(GL_POINTS, static_cast<GLint>(range.m_vertexOffset),
                                static_cast<GLsizei>(range.m_vertexCount));
            }
            f->glDisable(GL_PROGRAM_POINT_SIZE);
        }

        m_flatShader.release();
    }
    geom_buf.unbind();

    if(params.m_xRayMode) {
        f->glDisable(GL_BLEND);
    }
    f->glDepthFunc(GL_LESS);
}

void HighlightPass::renderMesh(const RenderPassContext& ctx) { // NOLINT
    if(!isInitialized() || !ctx.m_mesh.hasMesh()) {
        return;
    }

    const auto& select_mgr = RenderSelectManager::instance();
    const auto& color_map = Util::ColorMap::instance();

    // Build hover pick ID.
    uint32_t hover_lo = 0, hover_hi = 0;
    {
        const auto& hovered = select_mgr.hoveredEntity();
        if(hovered.m_type != RenderEntityType::None && isMeshDomain(hovered.m_type)) {
            toUvec2(PickId::encode(hovered.m_type, hovered.m_uid), hover_lo, hover_hi);
        }
    }
    // Build per-topology pick-ID lists (lo,hi interleaved, no size limit).
    // Topology index: 0 = surface, 1 = line, 2 = node.
    std::array<std::vector<uint32_t>, 3> picks;
    std::array<std::unordered_set<uint64_t>, 3> seen;

    for(const auto& sel : select_mgr.selections()) {
        if(!isMeshDomain(sel.m_type)) {
            continue;
        }

        const int idx = (sel.m_type == RenderEntityType::MeshNode
                             ? 2
                             : (sel.m_type == RenderEntityType::MeshLine ? 1 : 0));
        const uint64_t encoded = PickId::encode(sel.m_type, sel.m_uid);
        if(!seen[idx].insert(encoded).second) {
            continue;
        }
        uint32_t lo, hi;
        toUvec2(encoded, lo, hi);
        picks[idx].push_back(lo);
        picks[idx].push_back(hi);
    }
    const int surface_count = static_cast<int>(picks[0].size() / 2);
    const int line_count = static_cast<int>(picks[1].size() / 2);
    const int node_count = static_cast<int>(picks[2].size() / 2);
    const bool any_hover = (hover_lo != 0 || hover_hi != 0);

    if(!any_hover && surface_count == 0 && line_count == 0 && node_count == 0) {
        return;
    }

    // Upload per-topology pick-ID textures (GL_RG32UI; width = count, height = 1).
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    const uint32_t dummy[2] = {0u, 0u};
    for(int i = 0; i < 3; ++i) {
        f->glBindTexture(GL_TEXTURE_2D, m_selectPickTex[i]);
        if(picks[i].empty()) {
            f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, 1, 1, 0, GL_RG_INTEGER, GL_UNSIGNED_INT,
                            dummy);
        } else {
            f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, static_cast<GLsizei>(picks[i].size() / 2),
                            1, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, picks[i].data());
        }
    }
    f->glBindTexture(GL_TEXTURE_2D, 0);
    auto& params = ctx.m_params;
    const auto& mesh_batches = ctx.m_mesh.m_batches;
    auto& mesh_buffer = ctx.m_mesh.m_buffer;

    const auto& face_hover_color = color_map.getFaceHoverColor();
    const auto& face_select_color = color_map.getFaceSelectionColor();
    const auto& ev_hover_color = color_map.getEdgeVertexHoverColor();
    const auto& ev_select_color = color_map.getEdgeVertexSelectionColor();

    const float surface_alpha = params.m_xRayMode ? 0.25f : 1.0f;

    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LEQUAL);

    if(params.m_xRayMode) {
        f->glEnable(GL_BLEND);
        f->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    QOpenGLContext* ctx_gl = QOpenGLContext::currentContext();
    mesh_buffer.bindForDraw();

    // Bind common mesh-pick shader uniforms and the per-topology pick-ID texture.
    auto bind_mesh_pick_uniforms = [&](ShaderProgram& shader, int topo_idx, int select_count) {
        shader.setUniformMatrix4("u_viewMatrix", params.m_viewMatrix);
        shader.setUniformMatrix4("u_projMatrix", params.m_projMatrix);
        shader.setUniformFloat("u_viewOffset", 0.0f);
        shader.setUniformUvec2("u_hoverPickId", hover_lo, hover_hi);
        shader.setUniformInt("u_selectCount", select_count);
        f->glActiveTexture(GL_TEXTURE0);
        f->glBindTexture(GL_TEXTURE_2D, m_selectPickTex[topo_idx]);
        shader.setUniformInt("u_selectPickIds", 0);
    };
    auto& mesh_triangle_ranges = ctx.m_mesh.m_triangleRanges;
    auto& mesh_display_mode = ctx.m_mesh.m_displayMode;
    // --- Mesh surface highlight ---
    if(!mesh_triangle_ranges.empty() &&
       (hasMode(mesh_display_mode, RenderDisplayModeMask::Surface) || surface_count > 0 ||
        any_hover)) {
        f->glEnable(GL_POLYGON_OFFSET_FILL);
        f->glPolygonOffset(1.0f, 1.0f);

        m_meshSurfaceShader.bind();
        bind_mesh_pick_uniforms(m_meshSurfaceShader, 0, surface_count);
        m_meshSurfaceShader.setUniformVec3("u_cameraPos", params.m_cameraPos);
        m_meshSurfaceShader.setUniformFloat("u_alpha", surface_alpha);
        m_meshSurfaceShader.setUniformVec4("u_hoverColor", face_hover_color.m_r,
                                           face_hover_color.m_g, face_hover_color.m_b, 0.4f);
        m_meshSurfaceShader.setUniformVec4("u_selectColor", face_select_color.m_r,
                                           face_select_color.m_g, face_select_color.m_b, 0.5f);

        PassUtil::multiDrawArrays(ctx_gl, f, GL_TRIANGLES, mesh_batches.m_triangles.m_all);

        m_meshSurfaceShader.release();
        f->glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // --- Mesh wireframe highlight ---
    if(hasMode(mesh_display_mode, RenderDisplayModeMask::Wireframe) &&
       !mesh_batches.m_lines.m_all.empty()) {
        m_meshFlatShader.bind();
        bind_mesh_pick_uniforms(m_meshFlatShader, 1, line_count);
        // Slight view-space offset for mesh wire/highlight lines to avoid z-fighting
        m_meshFlatShader.setUniformFloat("u_viewOffset", K_MESH_LINE_VIEW_OFFSET);
        m_meshFlatShader.setUniformVec4("u_hoverColor", ev_hover_color.m_r, ev_hover_color.m_g,
                                        ev_hover_color.m_b, 1.0f);
        m_meshFlatShader.setUniformVec4("u_selectColor", ev_select_color.m_r, ev_select_color.m_g,
                                        ev_select_color.m_b, 1.0f);

        f->glLineWidth(1.0f);
        PassUtil::multiDrawArrays(ctx_gl, f, GL_LINES, mesh_batches.m_lines.m_all);

        m_meshFlatShader.release();
    }
    // --- Mesh node points highlight ---
    if(hasMode(mesh_display_mode, RenderDisplayModeMask::Points) &&
       !mesh_batches.m_points.m_all.empty()) {
        m_meshFlatShader.bind();
        bind_mesh_pick_uniforms(m_meshFlatShader, 2, node_count);
        m_meshFlatShader.setUniformFloat("u_pointSize", 3.0f);
        // Offset mesh highlight points slightly to avoid z-fighting with surfaces
        m_meshFlatShader.setUniformFloat("u_viewOffset", K_MESH_LINE_VIEW_OFFSET);
        m_meshFlatShader.setUniformVec4("u_hoverColor", ev_hover_color.m_r, ev_hover_color.m_g,
                                        ev_hover_color.m_b, 1.0f);
        m_meshFlatShader.setUniformVec4("u_selectColor", ev_select_color.m_r, ev_select_color.m_g,
                                        ev_select_color.m_b, 1.0f);

        f->glEnable(GL_PROGRAM_POINT_SIZE);
        PassUtil::multiDrawArrays(ctx_gl, f, GL_POINTS, mesh_batches.m_points.m_all);
        m_meshFlatShader.setUniformFloat("u_viewOffset", 0.0f);

        m_meshFlatShader.release();
    }
    f->glBindTexture(GL_TEXTURE_2D, 0);
    mesh_buffer.unbind();

    if(params.m_xRayMode) {
        f->glDisable(GL_BLEND);
    }
    f->glDepthFunc(GL_LESS);
}
} // namespace OpenGeoLab::Render