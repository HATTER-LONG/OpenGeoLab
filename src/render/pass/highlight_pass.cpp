/**
 * @file highlight_pass.cpp
 * @brief HighlightPass implementation — selected/hovered entity overdraw.
 */

#include "highlight_pass.hpp"

#include "draw_batch_utils.hpp"
#include "render/core/gpu_buffer.hpp"
#include "render/render_select_manager.hpp"
#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <array>
#include <unordered_set>

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

const float kMeshViewOffset = -1.0f;

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

void HighlightPass::render(const RenderPassContext& ctx_data) {
    if(!isInitialized()) {
        return;
    }
    if(ctx_data.m_geometry.m_buffer != nullptr && ctx_data.m_geometry.m_triangleRanges != nullptr &&
       ctx_data.m_geometry.m_lineRanges != nullptr &&
       ctx_data.m_geometry.m_pointRanges != nullptr) {
        renderGeometry(ctx_data.m_params, *ctx_data.m_geometry.m_buffer,
                       *ctx_data.m_geometry.m_triangleRanges, *ctx_data.m_geometry.m_lineRanges,
                       *ctx_data.m_geometry.m_pointRanges);
    }

    if(ctx_data.m_mesh.m_buffer != nullptr && ctx_data.m_mesh.m_triangleRanges != nullptr &&
       ctx_data.m_mesh.m_lineRanges != nullptr && ctx_data.m_mesh.m_pointRanges != nullptr) {
        renderMesh(ctx_data.m_params, *ctx_data.m_mesh.m_buffer, *ctx_data.m_mesh.m_triangleRanges,
                   *ctx_data.m_mesh.m_lineRanges, *ctx_data.m_mesh.m_pointRanges,
                   ctx_data.m_mesh.m_displayMode);
    }
}

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
                               const std::vector<DrawRangeEx>& meshTriangleRanges,
                               const std::vector<DrawRangeEx>& meshLineRanges,
                               const std::vector<DrawRangeEx>& meshPointRanges,
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

    // Build selected pick IDs for shader (split by topology to reduce shader loop work)
    constexpr int MAX_SELECTED = 32;
    std::array<uint32_t, MAX_SELECTED> surface_select_lo{};
    std::array<uint32_t, MAX_SELECTED> surface_select_hi{};
    std::array<uint32_t, MAX_SELECTED> line_select_lo{};
    std::array<uint32_t, MAX_SELECTED> line_select_hi{};
    std::array<uint32_t, MAX_SELECTED> node_select_lo{};
    std::array<uint32_t, MAX_SELECTED> node_select_hi{};
    int surface_select_count = 0;
    int line_select_count = 0;
    int node_select_count = 0;

    std::unordered_set<uint64_t> seen_surface;
    std::unordered_set<uint64_t> seen_line;
    std::unordered_set<uint64_t> seen_node;
    seen_surface.reserve(MAX_SELECTED);
    seen_line.reserve(MAX_SELECTED);
    seen_node.reserve(MAX_SELECTED);

    auto append_pick = [](uint64_t encoded, std::unordered_set<uint64_t>& seen,
                          std::array<uint32_t, MAX_SELECTED>& lo,
                          std::array<uint32_t, MAX_SELECTED>& hi, int& count) {
        if(count >= MAX_SELECTED || !seen.insert(encoded).second) {
            return;
        }
        toUvec2(encoded, lo[count], hi[count]);
        ++count;
    };

    {
        const auto selections = selectMgr.selections();
        for(const auto& sel : selections) {
            if(!isMeshDomain(sel.m_type)) {
                continue;
            }
            const uint64_t encoded = PickId::encode(sel.m_type, sel.m_uid);
            if(sel.m_type == RenderEntityType::MeshLine) {
                append_pick(encoded, seen_line, line_select_lo, line_select_hi, line_select_count);
            } else if(sel.m_type == RenderEntityType::MeshNode) {
                append_pick(encoded, seen_node, node_select_lo, node_select_hi, node_select_count);
            } else {
                append_pick(encoded, seen_surface, surface_select_lo, surface_select_hi,
                            surface_select_count);
            }
        }
    }

    // Skip if nothing to highlight
    if(hoverLo == 0 && hoverHi == 0 && surface_select_count == 0 && line_select_count == 0 &&
       node_select_count == 0) {
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

    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    meshBuffer.bindForDraw();

    auto set_select_uniforms = [](ShaderProgram& shader,
                                  const std::array<uint32_t, MAX_SELECTED>& lo,
                                  const std::array<uint32_t, MAX_SELECTED>& hi, int count) {
        shader.setUniformInt("u_selectCount", count);
        for(int i = 0; i < count; ++i) {
            const std::string name = "u_selectPickIds[" + std::to_string(i) + "]";
            shader.setUniformUvec2(name.c_str(), lo[static_cast<size_t>(i)],
                                   hi[static_cast<size_t>(i)]);
        }
    };

    // --- Mesh surface highlight ---
    if(!meshTriangleRanges.empty() &&
       (hasMode(meshDisplayMode, RenderDisplayModeMask::Surface) || surface_select_count > 0 ||
        (hoverLo != 0 || hoverHi != 0))) {
        f->glEnable(GL_POLYGON_OFFSET_FILL);
        f->glPolygonOffset(1.0f, 1.0f);

        m_meshSurfaceShader.bind();
        m_meshSurfaceShader.setUniformMatrix4("u_viewMatrix", params.viewMatrix);
        m_meshSurfaceShader.setUniformMatrix4("u_projMatrix", params.projMatrix);
        m_meshSurfaceShader.setUniformVec3("u_cameraPos", params.cameraPos);
        m_meshSurfaceShader.setUniformFloat("u_alpha", surfaceAlpha);
        m_meshSurfaceShader.setUniformFloat("u_viewOffset", kMeshViewOffset);
        m_meshSurfaceShader.setUniformUvec2("u_hoverPickId", hoverLo, hoverHi);
        m_meshSurfaceShader.setUniformVec4("u_hoverColor", faceHover.m_r, faceHover.m_g,
                                           faceHover.m_b, 0.4f);
        set_select_uniforms(m_meshSurfaceShader, surface_select_lo, surface_select_hi,
                            surface_select_count);
        m_meshSurfaceShader.setUniformVec4("u_selectColor", faceSelect.m_r, faceSelect.m_g,
                                           faceSelect.m_b, 0.5f);

        std::vector<GLint> firsts;
        std::vector<GLsizei> counts;
        PassUtil::buildArrayBatch(
            meshTriangleRanges, [](const DrawRangeEx&) { return true; }, firsts, counts);
        PassUtil::multiDrawArrays(ctx, f, GL_TRIANGLES, firsts, counts);

        m_meshSurfaceShader.release();
        f->glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // --- Mesh wireframe highlight ---
    if(hasMode(meshDisplayMode, RenderDisplayModeMask::Wireframe) && !meshLineRanges.empty()) {
        m_meshFlatShader.bind();
        m_meshFlatShader.setUniformMatrix4("u_viewMatrix", params.viewMatrix);
        m_meshFlatShader.setUniformMatrix4("u_projMatrix", params.projMatrix);
        m_meshFlatShader.setUniformFloat("u_viewOffset", kMeshViewOffset);
        m_meshFlatShader.setUniformUvec2("u_hoverPickId", hoverLo, hoverHi);
        m_meshFlatShader.setUniformVec4("u_hoverColor", evHover.m_r, evHover.m_g, evHover.m_b,
                                        1.0f);
        set_select_uniforms(m_meshFlatShader, line_select_lo, line_select_hi, line_select_count);
        m_meshFlatShader.setUniformVec4("u_selectColor", evSelect.m_r, evSelect.m_g, evSelect.m_b,
                                        1.0f);

        f->glLineWidth(1.0f);
        std::vector<GLint> firsts;
        std::vector<GLsizei> counts;
        PassUtil::buildArrayBatch(
            meshLineRanges, [](const DrawRangeEx&) { return true; }, firsts, counts);
        PassUtil::multiDrawArrays(ctx, f, GL_LINES, firsts, counts);

        m_meshFlatShader.release();
    }

    // --- Mesh node points highlight ---
    if(hasMode(meshDisplayMode, RenderDisplayModeMask::Points) && !meshPointRanges.empty()) {
        m_meshFlatShader.bind();
        m_meshFlatShader.setUniformMatrix4("u_viewMatrix", params.viewMatrix);
        m_meshFlatShader.setUniformMatrix4("u_projMatrix", params.projMatrix);
        m_meshFlatShader.setUniformFloat("u_pointSize", 3.0f);
        m_meshFlatShader.setUniformFloat("u_viewOffset", kMeshViewOffset);
        m_meshFlatShader.setUniformUvec2("u_hoverPickId", hoverLo, hoverHi);
        m_meshFlatShader.setUniformVec4("u_hoverColor", evHover.m_r, evHover.m_g, evHover.m_b,
                                        1.0f);
        set_select_uniforms(m_meshFlatShader, node_select_lo, node_select_hi, node_select_count);
        m_meshFlatShader.setUniformVec4("u_selectColor", evSelect.m_r, evSelect.m_g, evSelect.m_b,
                                        1.0f);

        f->glEnable(GL_PROGRAM_POINT_SIZE);
        std::vector<GLint> firsts;
        std::vector<GLsizei> counts;
        PassUtil::buildArrayBatch(
            meshPointRanges, [](const DrawRangeEx&) { return true; }, firsts, counts);
        PassUtil::multiDrawArrays(ctx, f, GL_POINTS, firsts, counts);

        m_meshFlatShader.release();
    }

    meshBuffer.unbind();

    if(params.xRayMode) {
        f->glDisable(GL_BLEND);
    }

    f->glDepthFunc(GL_LESS);
}

} // namespace OpenGeoLab::Render
