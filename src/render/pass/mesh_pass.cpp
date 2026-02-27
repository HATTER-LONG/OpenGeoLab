/**
 * @file mesh_pass.cpp
 * @brief MeshPass implementation — separate surface, wireframe, and point
 *        rendering with X-ray mode and shader-based hover/selection highlight.
 */

#include "mesh_pass.hpp"

#include "render/render_select_manager.hpp"
#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {

// =============================================================================
// Shader sources
// =============================================================================

namespace {

const char* SURFACE_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;
layout(location = 3) in uvec2 a_pickId;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;
out vec3 v_worldPos;
out vec3 v_normal;
out vec4 v_color;
flat out uvec2 v_pickId;
void main() {
    v_worldPos = a_position;
    v_normal = a_normal;
    v_color = a_color;
    v_pickId = a_pickId;
    gl_Position = u_projMatrix * u_viewMatrix * vec4(a_position, 1.0);
}
)";

const char* SURFACE_FRAGMENT_SHADER = R"(
#version 330 core
in vec3 v_worldPos;
in vec3 v_normal;
in vec4 v_color;
flat in uvec2 v_pickId;
uniform vec3 u_cameraPos;
uniform vec4 u_highlightColor;
uniform float u_alpha;
uniform uvec2 u_hoverPickId;
uniform vec4 u_hoverColor;
uniform uvec2 u_selectPickIds[32];
uniform int u_selectCount;
uniform vec4 u_selectColor;
uniform int u_highlightOnly;
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

    // Check selection first (higher priority than hover)
    bool isSelected = false;
    for(int i = 0; i < u_selectCount; i++) {
        if(v_pickId == u_selectPickIds[i]) {
            isSelected = true;
            break;
        }
    }
    if(isSelected) {
        finalColor = mix(litColor, u_selectColor.rgb, u_selectColor.a);
    } else if(u_hoverPickId != uvec2(0, 0) && v_pickId == u_hoverPickId) {
        finalColor = mix(litColor, u_hoverColor.rgb, u_hoverColor.a);
    }

    // Premultiply alpha for correct Qt Quick scene-graph compositing.
    // When u_highlightOnly is set, discard non-highlighted fragments so that
    // only hovered/selected mesh elements are visible in wireframe-only mode.
    bool isHighlighted = isSelected || (u_hoverPickId != uvec2(0, 0) && v_pickId == u_hoverPickId);
    if(u_highlightOnly == 1 && !isHighlighted) {
        discard;
    }
    fragColor = vec4(finalColor * u_alpha, u_alpha);
}
)";

const char* FLAT_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 2) in vec4 a_color;
layout(location = 3) in uvec2 a_pickId;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;
uniform float u_pointSize;
out vec4 v_color;
flat out uvec2 v_pickId;
void main() {
    v_color = a_color;
    v_pickId = a_pickId;
    gl_PointSize = u_pointSize;
    gl_Position = u_projMatrix * u_viewMatrix * vec4(a_position, 1.0);
}
)";

const char* FLAT_FRAGMENT_SHADER = R"(
#version 330 core
in vec4 v_color;
flat in uvec2 v_pickId;
uniform vec4 u_highlightColor;
uniform uvec2 u_hoverPickId;
uniform vec4 u_hoverColor;
uniform uvec2 u_selectPickIds[32];
uniform int u_selectCount;
uniform vec4 u_selectColor;
out vec4 fragColor;
void main() {
    vec3 color = v_color.rgb;

    // Check selection first
    bool isSelected = false;
    for(int i = 0; i < u_selectCount; i++) {
        if(v_pickId == u_selectPickIds[i]) {
            isSelected = true;
            break;
        }
    }
    if(isSelected) {
        color = u_selectColor.rgb;
    } else if(u_hoverPickId != uvec2(0, 0) && v_pickId == u_hoverPickId) {
        color = u_hoverColor.rgb;
    } else if(u_highlightColor.a > 0.0) {
        color = u_highlightColor.rgb;
    }

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

} // anonymous namespace

// =============================================================================
// Lifecycle
// =============================================================================

void MeshPass::initialize() {
    if(m_initialized) {
        return;
    }

    if(!m_surfaceShader.compile(SURFACE_VERTEX_SHADER, SURFACE_FRAGMENT_SHADER)) {
        LOG_ERROR("MeshPass: Failed to compile surface shader");
        return;
    }

    if(!m_flatShader.compile(FLAT_VERTEX_SHADER, FLAT_FRAGMENT_SHADER)) {
        LOG_ERROR("MeshPass: Failed to compile flat shader");
        return;
    }

    m_gpuBuffer.initialize();
    m_initialized = true;
    LOG_DEBUG("MeshPass: Initialized");
}

void MeshPass::cleanup() {
    m_totalVertexCount = 0;
    m_surfaceVertexCount = 0;
    m_wireframeVertexCount = 0;
    m_nodeVertexCount = 0;
    m_gpuBuffer.cleanup();
    m_initialized = false;
    LOG_DEBUG("MeshPass: Cleaned up");
}

// =============================================================================
// Buffer update
// =============================================================================

void MeshPass::updateBuffers(const RenderData& data) {
    auto passIt = data.m_passData.find(RenderPassType::Mesh);
    if(passIt == data.m_passData.end()) {
        m_totalVertexCount = 0;
        m_surfaceVertexCount = 0;
        m_wireframeVertexCount = 0;
        m_nodeVertexCount = 0;
        return;
    }

    const RenderPassData& passData = passIt->second;

    if(passData.m_dirty) {
        m_gpuBuffer.upload(passData);
        LOG_DEBUG("MeshPass: Uploaded {} vertices", passData.m_vertices.size());
    }

    m_totalVertexCount = static_cast<uint32_t>(passData.m_vertices.size());

    // Extract per-topology vertex counts from the mesh root's DrawRanges
    m_surfaceVertexCount = 0;
    m_wireframeVertexCount = 0;
    m_nodeVertexCount = 0;

    for(const auto& root : data.m_roots) {
        if(!isMeshDomain(root.m_key.m_type)) {
            continue;
        }
        auto it = root.m_drawRanges.find(RenderPassType::Mesh);
        if(it == root.m_drawRanges.end()) {
            continue;
        }
        for(const auto& range : it->second) {
            switch(range.m_topology) {
            case PrimitiveTopology::Triangles:
                m_surfaceVertexCount += range.m_vertexCount;
                break;
            case PrimitiveTopology::Lines:
                m_wireframeVertexCount += range.m_vertexCount;
                break;
            case PrimitiveTopology::Points:
                m_nodeVertexCount += range.m_vertexCount;
                break;
            }
        }
    }
}

// =============================================================================
// Rendering
// =============================================================================

void MeshPass::render(const QMatrix4x4& view,
                      const QMatrix4x4& projection,
                      const QVector3D& camera_pos,
                      bool x_ray_mode) {
    if(!m_initialized || m_totalVertexCount == 0) {
        return;
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    const auto& selectMgr = RenderSelectManager::instance();
    const auto& colorMap = Util::ColorMap::instance();

    // Build hover pick ID for shader
    uint32_t hoverLo = 0, hoverHi = 0;
    {
        const auto& hovered = selectMgr.hoveredEntity();
        if(hovered.m_type != RenderEntityType::None && isMeshDomain(hovered.m_type)) {
            const uint64_t encoded = PickId::encode(hovered.m_type, hovered.m_uid);
            toUvec2(encoded, hoverLo, hoverHi);
        }
    }

    // Build selected pick IDs for shader (limited to 32)
    constexpr int MAX_SELECTED = 32;
    uint32_t selectLo[MAX_SELECTED] = {};
    uint32_t selectHi[MAX_SELECTED] = {};
    int selectCount = 0;
    {
        const auto selections = selectMgr.selections();
        for(const auto& sel : selections) {
            if(selectCount >= MAX_SELECTED) {
                break;
            }
            if(isMeshDomain(sel.m_type)) {
                const uint64_t encoded = PickId::encode(sel.m_type, sel.m_uid);
                toUvec2(encoded, selectLo[selectCount], selectHi[selectCount]);
                ++selectCount;
            }
        }
    }

    // Highlight colors
    const auto& evHover = colorMap.getEdgeVertexHoverColor();
    const auto& evSelect = colorMap.getEdgeVertexSelectionColor();
    const auto& faceHover = colorMap.getFaceHoverColor();
    const auto& faceSelect = colorMap.getFaceSelectionColor();

    const float surfaceAlpha = x_ray_mode ? 0.25f : 1.0f;

    m_gpuBuffer.bindForDraw();
    f->glEnable(GL_DEPTH_TEST);

    // --- Surface pass (triangles) ---
    // Draw surfaces when Surface display mode is active, or when in highlight-only
    // mode (to show hovered/selected mesh elements even in wireframe-only mode).
    const bool surfaceMode = hasMode(m_displayMode, RenderDisplayModeMask::Surface);
    const bool hasMeshHighlight = (hoverLo != 0 || hoverHi != 0) || selectCount > 0;
    const bool drawSurface = (surfaceMode || hasMeshHighlight) && m_surfaceVertexCount > 0;
    if(drawSurface) {
        // Enable blending for X-ray mode
        if(x_ray_mode) {
            f->glEnable(GL_BLEND);
            // Use premultiplied alpha blending: src already has color * alpha
            f->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            f->glDepthMask(GL_FALSE);
        }

        // Push surfaces slightly back in depth so coplanar wireframe edges
        // on visible faces pass the depth test, while edges behind the model
        // remain properly occluded by front-face surfaces.
        f->glEnable(GL_POLYGON_OFFSET_FILL);
        f->glPolygonOffset(1.0f, 1.0f);

        m_surfaceShader.bind();
        m_surfaceShader.setUniformMatrix4("u_viewMatrix", view);
        m_surfaceShader.setUniformMatrix4("u_projMatrix", projection);
        m_surfaceShader.setUniformVec3("u_cameraPos", camera_pos);
        m_surfaceShader.setUniformVec4("u_highlightColor", 0.0f, 0.0f, 0.0f, 0.0f);
        m_surfaceShader.setUniformFloat("u_alpha", surfaceAlpha);
        // When surface display is off, only draw highlighted (hovered/selected) fragments
        m_surfaceShader.setUniformInt("u_highlightOnly", surfaceMode ? 0 : 1);

        // Hover/selection uniforms
        m_surfaceShader.setUniformUvec2("u_hoverPickId", hoverLo, hoverHi);
        m_surfaceShader.setUniformVec4("u_hoverColor", faceHover.m_r, faceHover.m_g, faceHover.m_b,
                                       0.4f);
        m_surfaceShader.setUniformInt("u_selectCount", selectCount);
        m_surfaceShader.setUniformVec4("u_selectColor", faceSelect.m_r, faceSelect.m_g,
                                       faceSelect.m_b, 0.5f);
        for(int i = 0; i < selectCount; ++i) {
            const std::string name = "u_selectPickIds[" + std::to_string(i) + "]";
            m_surfaceShader.setUniformUvec2(name.c_str(), selectLo[i], selectHi[i]);
        }

        f->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_surfaceVertexCount));

        m_surfaceShader.release();

        f->glDisable(GL_POLYGON_OFFSET_FILL);

        // Restore state after X-ray blending
        if(x_ray_mode) {
            f->glDepthMask(GL_TRUE);
            f->glDisable(GL_BLEND);
        }
    }

    // --- Wireframe pass (lines) ---
    if(hasMode(m_displayMode, RenderDisplayModeMask::Wireframe) && m_wireframeVertexCount > 0) {
        m_flatShader.bind();
        m_flatShader.setUniformMatrix4("u_viewMatrix", view);
        m_flatShader.setUniformMatrix4("u_projMatrix", projection);
        m_flatShader.setUniformVec4("u_highlightColor", 0.0f, 0.0f, 0.0f, 0.0f);

        // Hover/selection uniforms
        m_flatShader.setUniformUvec2("u_hoverPickId", hoverLo, hoverHi);
        m_flatShader.setUniformVec4("u_hoverColor", evHover.m_r, evHover.m_g, evHover.m_b, 1.0f);
        m_flatShader.setUniformInt("u_selectCount", selectCount);
        m_flatShader.setUniformVec4("u_selectColor", evSelect.m_r, evSelect.m_g, evSelect.m_b,
                                    1.0f);
        for(int i = 0; i < selectCount; ++i) {
            const std::string name = "u_selectPickIds[" + std::to_string(i) + "]";
            m_flatShader.setUniformUvec2(name.c_str(), selectLo[i], selectHi[i]);
        }

        // Slight depth offset to prevent z-fighting with surface triangles.
        // GL_POLYGON_OFFSET_LINE only affects polygon-mode lines, not GL_LINES
        // primitives. Depth separation is handled by GL_POLYGON_OFFSET_FILL
        // applied during the surface pass above.

        f->glLineWidth(1.0f);
        f->glDrawArrays(GL_LINES, static_cast<GLint>(m_surfaceVertexCount),
                        static_cast<GLsizei>(m_wireframeVertexCount));

        m_flatShader.release();
    }

    // --- Node points pass ---
    if(hasMode(m_displayMode, RenderDisplayModeMask::Points) && m_nodeVertexCount > 0) {
        m_flatShader.bind();
        m_flatShader.setUniformMatrix4("u_viewMatrix", view);
        m_flatShader.setUniformMatrix4("u_projMatrix", projection);
        m_flatShader.setUniformVec4("u_highlightColor", 0.0f, 0.0f, 0.0f, 0.0f);
        m_flatShader.setUniformFloat("u_pointSize", 3.0f);

        // Hover/selection uniforms
        m_flatShader.setUniformUvec2("u_hoverPickId", hoverLo, hoverHi);
        m_flatShader.setUniformVec4("u_hoverColor", evHover.m_r, evHover.m_g, evHover.m_b, 1.0f);
        m_flatShader.setUniformInt("u_selectCount", selectCount);
        m_flatShader.setUniformVec4("u_selectColor", evSelect.m_r, evSelect.m_g, evSelect.m_b,
                                    1.0f);
        for(int i = 0; i < selectCount; ++i) {
            const std::string name = "u_selectPickIds[" + std::to_string(i) + "]";
            m_flatShader.setUniformUvec2(name.c_str(), selectLo[i], selectHi[i]);
        }

        f->glEnable(GL_PROGRAM_POINT_SIZE);
        f->glDrawArrays(GL_POINTS,
                        static_cast<GLint>(m_surfaceVertexCount + m_wireframeVertexCount),
                        static_cast<GLsizei>(m_nodeVertexCount));

        m_flatShader.release();
    }

    m_gpuBuffer.unbind();
}

} // namespace OpenGeoLab::Render
