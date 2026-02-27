/**
 * @file pick_pass.cpp
 * @brief PickPass implementation — offscreen FBO picking with integer IDs
 */

#include "pick_pass.hpp"
#include "render/core/gpu_buffer.hpp"

#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {

// =============================================================================
// Shader sources
// =============================================================================

namespace {

const char* PICK_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 3) in uvec2 a_pickId;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;
uniform float u_pointSize;
flat out uvec2 v_pickId;
void main() {
    v_pickId = a_pickId;
    gl_PointSize = u_pointSize;
    gl_Position = u_projMatrix * u_viewMatrix * vec4(a_position, 1.0);
}
)";

const char* PICK_FRAGMENT_SHADER = R"(
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

/// Triangle-based geometry types (drawn with GL_TRIANGLES via index buffer)
constexpr auto TRIANGLE_PICK_TYPES = RenderEntityTypeMask::Face | RenderEntityTypeMask::Shell |
                                     RenderEntityTypeMask::Solid | RenderEntityTypeMask::Part |
                                     RenderEntityTypeMask::Wire;

/// Mesh types that use the mesh buffer
constexpr auto MESH_PICK_TYPES =
    RenderEntityTypeMask::MeshNode | RenderEntityTypeMask::MeshLine | RENDER_MESH_ELEMENTS;

} // anonymous namespace

// =============================================================================
// Lifecycle
// =============================================================================

void PickPass::initialize(int width, int height) {
    if(m_initialized) {
        return;
    }

    if(!m_pickShader.compile(PICK_VERTEX_SHADER, PICK_FRAGMENT_SHADER)) {
        LOG_ERROR("PickPass: Failed to compile pick shader");
        return;
    }

    m_fbo.initialize(width, height);
    m_initialized = true;
    LOG_DEBUG("PickPass: Initialized ({}x{})", width, height);
}

void PickPass::resize(int width, int height) {
    if(!m_initialized) {
        return;
    }

    m_fbo.resize(width, height);
    LOG_DEBUG("PickPass: Resized to {}x{}", width, height);
}

void PickPass::cleanup() {
    m_fbo.cleanup();
    m_initialized = false;
    LOG_DEBUG("PickPass: Cleaned up");
}

// =============================================================================
// Render to FBO
// =============================================================================

void PickPass::renderToFbo(const QMatrix4x4& view,
                           const QMatrix4x4& projection,
                           GpuBuffer& geom_buffer,
                           const std::vector<DrawRangeEx>& tri_ranges,
                           const std::vector<DrawRangeEx>& line_ranges,
                           const std::vector<DrawRangeEx>& point_ranges,
                           GpuBuffer& mesh_buffer,
                           uint32_t mesh_surface_count,
                           uint32_t mesh_wireframe_count,
                           uint32_t mesh_node_count,
                           RenderEntityTypeMask pickMask) {
    if(!m_initialized) {
        LOG_ERROR("PickPass: Not initialized");
        return;
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    QOpenGLExtraFunctions* ef = QOpenGLContext::currentContext()->extraFunctions();

    // Bind offscreen FBO and clear
    m_fbo.bind();

    const GLuint clearColor[4] = {0, 0, 0, 0};
    ef->glClearBufferuiv(GL_COLOR, 0, clearColor);
    f->glClear(GL_DEPTH_BUFFER_BIT);
    f->glEnable(GL_DEPTH_TEST);

    // Bind pick shader
    m_pickShader.bind();
    m_pickShader.setUniformMatrix4("u_viewMatrix", view);
    m_pickShader.setUniformMatrix4("u_projMatrix", projection);
    m_pickShader.setUniformFloat("u_pointSize", 1.0f);

    // --- Draw geometry buffer (per-entity selective rendering) ---
    if(geom_buffer.vertexCount() > 0) {
        geom_buffer.bindForDraw();

        // Triangles — draw only if any triangle-based type is in the mask
        if(hasAny(pickMask, TRIANGLE_PICK_TYPES) && !tri_ranges.empty()) {
            for(const auto& rangeEx : tri_ranges) {
                if(!hasAny(pickMask, toMask(rangeEx.m_entityKey.m_type))) {
                    continue;
                }
                const auto& range = rangeEx.m_range;
                f->glDrawElements(
                    GL_TRIANGLES, static_cast<GLsizei>(range.m_indexCount), GL_UNSIGNED_INT,
                    reinterpret_cast<const void*>(static_cast<uintptr_t>(range.m_indexOffset) *
                                                  sizeof(uint32_t)));
            }
        }

        // Lines — draw only if Edge type is in the mask
        if(hasAny(pickMask, RenderEntityTypeMask::Edge) && !line_ranges.empty()) {
            f->glLineWidth(3.0f); // Thicker lines for easier picking
            for(const auto& rangeEx : line_ranges) {
                if(!hasAny(pickMask, toMask(rangeEx.m_entityKey.m_type))) {
                    continue;
                }
                const auto& range = rangeEx.m_range;
                f->glDrawElements(
                    GL_LINES, static_cast<GLsizei>(range.m_indexCount), GL_UNSIGNED_INT,
                    reinterpret_cast<const void*>(static_cast<uintptr_t>(range.m_indexOffset) *
                                                  sizeof(uint32_t)));
            }
            f->glLineWidth(1.0f);
        }

        // Points — draw only if Vertex type is in the mask
        if(hasAny(pickMask, RenderEntityTypeMask::Vertex) && !point_ranges.empty()) {
            f->glEnable(GL_PROGRAM_POINT_SIZE);
            m_pickShader.setUniformFloat("u_pointSize", 12.0f); // Enlarged for easier picking

            for(const auto& rangeEx : point_ranges) {
                if(!hasAny(pickMask, toMask(rangeEx.m_entityKey.m_type))) {
                    continue;
                }
                const auto& range = rangeEx.m_range;
                f->glDrawArrays(GL_POINTS, static_cast<GLint>(range.m_vertexOffset),
                                static_cast<GLsizei>(range.m_vertexCount));
            }

            m_pickShader.setUniformFloat("u_pointSize", 1.0f);
        }

        geom_buffer.unbind();
    }

    // --- Draw mesh buffer (selective rendering per mesh topology) ---
    if(hasAny(pickMask, MESH_PICK_TYPES) && mesh_buffer.vertexCount() > 0) {
        mesh_buffer.bindForDraw();

        // Surface triangles — for mesh element picking
        if(hasAny(pickMask, RENDER_MESH_ELEMENTS) && mesh_surface_count > 0) {
            f->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh_surface_count));
        }

        // Wireframe edges — for mesh line picking
        if(hasAny(pickMask, RenderEntityTypeMask::MeshLine) && mesh_wireframe_count > 0) {
            f->glLineWidth(3.0f); // Thicker lines for easier picking
            f->glDrawArrays(GL_LINES, static_cast<GLint>(mesh_surface_count),
                            static_cast<GLsizei>(mesh_wireframe_count));
            f->glLineWidth(1.0f);
        }

        // Node points — for mesh node picking
        if(hasAny(pickMask, RenderEntityTypeMask::MeshNode) && mesh_node_count > 0) {
            f->glEnable(GL_PROGRAM_POINT_SIZE);
            m_pickShader.setUniformFloat("u_pointSize", 12.0f); // Enlarged for easier picking
            f->glDrawArrays(GL_POINTS,
                            static_cast<GLint>(mesh_surface_count + mesh_wireframe_count),
                            static_cast<GLsizei>(mesh_node_count));
            m_pickShader.setUniformFloat("u_pointSize", 1.0f);
        }

        mesh_buffer.unbind();
    }

    m_pickShader.release();
    m_fbo.unbind();
}

// =============================================================================
// Pick-id readback
// =============================================================================

uint64_t PickPass::readPickId(int pixel_x, int pixel_y) const {
    return m_fbo.readPickId(pixel_x, pixel_y);
}

std::vector<uint64_t> PickPass::readPickRegion(int cx, int cy, int radius) const {
    return m_fbo.readPickRegion(cx, cy, radius);
}

} // namespace OpenGeoLab::Render
