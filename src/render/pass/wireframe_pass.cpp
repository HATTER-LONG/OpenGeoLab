/**
 * @file wireframe_pass.cpp
 * @brief WireframePass implementation — edges and points rendering.
 */

#include "wireframe_pass.hpp"
#include "draw_batch_utils.hpp"

#include "../core/gpu_buffer.hpp"

#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {

namespace {
const char* flat_vertex_shader = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 2) in vec4 a_color;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;
uniform float u_pointSize;
uniform float u_viewOffset;
out vec4 v_color;
void main() {
    v_color = a_color;
    gl_PointSize = u_pointSize;
    vec4 viewPos = u_viewMatrix * vec4(a_position, 1.0);
    viewPos.z -= u_viewOffset;
    gl_Position = u_projMatrix * viewPos;
}
)";
const char* flat_fragment_shader = R"(
#version 330 core
in vec4 v_color;
out vec4 fragColor;
void main() {
    fragColor = vec4(v_color.rgb, v_color.a);
}
)";

[[nodiscard]] constexpr bool hasMode(RenderDisplayModeMask value, RenderDisplayModeMask flag) {
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

const float K_MESH_VIEW_OFFSET = -1.0f;
} // namespace
bool WireframePass::onInitialize() {
    if(!m_flatShader.compile(flat_vertex_shader, flat_fragment_shader)) {
        LOG_ERROR("WireframePass: Failed to compile flat shader");
        return false;
    }

    LOG_DEBUG("WireframePass: Initialized");
    return true;
}
void WireframePass::onCleanup() { LOG_DEBUG("WireframePass: Cleaned up"); }

void WireframePass::render(RenderPassContext& ctx) {
    if(!isInitialized()) {
        return;
    }
    const auto& params = ctx.m_params;

    auto& geo_buf = ctx.m_geometry.m_buffer;
    auto& mesh_buf = ctx.m_mesh.m_buffer;
    const auto& line_ranges = ctx.m_geometry.m_lineRanges;
    const auto& point_ranges = ctx.m_geometry.m_pointRanges;
    const auto& mesh_line_ranges = ctx.m_mesh.m_lineRanges;
    const auto& mesh_point_ranges = ctx.m_mesh.m_pointRanges;
    const auto mesh_display_mode = ctx.m_mesh.m_displayMode;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    QOpenGLContext* ctx_gl = QOpenGLContext::currentContext();
    f->glEnable(GL_DEPTH_TEST);

    m_flatShader.bind();
    m_flatShader.setUniformMatrix4("u_viewMatrix", params.m_viewMatrix);
    m_flatShader.setUniformMatrix4("u_projMatrix", params.m_projMatrix);
    m_flatShader.setUniformFloat("u_pointSize", Util::RenderStyle::VERTEX_POINT_SIZE);
    m_flatShader.setUniformFloat("u_viewOffset", 0.0f);
    // --- Geometry edges (indexed GL_LINES) ---
    if(!line_ranges.empty() && geo_buf.vertexCount() > 0) {
        geo_buf.bindForDraw();
        f->glLineWidth(Util::RenderStyle::EDGE_LINE_WIDTH);
        std::vector<GLsizei> counts;
        std::vector<const void*> offsets;
        PassUtil::buildIndexedBatch(
            line_ranges, [](const DrawRange&) { return true; }, counts, offsets);
        PassUtil::multiDrawElements(ctx_gl, f, GL_LINES, counts, offsets);

        f->glLineWidth(1.0f);
        geo_buf.unbind();
    }
    // --- Geometry points (GL_POINTS) ---
    if(!point_ranges.empty() && geo_buf.vertexCount() > 0) {
        geo_buf.bindForDraw();
        f->glEnable(GL_PROGRAM_POINT_SIZE);
        m_flatShader.setUniformFloat("u_pointSize", Util::RenderStyle::VERTEX_POINT_SIZE);

        std::vector<GLint> firsts;
        std::vector<GLsizei> counts;
        PassUtil::buildArrayBatch(
            point_ranges, [](const DrawRange&) { return true; }, firsts, counts);
        PassUtil::multiDrawArrays(ctx_gl, f, GL_POINTS, firsts, counts);

        geo_buf.unbind();
    }
    // --- Mesh wireframe edges (array GL_LINES) ---
    if(hasMode(mesh_display_mode, RenderDisplayModeMask::Wireframe) && !mesh_line_ranges.empty() &&
       mesh_buf.vertexCount() > 0) {
        m_flatShader.setUniformFloat("u_viewOffset", K_MESH_VIEW_OFFSET);
        mesh_buf.bindForDraw();

        f->glLineWidth(1.0f);
        std::vector<GLint> firsts;
        std::vector<GLsizei> counts;
        PassUtil::buildArrayBatch(
            mesh_line_ranges, [](const DrawRange&) { return true; }, firsts, counts);
        PassUtil::multiDrawArrays(ctx_gl, f, GL_LINES, firsts, counts);

        mesh_buf.unbind();
        m_flatShader.setUniformFloat("u_viewOffset", 0.0f);
    }

    // --- Mesh node points (array GL_POINTS) ---
    if(hasMode(mesh_display_mode, RenderDisplayModeMask::Points) && !mesh_point_ranges.empty() &&
       mesh_buf.vertexCount() > 0) {
        m_flatShader.setUniformFloat("u_viewOffset", K_MESH_VIEW_OFFSET);
        mesh_buf.bindForDraw();

        f->glEnable(GL_PROGRAM_POINT_SIZE);
        m_flatShader.setUniformFloat("u_pointSize", 3.0f);
        std::vector<GLint> firsts;
        std::vector<GLsizei> counts;
        PassUtil::buildArrayBatch(
            mesh_point_ranges, [](const DrawRange&) { return true; }, firsts, counts);
        PassUtil::multiDrawArrays(ctx_gl, f, GL_POINTS, firsts, counts);

        mesh_buf.unbind();
        m_flatShader.setUniformFloat("u_viewOffset", 0.0f);
    }
}

} // namespace OpenGeoLab::Render