/**
 * @file mesh_pass.cpp
 * @brief MeshPass implementation — triangulated FEM element rendering
 */

#include "mesh_pass.hpp"

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
out vec4 v_color;
void main() {
    v_color = a_color;
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
        return;
    }

    const RenderPassData& passData = passIt->second;

    if(passData.m_dirty) {
        m_gpuBuffer.upload(passData);
        LOG_DEBUG("MeshPass: Uploaded {} vertices, {} indices", passData.m_vertices.size(),
                  passData.m_indices.size());
    }

    m_totalVertexCount = static_cast<uint32_t>(passData.m_vertices.size());
}

// =============================================================================
// Rendering
// =============================================================================

void MeshPass::render(const QMatrix4x4& view,
                      const QMatrix4x4& projection,
                      const QVector3D& camera_pos) {
    if(!m_initialized || m_totalVertexCount == 0) {
        return;
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();

    m_gpuBuffer.bindForDraw();
    f->glEnable(GL_DEPTH_TEST);

    // --- Surface pass (all mesh vertices as triangles, non-indexed) ---
    m_surfaceShader.bind();
    m_surfaceShader.setUniformMatrix4("u_viewMatrix", view);
    m_surfaceShader.setUniformMatrix4("u_projMatrix", projection);
    m_surfaceShader.setUniformVec3("u_cameraPos", camera_pos);

    f->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_totalVertexCount));

    m_surfaceShader.release();
    m_gpuBuffer.unbind();
}

} // namespace OpenGeoLab::Render
