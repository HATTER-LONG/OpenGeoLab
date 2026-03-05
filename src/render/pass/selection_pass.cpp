/**
 * @file selection_pass.cpp
 * @brief SelectionPass implementation — offscreen FBO picking with integer IDs.
 *
 * Renamed from PickPass. Renders entity IDs to an offscreen framebuffer
 * for GPU-based selection and hover detection.
 */

#include "selection_pass.hpp"
#include "../core/gpu_buffer.hpp"
#include "draw_batch_utils.hpp"

#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {

namespace {

const char* pick_vertex_shader = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 3) in uvec2 a_pickId;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;
uniform float u_pointSize;
uniform float u_viewOffset;
flat out uvec2 v_pickId;
void main() {
    v_pickId = a_pickId;
    gl_PointSize = u_pointSize;
    vec4 viewPos = u_viewMatrix * vec4(a_position, 1.0);
    viewPos.z -= u_viewOffset;
    gl_Position = u_projMatrix * viewPos;
}
)";

const char* pick_fragment_shader = R"(
#version 330 core
flat in uvec2 v_pickId;
layout(location = 0) out uvec2 fragPickId;
void main() {
    fragPickId = v_pickId;
}
)";

[[nodiscard]] constexpr bool hasAny(RenderEntityTypeMask value, RenderEntityTypeMask mask) {
    return static_cast<uint32_t>(value & mask) != 0u;
}

const float K_MESH_VIEW_OFFSET = -1.0f;

/// Triangle-based geometry types
constexpr auto TRIANGLE_PICK_TYPES = RenderEntityTypeMask::Face | RenderEntityTypeMask::Shell |
                                     RenderEntityTypeMask::Solid | RenderEntityTypeMask::Part |
                                     RenderEntityTypeMask::Wire;

/// Mesh types that use the mesh buffer
constexpr auto MESH_PICK_TYPES =
    RenderEntityTypeMask::MeshNode | RenderEntityTypeMask::MeshLine | RENDER_MESH_ELEMENTS;

} // namespace

void SelectionPass::initialize(int width, int height) {
    if(m_initialized) {
        return;
    }

    if(!m_pickShader.compile(pick_vertex_shader, pick_fragment_shader)) {
        LOG_ERROR("SelectionPass: Failed to compile pick shader");
        return;
    }

    m_fbo.initialize(width, height);
    m_initialized = true;
    LOG_DEBUG("SelectionPass: Initialized ({}x{})", width, height);
}
void SelectionPass::resize(int width, int height) {
    if(!m_initialized) {
        return;
    }
    m_fbo.resize(width, height);
    LOG_DEBUG("SelectionPass: Resized to {}x{}", width, height);
}

void SelectionPass::cleanup() {
    m_fbo.cleanup();
    m_initialized = false;
    LOG_DEBUG("SelectionPass: Cleaned up");
}

void SelectionPass::render(RenderPassContext& ctx) {
    if(!m_initialized) {
        return;
    }

    auto& geom_buffer = ctx.m_geometry.m_buffer;
    auto& mesh_buffer = ctx.m_mesh.m_buffer;
    const auto& tri_ranges = ctx.m_geometry.m_triangleRanges;
    const auto& line_ranges = ctx.m_geometry.m_lineRanges;
    const auto& point_ranges = ctx.m_geometry.m_pointRanges;

    const auto& mesh_tri_ranges = ctx.m_mesh.m_triangleRanges;
    const auto& mesh_line_ranges = ctx.m_mesh.m_lineRanges;
    const auto& mesh_point_ranges = ctx.m_mesh.m_pointRanges;

    const auto& view_matrix = ctx.m_params.m_viewMatrix;
    const auto& proj_matrix = ctx.m_params.m_projMatrix;

    const auto& pick_mask = ctx.m_params.m_pickEntityMask;
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    QOpenGLExtraFunctions* ef = QOpenGLContext::currentContext()->extraFunctions();
    QOpenGLContext* ctx_gl = QOpenGLContext::currentContext();

    m_fbo.bind();

    const GLuint clear_color[4] = {0, 0, 0, 0};
    ef->glClearBufferuiv(GL_COLOR, 0, clear_color);
    f->glClear(GL_DEPTH_BUFFER_BIT);
    f->glEnable(GL_DEPTH_TEST);

    m_pickShader.bind();
    m_pickShader.setUniformMatrix4("u_viewMatrix", view_matrix);
    m_pickShader.setUniformMatrix4("u_projMatrix", proj_matrix);
    m_pickShader.setUniformFloat("u_pointSize", 1.0f);
    m_pickShader.setUniformFloat("u_viewOffset", 0.0f);

    if(geom_buffer.vertexCount() > 0) {
        geom_buffer.bindForDraw();

        // Triangles
        if(hasAny(pick_mask, TRIANGLE_PICK_TYPES) && !tri_ranges.empty()) {
            std::vector<GLsizei> counts;
            std::vector<const void*> offsets;
            PassUtil::buildIndexedBatch(
                tri_ranges,
                [&pick_mask](const DrawRange& range) {
                    return hasAny(pick_mask, toMask(range.m_entityKey.m_type));
                },
                counts, offsets);
            PassUtil::multiDrawElements(ctx_gl, f, GL_TRIANGLES, counts, offsets);
        }
        // Lines
        if(hasAny(pick_mask, RenderEntityTypeMask::Edge) && !line_ranges.empty()) {
            f->glLineWidth(4.0f);
            std::vector<GLsizei> counts;
            std::vector<const void*> offsets;
            PassUtil::buildIndexedBatch(
                line_ranges,
                [&pick_mask](const DrawRange& range) {
                    return hasAny(pick_mask, toMask(range.m_entityKey.m_type));
                },
                counts, offsets);
            PassUtil::multiDrawElements(ctx_gl, f, GL_LINES, counts, offsets);
            f->glLineWidth(1.0f);
        }

        // Points
        if(hasAny(pick_mask, RenderEntityTypeMask::Vertex) && !point_ranges.empty()) {
            f->glEnable(GL_PROGRAM_POINT_SIZE);
            m_pickShader.setUniformFloat("u_pointSize", 12.0f);

            std::vector<GLint> firsts;
            std::vector<GLsizei> counts;
            PassUtil::buildArrayBatch(
                point_ranges,
                [&pick_mask](const DrawRange& range) {
                    return hasAny(pick_mask, toMask(range.m_entityKey.m_type));
                },
                firsts, counts);
            PassUtil::multiDrawArrays(ctx_gl, f, GL_POINTS, firsts, counts);
            m_pickShader.setUniformFloat("u_pointSize", 1.0f);
        }

        geom_buffer.unbind();
    }

    if(mesh_buffer.vertexCount() > 0 && hasAny(pick_mask, MESH_PICK_TYPES)) {
        m_pickShader.setUniformFloat("u_viewOffset", K_MESH_VIEW_OFFSET);
        mesh_buffer.bindForDraw();

        if(hasAny(pick_mask, RENDER_MESH_ELEMENTS) && !mesh_tri_ranges.empty()) {
            std::vector<GLint> firsts;
            std::vector<GLsizei> counts;
            PassUtil::buildArrayBatch(
                mesh_tri_ranges,
                [&pick_mask](const DrawRange& range) {
                    return hasAny(pick_mask, toMask(range.m_entityKey.m_type));
                },
                firsts, counts);
            PassUtil::multiDrawArrays(ctx_gl, f, GL_TRIANGLES, firsts, counts);
        }

        if(hasAny(pick_mask, RenderEntityTypeMask::MeshLine) && !mesh_line_ranges.empty()) {
            f->glLineWidth(3.0f);
            std::vector<GLint> firsts;
            std::vector<GLsizei> counts;
            PassUtil::buildArrayBatch(
                mesh_line_ranges, [](const DrawRange&) { return true; }, firsts, counts);
            PassUtil::multiDrawArrays(ctx_gl, f, GL_LINES, firsts, counts);
            f->glLineWidth(1.0f);
        }
        if(hasAny(pick_mask, RenderEntityTypeMask::MeshNode) && !mesh_point_ranges.empty()) {
            f->glEnable(GL_PROGRAM_POINT_SIZE);
            m_pickShader.setUniformFloat("u_pointSize", 12.0f);
            std::vector<GLint> firsts;
            std::vector<GLsizei> counts;
            PassUtil::buildArrayBatch(
                mesh_point_ranges, [](const DrawRange&) { return true; }, firsts, counts);
            PassUtil::multiDrawArrays(ctx_gl, f, GL_POINTS, firsts, counts);
            m_pickShader.setUniformFloat("u_pointSize", 1.0f);
        }
        mesh_buffer.unbind();
        m_pickShader.setUniformFloat("u_viewOffset", 0.0f);
    }
    m_pickShader.release();
    m_fbo.unbind();
}

uint64_t SelectionPass::readPickId(int pixel_x, int pixel_y) const {
    return m_fbo.readPickId(pixel_x, pixel_y);
}

std::vector<uint64_t> SelectionPass::readPickRegion(int cx, int cy, int radius) const {
    return m_fbo.readPickRegion(cx, cy, radius);
}
} // namespace OpenGeoLab::Render