/**
 * @file geometry_pass.cpp
 * @brief GeometryPass implementation — surfaces, wireframes, and points
 */

#include "geometry_pass.hpp"

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

const char* SURFACE_FRAGMENT_SHADER = R"(
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

const char* FLAT_VERTEX_SHADER = R"(
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

const char* FLAT_FRAGMENT_SHADER = R"(
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

} // anonymous namespace

// =============================================================================
// Lifecycle
// =============================================================================

void GeometryPass::initialize() {
    if(m_initialized) {
        return;
    }

    if(!m_surfaceShader.compile(SURFACE_VERTEX_SHADER, SURFACE_FRAGMENT_SHADER)) {
        LOG_ERROR("GeometryPass: Failed to compile surface shader");
        return;
    }

    if(!m_flatShader.compile(FLAT_VERTEX_SHADER, FLAT_FRAGMENT_SHADER)) {
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
    auto passIt = data.m_passData.find(RenderPassType::Geometry);
    if(passIt == data.m_passData.end()) {
        // No geometry pass data — clear cached ranges
        m_triangleRanges.clear();
        m_lineRanges.clear();
        m_pointRanges.clear();
        return;
    }

    const RenderPassData& passData = passIt->second;

    if(passData.m_dirty) {
        m_gpuBuffer.upload(passData);
        LOG_DEBUG("GeometryPass: Uploaded {} vertices, {} indices", passData.m_vertices.size(),
                  passData.m_indices.size());
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

// =============================================================================
// Rendering
// =============================================================================

void GeometryPass::render(const QMatrix4x4& view,
                          const QMatrix4x4& projection,
                          const QVector3D& camera_pos,
                          bool x_ray_mode) {
    if(!m_initialized || m_gpuBuffer.vertexCount() == 0) {
        return;
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    const auto& selectMgr = RenderSelectManager::instance();
    const auto& colorMap = Util::ColorMap::instance();

    // Only apply Part/Wire-level highlighting when the corresponding pick mode is active.
    // Without this gating, hovering any sub-entity would highlight the whole Part.
    const bool partMode = selectMgr.isTypePickable(RenderEntityType::Part);
    const bool wireMode = selectMgr.isTypePickable(RenderEntityType::Wire);

    // Per-category highlight colors
    const auto& evHover = colorMap.getEdgeVertexHoverColor();
    const auto& evSelect = colorMap.getEdgeVertexSelectionColor();
    const auto& faceHover = colorMap.getFaceHoverColor();
    const auto& faceSelect = colorMap.getFaceSelectionColor();

    // Line width / point size from RenderStyle
    const float edgeWidthDefault = Util::RenderStyle::EDGE_LINE_WIDTH;
    const float edgeWidthHover = Util::RenderStyle::EDGE_LINE_WIDTH_HOVER;
    const float edgeWidthSelected = Util::RenderStyle::EDGE_LINE_WIDTH_SELECTED;
    const float vtxSizeBase = Util::RenderStyle::VERTEX_POINT_SIZE;
    const float vtxScaleHover = Util::RenderStyle::VERTEX_SCALE_HOVER;
    const float vtxScaleSelected = Util::RenderStyle::VERTEX_SCALE_SELECTED;

    // Surface alpha: 1.0 normally, reduced in X-ray mode for see-through effect
    const float surfaceAlpha = x_ray_mode ? 0.25f : 1.0f;

    m_gpuBuffer.bindForDraw();
    f->glEnable(GL_DEPTH_TEST);

    // --- Surface pass (triangles) ---
    if(!m_triangleRanges.empty()) {
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
        m_surfaceShader.setUniformFloat("u_alpha", surfaceAlpha);

        for(const auto& rangeEx : m_triangleRanges) {
            if(selectMgr.isSelected(rangeEx.m_entityKey) ||
               (partMode && selectMgr.isPartSelected(rangeEx.m_partUid))) {
                m_surfaceShader.setUniformVec4("u_highlightColor", faceSelect.m_r, faceSelect.m_g,
                                               faceSelect.m_b, 0.5f);
            } else if(selectMgr.isEntityHovered(rangeEx.m_entityKey) ||
                      (partMode && selectMgr.isPartHovered(rangeEx.m_partUid))) {
                m_surfaceShader.setUniformVec4("u_highlightColor", faceHover.m_r, faceHover.m_g,
                                               faceHover.m_b, 0.4f);
            } else {
                m_surfaceShader.setUniformVec4("u_highlightColor", 0.0f, 0.0f, 0.0f, 0.0f);
            }

            const auto& range = rangeEx.m_range;
            f->glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(range.m_indexCount),
                              GL_UNSIGNED_INT,
                              reinterpret_cast<const void*>(
                                  static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
        }

        m_surfaceShader.release();

        f->glDisable(GL_POLYGON_OFFSET_FILL);

        // Restore state after X-ray blending
        if(x_ray_mode) {
            f->glDepthMask(GL_TRUE);
            f->glDisable(GL_BLEND);
        }
    }

    // --- Wireframe pass (lines) ---
    if(!m_lineRanges.empty()) {
        m_flatShader.bind();
        m_flatShader.setUniformMatrix4("u_viewMatrix", view);
        m_flatShader.setUniformMatrix4("u_projMatrix", projection);

        for(const auto& rangeEx : m_lineRanges) {
            float lineWidth = edgeWidthDefault;

            if(selectMgr.isSelected(rangeEx.m_entityKey) ||
               (partMode && selectMgr.isPartSelected(rangeEx.m_partUid)) ||
               (wireMode && selectMgr.isEdgeInSelectedWire(rangeEx.m_entityKey.m_uid))) {
                m_flatShader.setUniformVec4("u_highlightColor", evSelect.m_r, evSelect.m_g,
                                            evSelect.m_b, 1.0f);
                lineWidth = edgeWidthSelected;
            } else if(selectMgr.isEntityHovered(rangeEx.m_entityKey) ||
                      (partMode && selectMgr.isPartHovered(rangeEx.m_partUid)) ||
                      (wireMode && selectMgr.isEdgeInHoveredWire(rangeEx.m_entityKey.m_uid))) {
                m_flatShader.setUniformVec4("u_highlightColor", evHover.m_r, evHover.m_g,
                                            evHover.m_b, 1.0f);
                lineWidth = edgeWidthHover;
            } else {
                m_flatShader.setUniformVec4("u_highlightColor", 0.0f, 0.0f, 0.0f, 0.0f);
            }

            f->glLineWidth(lineWidth);
            const auto& range = rangeEx.m_range;
            f->glDrawElements(GL_LINES, static_cast<GLsizei>(range.m_indexCount), GL_UNSIGNED_INT,
                              reinterpret_cast<const void*>(
                                  static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
        }

        f->glLineWidth(1.0f);
        m_flatShader.release();
    }

    // --- Points pass ---
    if(!m_pointRanges.empty()) {
        m_flatShader.bind();
        m_flatShader.setUniformMatrix4("u_viewMatrix", view);
        m_flatShader.setUniformMatrix4("u_projMatrix", projection);
        f->glEnable(GL_PROGRAM_POINT_SIZE);

        for(const auto& rangeEx : m_pointRanges) {
            float pointSize = vtxSizeBase;

            if(selectMgr.isSelected(rangeEx.m_entityKey) ||
               (partMode && selectMgr.isPartSelected(rangeEx.m_partUid))) {
                m_flatShader.setUniformVec4("u_highlightColor", evSelect.m_r, evSelect.m_g,
                                            evSelect.m_b, 1.0f);
                pointSize = vtxSizeBase * vtxScaleSelected;
            } else if(selectMgr.isEntityHovered(rangeEx.m_entityKey) ||
                      (partMode && selectMgr.isPartHovered(rangeEx.m_partUid))) {
                m_flatShader.setUniformVec4("u_highlightColor", evHover.m_r, evHover.m_g,
                                            evHover.m_b, 1.0f);
                pointSize = vtxSizeBase * vtxScaleHover;
            } else {
                m_flatShader.setUniformVec4("u_highlightColor", 0.0f, 0.0f, 0.0f, 0.0f);
            }

            m_flatShader.setUniformFloat("u_pointSize", pointSize);
            const auto& range = rangeEx.m_range;
            f->glDrawArrays(GL_POINTS, static_cast<GLint>(range.m_vertexOffset),
                            static_cast<GLsizei>(range.m_vertexCount));
        }

        m_flatShader.release();
    }

    m_gpuBuffer.unbind();
}

} // namespace OpenGeoLab::Render
