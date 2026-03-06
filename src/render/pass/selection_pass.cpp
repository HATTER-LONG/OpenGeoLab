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

/// Triangle-based geometry types
constexpr auto TRIANGLE_PICK_TYPES = RenderEntityTypeMask::Face | RenderEntityTypeMask::Shell |
                                     RenderEntityTypeMask::Solid | RenderEntityTypeMask::Part |
                                     RenderEntityTypeMask::Wire;

/// Mesh types that use the mesh buffer
constexpr auto MESH_PICK_TYPES =
    RenderEntityTypeMask::MeshNode | RenderEntityTypeMask::MeshLine | RENDER_MESH_ELEMENTS;

IndexedDrawBatch collectIndexedBatches(const IndexedBatchCache& cache, RenderEntityTypeMask mask) {
    IndexedDrawBatch result;
    for(const auto& [type, batch] : cache.m_byType) {
        if(hasAny(mask, toMask(type))) {
            result.append(batch);
        }
    }
    return result;
}

ArrayDrawBatch collectArrayBatches(const ArrayBatchCache& cache, RenderEntityTypeMask mask) {
    ArrayDrawBatch result;
    for(const auto& [type, batch] : cache.m_byType) {
        if(hasAny(mask, toMask(type))) {
            result.append(batch);
        }
    }
    return result;
}

void renderDepthOccluders(QOpenGLContext* ctx_gl,
                          QOpenGLFunctions* f,
                          const GeometryPassInput& geometry,
                          const MeshPassInput& mesh) {
    f->glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    if(geometry.m_buffer.vertexCount() > 0 && !geometry.m_batches.m_triangles.m_all.empty()) {
        geometry.m_buffer.bindForDraw();
        PassUtil::multiDrawElements(ctx_gl, f, GL_TRIANGLES, geometry.m_batches.m_triangles.m_all);
        geometry.m_buffer.unbind();
    }

    if(mesh.m_buffer.vertexCount() > 0 && !mesh.m_batches.m_triangles.m_all.empty()) {
        mesh.m_buffer.bindForDraw();
        PassUtil::multiDrawArrays(ctx_gl, f, GL_TRIANGLES, mesh.m_batches.m_triangles.m_all);
        mesh.m_buffer.unbind();
    }

    f->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

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
    const auto& geometry_batches = ctx.m_geometry.m_batches;
    const auto& mesh_batches = ctx.m_mesh.m_batches;

    const auto& view_matrix = ctx.m_params.m_viewMatrix;
    const auto& proj_matrix = ctx.m_params.m_projMatrix;
    const bool xray_mode = ctx.m_params.m_xRayMode;

    const auto& pick_mask = ctx.m_params.m_pickEntityMask;
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    QOpenGLExtraFunctions* ef = QOpenGLContext::currentContext()->extraFunctions();
    QOpenGLContext* ctx_gl = QOpenGLContext::currentContext();

    m_fbo.bind();

    const GLuint clear_color[4] = {0, 0, 0, 0};
    ef->glClearBufferuiv(GL_COLOR, 0, clear_color);
    f->glClear(GL_DEPTH_BUFFER_BIT);
    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LEQUAL);

    m_pickShader.bind();
    m_pickShader.setUniformMatrix4("u_viewMatrix", view_matrix);
    m_pickShader.setUniformMatrix4("u_projMatrix", proj_matrix);
    m_pickShader.setUniformFloat("u_pointSize", 1.0f);
    m_pickShader.setUniformFloat("u_viewOffset", 0.0f);

    if(!xray_mode) {
        renderDepthOccluders(ctx_gl, f, ctx.m_geometry, ctx.m_mesh);
    }

    if(geom_buffer.vertexCount() > 0) {
        geom_buffer.bindForDraw();

        // Triangles
        if(hasAny(pick_mask, TRIANGLE_PICK_TYPES) && !geometry_batches.m_triangles.m_all.empty()) {
            if(!xray_mode) {
                f->glEnable(GL_CULL_FACE);
                f->glCullFace(GL_BACK);
            }
            const auto triangle_batch =
                collectIndexedBatches(geometry_batches.m_triangles, pick_mask);
            PassUtil::multiDrawElements(ctx_gl, f, GL_TRIANGLES, triangle_batch);
            if(!xray_mode) {
                f->glDisable(GL_CULL_FACE);
            }
        }
        // Lines
        if(hasAny(pick_mask, RenderEntityTypeMask::Edge) &&
           !geometry_batches.m_lines.m_all.empty()) {
            f->glLineWidth(4.0f);
            const auto line_batch = collectIndexedBatches(geometry_batches.m_lines, pick_mask);
            PassUtil::multiDrawElements(ctx_gl, f, GL_LINES, line_batch);
            f->glLineWidth(1.0f);
        }

        // Points
        if(hasAny(pick_mask, RenderEntityTypeMask::Vertex) &&
           !geometry_batches.m_points.m_all.empty()) {
            f->glEnable(GL_PROGRAM_POINT_SIZE);
            m_pickShader.setUniformFloat("u_pointSize", 12.0f);

            const auto point_batch = collectArrayBatches(geometry_batches.m_points, pick_mask);
            PassUtil::multiDrawArrays(ctx_gl, f, GL_POINTS, point_batch);
            m_pickShader.setUniformFloat("u_pointSize", 1.0f);
        }

        geom_buffer.unbind();
    }

    if(mesh_buffer.vertexCount() > 0 && hasAny(pick_mask, MESH_PICK_TYPES)) {
        mesh_buffer.bindForDraw();

        if(hasAny(pick_mask, RENDER_MESH_ELEMENTS) && !mesh_batches.m_triangles.m_all.empty()) {
            if(!xray_mode) {
                f->glEnable(GL_CULL_FACE);
                f->glCullFace(GL_BACK);
            }
            const auto triangle_batch = collectArrayBatches(mesh_batches.m_triangles, pick_mask);
            PassUtil::multiDrawArrays(ctx_gl, f, GL_TRIANGLES, triangle_batch);
            if(!xray_mode) {
                f->glDisable(GL_CULL_FACE);
            }
        }

        if(hasAny(pick_mask, RenderEntityTypeMask::MeshLine) &&
           !mesh_batches.m_lines.m_all.empty()) {
            f->glLineWidth(3.0f);
            const auto line_batch = collectArrayBatches(mesh_batches.m_lines, pick_mask);
            PassUtil::multiDrawArrays(ctx_gl, f, GL_LINES, line_batch);
            f->glLineWidth(1.0f);
        }
        if(hasAny(pick_mask, RenderEntityTypeMask::MeshNode) &&
           !mesh_batches.m_points.m_all.empty()) {
            f->glEnable(GL_PROGRAM_POINT_SIZE);
            m_pickShader.setUniformFloat("u_pointSize", 12.0f);
            const auto point_batch = collectArrayBatches(mesh_batches.m_points, pick_mask);
            PassUtil::multiDrawArrays(ctx_gl, f, GL_POINTS, point_batch);
            m_pickShader.setUniformFloat("u_pointSize", 1.0f);
        }
        mesh_buffer.unbind();
    }
    f->glDisable(GL_PROGRAM_POINT_SIZE);
    m_pickShader.release();
    m_fbo.unbind();
    f->glDepthFunc(GL_LESS);
}

uint64_t SelectionPass::readPickId(int pixel_x, int pixel_y) const {
    return m_fbo.readPickId(pixel_x, pixel_y);
}

std::vector<uint64_t> SelectionPass::readPickRegion(int cx, int cy, int radius) const {
    return m_fbo.readPickRegion(cx, cy, radius);
}
} // namespace OpenGeoLab::Render