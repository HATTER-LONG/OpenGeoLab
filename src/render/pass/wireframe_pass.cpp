/**
 * @file wireframe_pass.cpp
 * @brief WireframePass implementation — batch line and point rendering
 */

#include "wireframe_pass.hpp"

#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>

namespace OpenGeoLab::Render {
namespace {

const char* wireframe_vertex_shader = R"(
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

const char* wireframe_fragment_shader = R"(
#version 330 core
in vec4 v_color;
out vec4 fragColor;
void main() {
    fragColor = vec4(v_color.rgb, v_color.a);
}
)";

} // namespace

void WireframePass::initialize() {
    if(m_initialized) {
        return;
    }
    if(!m_shader.compile(wireframe_vertex_shader, wireframe_fragment_shader)) {
        LOG_ERROR("WireframePass: Failed to compile shader");
        return;
    }
    m_initialized = true;
    LOG_DEBUG("WireframePass: Initialized");
}

void WireframePass::cleanup() {
    m_initialized = false;
    LOG_DEBUG("WireframePass: Cleaned up");
}

void WireframePass::render(QOpenGLFunctions* f,
                           GpuBuffer& gpu_buffer,
                           const QMatrix4x4& view,
                           const QMatrix4x4& projection,
                           const std::vector<DrawRange>& line_ranges,
                           const std::vector<DrawRange>& point_ranges) {
    if(!m_initialized) {
        return;
    }
    if(line_ranges.empty() && point_ranges.empty()) {
        return;
    }

    gpu_buffer.bindForDraw();

    m_shader.bind();
    m_shader.setUniformMatrix4("u_viewMatrix", view);
    m_shader.setUniformMatrix4("u_projMatrix", projection);

    // Lines: single batch draw
    if(!line_ranges.empty()) {
        uint32_t total_index_count = 0;
        for(const auto& r : line_ranges) {
            total_index_count += r.m_indexCount;
        }
        if(total_index_count > 0) {
            const uint32_t first_offset = line_ranges.front().m_indexOffset;
            f->glLineWidth(Util::RenderStyle::EDGE_LINE_WIDTH);
            f->glDrawElements(GL_LINES, static_cast<GLsizei>(total_index_count), GL_UNSIGNED_INT,
                              reinterpret_cast<const void*>(static_cast<uintptr_t>(first_offset) *
                                                            sizeof(uint32_t)));
        }
    }

    // Points: single batch draw
    if(!point_ranges.empty()) {
        uint32_t total_vertex_count = 0;
        for(const auto& r : point_ranges) {
            total_vertex_count += r.m_vertexCount;
        }
        if(total_vertex_count > 0) {
            const uint32_t first_vertex = point_ranges.front().m_vertexOffset;
            f->glEnable(GL_PROGRAM_POINT_SIZE);
            m_shader.setUniformFloat("u_pointSize", Util::RenderStyle::VERTEX_POINT_SIZE);
            f->glDrawArrays(GL_POINTS, static_cast<GLint>(first_vertex),
                            static_cast<GLsizei>(total_vertex_count));
        }
    }

    f->glLineWidth(1.0f);
    m_shader.release();
    gpu_buffer.unbind();
}

} // namespace OpenGeoLab::Render
