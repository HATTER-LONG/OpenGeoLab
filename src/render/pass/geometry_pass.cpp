/**
 * @file geometry_pass.cpp
 * @brief GeometryPass implementation — surfaces, wireframes, and points
 */

#include "geometry_pass.hpp"

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
out vec4 fragColor;
void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(u_cameraPos - v_worldPos);
    float diffuse = max(dot(N, V), 0.0);
    float ambient = 0.25;
    diffuse = max(diffuse, max(dot(-N, V), 0.0) * 0.6);
    vec3 color = v_color.rgb * (ambient + diffuse * 0.75);
    fragColor = vec4(color, v_color.a);
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
out vec4 fragColor;
void main() {
    fragColor = v_color;
}
)";

/**
 * @brief Recursively collect DrawRanges from a visible RenderNode tree
 *        for the Geometry pass, grouped by primitive topology.
 */
void collectDrawRanges(const RenderNode& node,
                       std::vector<DrawRange>& tris,
                       std::vector<DrawRange>& lines,
                       std::vector<DrawRange>& points) {
    if(!node.m_visible) {
        return;
    }

    auto it = node.m_drawRanges.find(RenderPassType::Geometry);
    if(it != node.m_drawRanges.end()) {
        for(const auto& range : it->second) {
            switch(range.m_topology) {
            case PrimitiveTopology::Triangles:
                tris.push_back(range);
                break;
            case PrimitiveTopology::Lines:
                lines.push_back(range);
                break;
            case PrimitiveTopology::Points:
                points.push_back(range);
                break;
            }
        }
    }

    for(const auto& child : node.m_children) {
        collectDrawRanges(child, tris, lines, points);
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
            collectDrawRanges(root, m_triangleRanges, m_lineRanges, m_pointRanges);
        }
    }
}

// =============================================================================
// Rendering
// =============================================================================

void GeometryPass::render(const QMatrix4x4& view,
                          const QMatrix4x4& projection,
                          const QVector3D& camera_pos) {
    if(!m_initialized || m_gpuBuffer.vertexCount() == 0) {
        return;
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();

    m_gpuBuffer.bindForDraw();
    f->glEnable(GL_DEPTH_TEST);

    // --- Surface pass (triangles) ---
    if(!m_triangleRanges.empty()) {
        m_surfaceShader.bind();
        m_surfaceShader.setUniformMatrix4("u_viewMatrix", view);
        m_surfaceShader.setUniformMatrix4("u_projMatrix", projection);
        m_surfaceShader.setUniformVec3("u_cameraPos", camera_pos);

        for(const auto& range : m_triangleRanges) {
            f->glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(range.m_indexCount),
                              GL_UNSIGNED_INT,
                              reinterpret_cast<const void*>(
                                  static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
        }

        m_surfaceShader.release();
    }

    // --- Wireframe pass (lines) ---
    if(!m_lineRanges.empty()) {
        m_flatShader.bind();
        m_flatShader.setUniformMatrix4("u_viewMatrix", view);
        m_flatShader.setUniformMatrix4("u_projMatrix", projection);

        for(const auto& range : m_lineRanges) {
            f->glDrawElements(GL_LINES, static_cast<GLsizei>(range.m_indexCount), GL_UNSIGNED_INT,
                              reinterpret_cast<const void*>(
                                  static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
        }

        m_flatShader.release();
    }

    // --- Points pass ---
    if(!m_pointRanges.empty()) {
        m_flatShader.bind();
        m_flatShader.setUniformMatrix4("u_viewMatrix", view);
        m_flatShader.setUniformMatrix4("u_projMatrix", projection);
        m_flatShader.setUniformFloat("u_pointSize", 5.0f);
        f->glEnable(GL_PROGRAM_POINT_SIZE);

        for(const auto& range : m_pointRanges) {
            f->glDrawArrays(GL_POINTS, static_cast<GLint>(range.m_vertexOffset),
                            static_cast<GLsizei>(range.m_vertexCount));
        }

        m_flatShader.release();
    }

    m_gpuBuffer.unbind();
}

} // namespace OpenGeoLab::Render
