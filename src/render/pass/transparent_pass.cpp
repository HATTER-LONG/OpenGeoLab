/**
 * @file transparent_pass.cpp
 * @brief TransparentPass implementation — X-ray mode with premultiplied alpha blending
 */

#include "transparent_pass.hpp"

#include "util/logger.hpp"

#include <QOpenGLContext>

namespace OpenGeoLab::Render {
namespace {

const char* transparent_vertex_shader = R"(
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

const char* transparent_fragment_shader = R"(
#version 330 core
in vec3 v_worldPos;
in vec3 v_normal;
in vec4 v_color;
uniform vec3 u_cameraPos;
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
    // Premultiply alpha for correct Qt Quick scene-graph compositing
    fragColor = vec4(litColor * u_alpha, u_alpha);
}
)";

} // namespace

void TransparentPass::initialize() {
    if(m_initialized) {
        return;
    }
    if(!m_shader.compile(transparent_vertex_shader, transparent_fragment_shader)) {
        LOG_ERROR("TransparentPass: Failed to compile shader");
        return;
    }
    m_initialized = true;
    LOG_DEBUG("TransparentPass: Initialized");
}

void TransparentPass::cleanup() {
    m_initialized = false;
    LOG_DEBUG("TransparentPass: Cleaned up");
}

void TransparentPass::render(QOpenGLFunctions* f,
                             GpuBuffer& gpu_buffer,
                             const QMatrix4x4& view,
                             const QMatrix4x4& projection,
                             const QVector3D& camera_pos,
                             const std::vector<DrawRange>& triangle_ranges) {
    if(!m_initialized || triangle_ranges.empty()) {
        return;
    }

    uint32_t total_index_count = 0;
    for(const auto& r : triangle_ranges) {
        total_index_count += r.m_indexCount;
    }
    if(total_index_count == 0) {
        return;
    }
    const uint32_t first_offset = triangle_ranges.front().m_indexOffset;

    // Enable blending for transparency
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    f->glDepthMask(GL_FALSE);

    gpu_buffer.bindForDraw();

    m_shader.bind();
    m_shader.setUniformMatrix4("u_viewMatrix", view);
    m_shader.setUniformMatrix4("u_projMatrix", projection);
    m_shader.setUniformVec3("u_cameraPos", camera_pos);
    m_shader.setUniformFloat("u_alpha", XRAY_ALPHA);

    f->glDrawElements(
        GL_TRIANGLES, static_cast<GLsizei>(total_index_count), GL_UNSIGNED_INT,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(first_offset) * sizeof(uint32_t)));

    m_shader.release();
    gpu_buffer.unbind();

    // Restore state
    f->glDepthMask(GL_TRUE);
    f->glDisable(GL_BLEND);
}

} // namespace OpenGeoLab::Render
