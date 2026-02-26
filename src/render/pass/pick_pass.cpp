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

/// Render geometry buffer entities matching the pick mask.
void renderGeometryPick(QOpenGLFunctions* f,
                        ShaderProgram& shader,
                        const GeometryPickInput& geom,
                        RenderEntityTypeMask pickMask) {
    if(geom.m_buffer.vertexCount() == 0) {
        return;
    }

    geom.m_buffer.bindForDraw();

    // Triangles — face/solid/part/shell/wire types
    if(hasAny(pickMask, TRIANGLE_PICK_TYPES) && !geom.m_triRanges.empty()) {
        for(const auto& rangeEx : geom.m_triRanges) {
            if(!hasAny(pickMask, toMask(rangeEx.m_entityKey.m_type))) {
                continue;
            }
            const auto& range = rangeEx.m_range;
            f->glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(range.m_indexCount),
                              GL_UNSIGNED_INT,
                              reinterpret_cast<const void*>(
                                  static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
        }
    }

    // Lines — edge types
    if(hasAny(pickMask, RenderEntityTypeMask::Edge) && !geom.m_lineRanges.empty()) {
        f->glLineWidth(3.0f);
        for(const auto& rangeEx : geom.m_lineRanges) {
            if(!hasAny(pickMask, toMask(rangeEx.m_entityKey.m_type))) {
                continue;
            }
            const auto& range = rangeEx.m_range;
            f->glDrawElements(GL_LINES, static_cast<GLsizei>(range.m_indexCount), GL_UNSIGNED_INT,
                              reinterpret_cast<const void*>(
                                  static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
        }
        f->glLineWidth(1.0f);
    }

    // Points — vertex types
    if(hasAny(pickMask, RenderEntityTypeMask::Vertex) && !geom.m_pointRanges.empty()) {
        f->glEnable(GL_PROGRAM_POINT_SIZE);
        shader.setUniformFloat("u_pointSize", 12.0f);
        for(const auto& rangeEx : geom.m_pointRanges) {
            if(!hasAny(pickMask, toMask(rangeEx.m_entityKey.m_type))) {
                continue;
            }
            const auto& range = rangeEx.m_range;
            f->glDrawArrays(GL_POINTS, static_cast<GLint>(range.m_vertexOffset),
                            static_cast<GLsizei>(range.m_vertexCount));
        }
        shader.setUniformFloat("u_pointSize", 1.0f);
    }

    geom.m_buffer.unbind();
}

/// Render mesh buffer entities matching the pick mask.
void renderMeshPick(QOpenGLFunctions* f,
                    ShaderProgram& shader,
                    const MeshPickInput& mesh,
                    RenderEntityTypeMask pickMask) {
    if(!hasAny(pickMask, MESH_PICK_TYPES) || mesh.m_buffer.vertexCount() == 0) {
        return;
    }

    mesh.m_buffer.bindForDraw();

    if(hasAny(pickMask, RENDER_MESH_ELEMENTS) && mesh.m_surfaceCount > 0) {
        f->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh.m_surfaceCount));
    }

    if(hasAny(pickMask, RenderEntityTypeMask::MeshLine) && mesh.m_wireframeCount > 0) {
        f->glLineWidth(3.0f);
        f->glDrawArrays(GL_LINES, static_cast<GLint>(mesh.m_surfaceCount),
                        static_cast<GLsizei>(mesh.m_wireframeCount));
        f->glLineWidth(1.0f);
    }

    if(hasAny(pickMask, RenderEntityTypeMask::MeshNode) && mesh.m_nodeCount > 0) {
        f->glEnable(GL_PROGRAM_POINT_SIZE);
        shader.setUniformFloat("u_pointSize", 12.0f);
        f->glDrawArrays(GL_POINTS, static_cast<GLint>(mesh.m_surfaceCount + mesh.m_wireframeCount),
                        static_cast<GLsizei>(mesh.m_nodeCount));
        shader.setUniformFloat("u_pointSize", 1.0f);
    }

    mesh.m_buffer.unbind();
}

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
                           const GeometryPickInput& geom,
                           const MeshPickInput& mesh,
                           RenderEntityTypeMask pickMask) {
    if(!m_initialized) {
        LOG_ERROR("PickPass: Not initialized");
        return;
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    QOpenGLExtraFunctions* ef = QOpenGLContext::currentContext()->extraFunctions();

    m_fbo.bind();

    const GLuint clearColor[4] = {0, 0, 0, 0};
    ef->glClearBufferuiv(GL_COLOR, 0, clearColor);
    f->glClear(GL_DEPTH_BUFFER_BIT);
    f->glEnable(GL_DEPTH_TEST);

    m_pickShader.bind();
    m_pickShader.setUniformMatrix4("u_viewMatrix", view);
    m_pickShader.setUniformMatrix4("u_projMatrix", projection);
    m_pickShader.setUniformFloat("u_pointSize", 1.0f);

    renderGeometryPick(f, m_pickShader, geom, pickMask);
    renderMeshPick(f, m_pickShader, mesh, pickMask);

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
