/**
 * @file highlight_pass.cpp
 * @brief HighlightPass implementation — selected/hovered entity overdraw.
 */

#include "highlight_pass.hpp"

#include "render/core/gpu_buffer.hpp"
#include "render/render_select_manager.hpp"
#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {

namespace {

// --- Geometry highlight shaders (entity-level uniform-based highlight) ---

const char* GEOM_SURFACE_VERTEX_SHADER = R"(
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

const char* GEOM_SURFACE_FRAGMENT_SHADER = R"(
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

const char* GEOM_FLAT_VERTEX_SHADER = R"(
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

const char* GEOM_FLAT_FRAGMENT_SHADER = R"(
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

const char* MESH_SURFACE_VERTEX_SHADER = R"(
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

const char* MESH_SURFACE_FRAGMENT_SHADER = R"(
#version 330 core
in vec3 v_worldPos;
in vec3 v_normal;
in vec4 v_color;
flat in uvec2 v_pickId;
uniform vec3 u_cameraPos;
uniform float u_alpha;
uniform uvec2 u_hoverPickId;
uniform vec4 u_hoverColor;
uniform uvec2 u_selectPickIds[32];
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
    vec3 finalColor = litColor;

    bool isSelected = false;
    for(int i = 0; i < u_selectCount; i++) {
        if(v_pickId == u_selectPickIds[i]) {
            isSelected = true;
            break;
        }
    }
    bool isHovered = (u_hoverPickId != uvec2(0, 0) && v_pickId == u_hoverPickId);

    if(!isSelected && !isHovered) {
        discard;
    }
    if(isSelected) {
        finalColor = mix(litColor, u_selectColor.rgb, u_selectColor.a);
    } else if(isHovered) {
        finalColor = mix(litColor, u_hoverColor.rgb, u_hoverColor.a);
    }
    fragColor = vec4(finalColor * u_alpha, u_alpha);
}
)";

const char* MESH_FLAT_VERTEX_SHADER = R"(
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

const char* MESH_FLAT_FRAGMENT_SHADER = R"(
#version 330 core
in vec4 v_color;
flat in uvec2 v_pickId;
uniform uvec2 u_hoverPickId;
uniform vec4 u_hoverColor;
uniform uvec2 u_selectPickIds[32];
uniform int u_selectCount;
uniform vec4 u_selectColor;
out vec4 fragColor;
void main() {
    bool isSelected = false;
    for(int i = 0; i < u_selectCount; i++) {
        if(v_pickId == u_selectPickIds[i]) {
            isSelected = true;
            break;
        }
    }
    bool isHovered = (u_hoverPickId != uvec2(0, 0) && v_pickId == u_hoverPickId);

    if(!isSelected && !isHovered) {
        discard;
    }
    vec3 color = v_color.rgb;
    if(isSelected) {
        color = u_selectColor.rgb;
    } else if(isHovered) {
        color = u_hoverColor.rgb;
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

bool HighlightPass::onInitialize() {
    if(!m_surfaceShader.compile(GEOM_SURFACE_VERTEX_SHADER, GEOM_SURFACE_FRAGMENT_SHADER)) {
        LOG_ERROR("HighlightPass: Failed to compile geometry surface shader");
        return false;
    }

    if(!m_flatShader.compile(GEOM_FLAT_VERTEX_SHADER, GEOM_FLAT_FRAGMENT_SHADER)) {
        LOG_ERROR("HighlightPass: Failed to compile geometry flat shader");
        return false;
    }

    if(!m_meshSurfaceShader.compile(MESH_SURFACE_VERTEX_SHADER, MESH_SURFACE_FRAGMENT_SHADER)) {
        LOG_ERROR("HighlightPass: Failed to compile mesh surface shader");
        return false;
    }

    if(!m_meshFlatShader.compile(MESH_FLAT_VERTEX_SHADER, MESH_FLAT_FRAGMENT_SHADER)) {
        LOG_ERROR("HighlightPass: Failed to compile mesh flat shader");
        return false;
    }

    LOG_DEBUG("HighlightPass: Initialized");
    return true;
}

void HighlightPass::onCleanup() { LOG_DEBUG("HighlightPass: Cleaned up"); }

void HighlightPass::renderGeometry(const PassRenderParams& params,
                                   GpuBuffer& geomBuffer,
                                   const std::vector<DrawRangeEx>& triangleRanges,
                                   const std::vector<DrawRangeEx>& lineRanges,
                                   const std::vector<DrawRangeEx>& pointRanges) {
    if(!isInitialized() || geomBuffer.vertexCount() == 0) {
        return;
    }

    const auto& selectMgr = RenderSelectManager::instance();
    const auto& colorMap = Util::ColorMap::instance();

    const bool partMode = selectMgr.isTypePickable(RenderEntityType::Part);
    const bool wireMode = selectMgr.isTypePickable(RenderEntityType::Wire);

    const auto& evHover = colorMap.getEdgeVertexHoverColor();
    const auto& evSelect = colorMap.getEdgeVertexSelectionColor();
    const auto& faceHover = colorMap.getFaceHoverColor();
    const auto& faceSelect = colorMap.getFaceSelectionColor();

    const float edgeWidthDefault = Util::RenderStyle::EDGE_LINE_WIDTH;
    const float edgeWidthHover = Util::RenderStyle::EDGE_LINE_WIDTH_HOVER;
    const float edgeWidthSelected = Util::RenderStyle::EDGE_LINE_WIDTH_SELECTED;
    const float vtxSizeBase = Util::RenderStyle::VERTEX_POINT_SIZE;
    const float vtxScaleHover = Util::RenderStyle::VERTEX_SCALE_HOVER;
    const float vtxScaleSelected = Util::RenderStyle::VERTEX_SCALE_SELECTED;

    const float surfaceAlpha = params.xRayMode ? 0.25f : 1.0f;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LEQUAL); // Overdraw on top of normal rendering

    if(params.xRayMode) {
        f->glEnable(GL_BLEND);
        f->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    geomBuffer.bindForDraw();

    // --- Highlighted surface triangles ---
    if(!triangleRanges.empty()) {
        f->glEnable(GL_POLYGON_OFFSET_FILL);
        f->glPolygonOffset(1.0f, 1.0f);

        m_surfaceShader.bind();
        m_surfaceShader.setUniformMatrix4("u_viewMatrix", params.viewMatrix);
        m_surfaceShader.setUniformMatrix4("u_projMatrix", params.projMatrix);
        m_surfaceShader.setUniformVec3("u_cameraPos", params.cameraPos);
        m_surfaceShader.setUniformFloat("u_alpha", surfaceAlpha);

        for(const auto& rangeEx : triangleRanges) {
            bool highlight = false;
            RenderColor hColor;
            float hAlpha = 0.0f;

            if(selectMgr.isSelected(rangeEx.m_entityKey) ||
               (partMode && selectMgr.isPartSelected(rangeEx.m_partUid))) {
                hColor = faceSelect;
                hAlpha = 0.5f;
                highlight = true;
            } else if(selectMgr.isEntityHovered(rangeEx.m_entityKey) ||
                      (partMode && selectMgr.isPartHovered(rangeEx.m_partUid))) {
                hColor = faceHover;
                hAlpha = 0.4f;
                highlight = true;
            }

            if(!highlight) {
                continue; // Skip non-highlighted entities
            }

            m_surfaceShader.setUniformVec4("u_highlightColor", hColor.m_r, hColor.m_g, hColor.m_b,
                                           hAlpha);

            const auto& range = rangeEx.m_range;
            f->glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(range.m_indexCount),
                              GL_UNSIGNED_INT,
                              reinterpret_cast<const void*>(
                                  static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
        }

        m_surfaceShader.release();
        f->glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // --- Highlighted edges ---
    if(!lineRanges.empty()) {
        m_flatShader.bind();
        m_flatShader.setUniformMatrix4("u_viewMatrix", params.viewMatrix);
        m_flatShader.setUniformMatrix4("u_projMatrix", params.projMatrix);

        for(const auto& rangeEx : lineRanges) {
            float lineWidth = edgeWidthDefault;
            bool highlight = false;
            RenderColor hColor;

            if(selectMgr.isSelected(rangeEx.m_entityKey) ||
               (partMode && selectMgr.isPartSelected(rangeEx.m_partUid)) ||
               (wireMode && selectMgr.isEdgeInSelectedWire(rangeEx.m_entityKey.m_uid))) {
                hColor = evSelect;
                lineWidth = edgeWidthSelected;
                highlight = true;
            } else if(selectMgr.isEntityHovered(rangeEx.m_entityKey) ||
                      (partMode && selectMgr.isPartHovered(rangeEx.m_partUid)) ||
                      (wireMode && selectMgr.isEdgeInHoveredWire(rangeEx.m_entityKey.m_uid))) {
                hColor = evHover;
                lineWidth = edgeWidthHover;
                highlight = true;
            }

            if(!highlight) {
                continue;
            }

            m_flatShader.setUniformVec4("u_highlightColor", hColor.m_r, hColor.m_g, hColor.m_b,
                                        1.0f);
            f->glLineWidth(lineWidth);
            const auto& range = rangeEx.m_range;
            f->glDrawElements(GL_LINES, static_cast<GLsizei>(range.m_indexCount), GL_UNSIGNED_INT,
                              reinterpret_cast<const void*>(
                                  static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
        }

        f->glLineWidth(1.0f);
        m_flatShader.release();
    }

    // --- Highlighted points ---
    if(!pointRanges.empty()) {
        m_flatShader.bind();
        m_flatShader.setUniformMatrix4("u_viewMatrix", params.viewMatrix);
        m_flatShader.setUniformMatrix4("u_projMatrix", params.projMatrix);
        f->glEnable(GL_PROGRAM_POINT_SIZE);

        for(const auto& rangeEx : pointRanges) {
            float pointSize = vtxSizeBase;
            bool highlight = false;
            RenderColor hColor;

            if(selectMgr.isSelected(rangeEx.m_entityKey) ||
               (partMode && selectMgr.isPartSelected(rangeEx.m_partUid))) {
                hColor = evSelect;
                pointSize = vtxSizeBase * vtxScaleSelected;
                highlight = true;
            } else if(selectMgr.isEntityHovered(rangeEx.m_entityKey) ||
                      (partMode && selectMgr.isPartHovered(rangeEx.m_partUid))) {
                hColor = evHover;
                pointSize = vtxSizeBase * vtxScaleHover;
                highlight = true;
            }

            if(!highlight) {
                continue;
            }

            m_flatShader.setUniformVec4("u_highlightColor", hColor.m_r, hColor.m_g, hColor.m_b,
                                        1.0f);
            m_flatShader.setUniformFloat("u_pointSize", pointSize);
            const auto& range = rangeEx.m_range;
            f->glDrawArrays(GL_POINTS, static_cast<GLint>(range.m_vertexOffset),
                            static_cast<GLsizei>(range.m_vertexCount));
        }

        m_flatShader.release();
    }

    geomBuffer.unbind();

    if(params.xRayMode) {
        f->glDisable(GL_BLEND);
    }

    // Restore default depth func
    f->glDepthFunc(GL_LESS);
}

void HighlightPass::renderMesh(const PassRenderParams& params,
                               GpuBuffer& meshBuffer,
                               uint32_t meshSurfaceCount,
                               uint32_t meshWireframeCount,
                               uint32_t meshNodeCount,
                               RenderDisplayModeMask meshDisplayMode) {
    if(!isInitialized() || meshBuffer.vertexCount() == 0) {
        return;
    }

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

    // Build selected pick IDs for shader
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

    // Skip if nothing to highlight
    if(hoverLo == 0 && hoverHi == 0 && selectCount == 0) {
        return;
    }

    const auto& faceHover = colorMap.getFaceHoverColor();
    const auto& faceSelect = colorMap.getFaceSelectionColor();
    const auto& evHover = colorMap.getEdgeVertexHoverColor();
    const auto& evSelect = colorMap.getEdgeVertexSelectionColor();

    const float surfaceAlpha = params.xRayMode ? 0.25f : 1.0f;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LEQUAL);

    if(params.xRayMode) {
        f->glEnable(GL_BLEND);
        f->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    meshBuffer.bindForDraw();

    // --- Mesh surface highlight ---
    if(meshSurfaceCount > 0 && (hasMode(meshDisplayMode, RenderDisplayModeMask::Surface) ||
                                selectCount > 0 || (hoverLo != 0 || hoverHi != 0))) {
        f->glEnable(GL_POLYGON_OFFSET_FILL);
        f->glPolygonOffset(1.0f, 1.0f);

        m_meshSurfaceShader.bind();
        m_meshSurfaceShader.setUniformMatrix4("u_viewMatrix", params.viewMatrix);
        m_meshSurfaceShader.setUniformMatrix4("u_projMatrix", params.projMatrix);
        m_meshSurfaceShader.setUniformVec3("u_cameraPos", params.cameraPos);
        m_meshSurfaceShader.setUniformFloat("u_alpha", surfaceAlpha);
        m_meshSurfaceShader.setUniformUvec2("u_hoverPickId", hoverLo, hoverHi);
        m_meshSurfaceShader.setUniformVec4("u_hoverColor", faceHover.m_r, faceHover.m_g,
                                           faceHover.m_b, 0.4f);
        m_meshSurfaceShader.setUniformInt("u_selectCount", selectCount);
        m_meshSurfaceShader.setUniformVec4("u_selectColor", faceSelect.m_r, faceSelect.m_g,
                                           faceSelect.m_b, 0.5f);
        for(int i = 0; i < selectCount; ++i) {
            const std::string name = "u_selectPickIds[" + std::to_string(i) + "]";
            m_meshSurfaceShader.setUniformUvec2(name.c_str(), selectLo[i], selectHi[i]);
        }

        f->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(meshSurfaceCount));

        m_meshSurfaceShader.release();
        f->glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // --- Mesh wireframe highlight ---
    if(hasMode(meshDisplayMode, RenderDisplayModeMask::Wireframe) && meshWireframeCount > 0) {
        m_meshFlatShader.bind();
        m_meshFlatShader.setUniformMatrix4("u_viewMatrix", params.viewMatrix);
        m_meshFlatShader.setUniformMatrix4("u_projMatrix", params.projMatrix);
        m_meshFlatShader.setUniformUvec2("u_hoverPickId", hoverLo, hoverHi);
        m_meshFlatShader.setUniformVec4("u_hoverColor", evHover.m_r, evHover.m_g, evHover.m_b,
                                        1.0f);
        m_meshFlatShader.setUniformInt("u_selectCount", selectCount);
        m_meshFlatShader.setUniformVec4("u_selectColor", evSelect.m_r, evSelect.m_g, evSelect.m_b,
                                        1.0f);
        for(int i = 0; i < selectCount; ++i) {
            const std::string name = "u_selectPickIds[" + std::to_string(i) + "]";
            m_meshFlatShader.setUniformUvec2(name.c_str(), selectLo[i], selectHi[i]);
        }

        f->glLineWidth(1.0f);
        f->glDrawArrays(GL_LINES, static_cast<GLint>(meshSurfaceCount),
                        static_cast<GLsizei>(meshWireframeCount));

        m_meshFlatShader.release();
    }

    // --- Mesh node points highlight ---
    if(hasMode(meshDisplayMode, RenderDisplayModeMask::Points) && meshNodeCount > 0) {
        m_meshFlatShader.bind();
        m_meshFlatShader.setUniformMatrix4("u_viewMatrix", params.viewMatrix);
        m_meshFlatShader.setUniformMatrix4("u_projMatrix", params.projMatrix);
        m_meshFlatShader.setUniformFloat("u_pointSize", 3.0f);
        m_meshFlatShader.setUniformUvec2("u_hoverPickId", hoverLo, hoverHi);
        m_meshFlatShader.setUniformVec4("u_hoverColor", evHover.m_r, evHover.m_g, evHover.m_b,
                                        1.0f);
        m_meshFlatShader.setUniformInt("u_selectCount", selectCount);
        m_meshFlatShader.setUniformVec4("u_selectColor", evSelect.m_r, evSelect.m_g, evSelect.m_b,
                                        1.0f);
        for(int i = 0; i < selectCount; ++i) {
            const std::string name = "u_selectPickIds[" + std::to_string(i) + "]";
            m_meshFlatShader.setUniformUvec2(name.c_str(), selectLo[i], selectHi[i]);
        }

        f->glEnable(GL_PROGRAM_POINT_SIZE);
        f->glDrawArrays(GL_POINTS, static_cast<GLint>(meshSurfaceCount + meshWireframeCount),
                        static_cast<GLsizei>(meshNodeCount));

        m_meshFlatShader.release();
    }

    meshBuffer.unbind();

    if(params.xRayMode) {
        f->glDisable(GL_BLEND);
    }

    f->glDepthFunc(GL_LESS);
}

} // namespace OpenGeoLab::Render
