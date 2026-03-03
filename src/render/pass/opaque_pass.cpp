/**
 * @file opaque_pass.cpp
 * @brief OpaquePass implementation — batch triangle rendering with lighting
 */

#include "opaque_pass.hpp"

#include "util/logger.hpp"

#include <QOpenGLContext>

namespace OpenGeoLab::Render {
namespace {

const char* opaque_vertex_shader = R"(
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

const char* opaque_fragment_shader = R"(
#version 330 core
in vec3 v_worldPos;
in vec3 v_normal;
in vec4 v_color;
uniform vec3 u_cameraPos;
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
    fragColor = vec4(litColor, 1.0);
}
)";

} // namespace

void OpaquePass::initialize() {
    if(m_initialized) {
        return;
    }
    if(!m_shader.compile(opaque_vertex_shader, opaque_fragment_shader)) {
        LOG_ERROR("OpaquePass: Failed to compile shader");
        return;
    }
    m_initialized = true;
    LOG_DEBUG("OpaquePass: Initialized");
}

void OpaquePass::cleanup() {
    m_initialized = false;
    LOG_DEBUG("OpaquePass: Cleaned up");
}

void OpaquePass::render(QOpenGLFunctions* f,
                        GpuBuffer& gpu_buffer,
                        const QMatrix4x4& view,
                        const QMatrix4x4& projection,
                        const QVector3D& camera_pos,
                        const std::vector<DrawRange>& triangle_ranges) {
    if(!m_initialized || triangle_ranges.empty()) {
        return;
    }

    // Compute total index range (all triangle indices are contiguous)
    uint32_t total_index_count = 0;
    for(const auto& r : triangle_ranges) {
        total_index_count += r.m_indexCount;
    }
    if(total_index_count == 0) {
        return;
    }
    const uint32_t first_offset = triangle_ranges.front().m_indexOffset;

    gpu_buffer.bindForDraw();

    m_shader.bind();
    m_shader.setUniformMatrix4("u_viewMatrix", view);
    m_shader.setUniformMatrix4("u_projMatrix", projection);
    m_shader.setUniformVec3("u_cameraPos", camera_pos);

    f->glDrawElements(
        GL_TRIANGLES, static_cast<GLsizei>(total_index_count), GL_UNSIGNED_INT,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(first_offset) * sizeof(uint32_t)));

    m_shader.release();
    gpu_buffer.unbind();
}

} // namespace OpenGeoLab::Render
