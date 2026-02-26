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
flat out uvec2 v_pickId;
void main() {
    v_pickId = a_pickId;
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
// Execute picking
// =============================================================================

uint64_t PickPass::execute(int pixel_x,
                           int pixel_y,
                           const QMatrix4x4& view,
                           const QMatrix4x4& projection,
                           GpuBuffer& geom_buffer,
                           GpuBuffer& mesh_buffer) {
    if(!m_initialized) {
        LOG_ERROR("PickPass: Not initialized");
        return 0;
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();

    // Bind offscreen FBO and clear
    m_fbo.bind();

    // Clear with zero (background = no pick)
    const GLuint clearColor[4] = {0, 0, 0, 0};
    QOpenGLExtraFunctions* ef = QOpenGLContext::currentContext()->extraFunctions();
    ef->glClearBufferuiv(GL_COLOR, 0, clearColor);
    f->glClear(GL_DEPTH_BUFFER_BIT);

    f->glEnable(GL_DEPTH_TEST);

    // Bind pick shader
    m_pickShader.bind();
    m_pickShader.setUniformMatrix4("u_viewMatrix", view);
    m_pickShader.setUniformMatrix4("u_projMatrix", projection);

    // --- Draw geometry buffer ---
    if(geom_buffer.vertexCount() > 0) {
        geom_buffer.bindForDraw();

        if(geom_buffer.hasIndices()) {
            f->glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(geom_buffer.indexCount()),
                              GL_UNSIGNED_INT, nullptr);
        } else {
            f->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(geom_buffer.vertexCount()));
        }

        geom_buffer.unbind();
    }

    // --- Draw mesh buffer ---
    if(mesh_buffer.vertexCount() > 0) {
        mesh_buffer.bindForDraw();

        if(mesh_buffer.hasIndices()) {
            f->glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh_buffer.indexCount()),
                              GL_UNSIGNED_INT, nullptr);
        } else {
            f->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh_buffer.vertexCount()));
        }

        mesh_buffer.unbind();
    }

    m_pickShader.release();

    // Read pick ID at the clicked pixel
    uint64_t pickResult = m_fbo.readPickId(pixel_x, pixel_y);

    m_fbo.unbind();

    return pickResult;
}

} // namespace OpenGeoLab::Render
