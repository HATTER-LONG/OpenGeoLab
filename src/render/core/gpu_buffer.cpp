/**
 * @file gpu_buffer.cpp
 * @brief GpuBuffer implementation — VAO/VBO/IBO management
 */

#include "gpu_buffer.hpp"

#include "render/render_data.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

namespace OpenGeoLab::Render {

// =============================================================================
// Lifecycle
// =============================================================================

GpuBuffer::~GpuBuffer() { cleanup(); }

// =============================================================================
// Initialize / Cleanup
// =============================================================================

void GpuBuffer::initialize() {
    if(m_initialized) {
        return;
    }

    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();

    f->glGenVertexArrays(1, &m_vao);
    f->glGenBuffers(1, &m_vbo);
    f->glGenBuffers(1, &m_ibo);

    m_initialized = true;
    LOG_DEBUG("GpuBuffer: Initialized (VAO={}, VBO={}, IBO={})", m_vao, m_vbo, m_ibo);
}

void GpuBuffer::cleanup() {
    if(!m_initialized) {
        return;
    }

    auto* ctx = QOpenGLContext::currentContext();
    if(!ctx) {
        LOG_ERROR("GpuBuffer: Cannot cleanup — no current GL context");
        return;
    }

    QOpenGLExtraFunctions* f = ctx->extraFunctions();

    if(m_vao != 0) {
        f->glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if(m_vbo != 0) {
        f->glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if(m_ibo != 0) {
        f->glDeleteBuffers(1, &m_ibo);
        m_ibo = 0;
    }

    m_vertexCount = 0;
    m_indexCount = 0;
    m_initialized = false;
    LOG_DEBUG("GpuBuffer: Cleaned up");
}

// =============================================================================
// Upload
// =============================================================================

void GpuBuffer::upload(const RenderPassData& data) {
    if(!m_initialized) {
        LOG_ERROR("GpuBuffer: upload() called before initialize()");
        return;
    }

    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();

    m_vertexCount = static_cast<uint32_t>(data.m_vertices.size());
    m_indexCount = static_cast<uint32_t>(data.m_indices.size());

    f->glBindVertexArray(m_vao);

    // ── VBO ──────────────────────────────────────────────────────────────

    f->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    f->glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vertexCount * sizeof(RenderVertex)),
                    data.m_vertices.data(), GL_STATIC_DRAW);

    constexpr GLsizei stride = sizeof(RenderVertex); // 48 bytes

    // location 0: position — vec3 (3 x float, offset 0)
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(0));

    // location 1: normal — vec3 (3 x float, offset 12)
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(12));

    // location 2: color — vec4 (4 x float, offset 24)
    f->glEnableVertexAttribArray(2);
    f->glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(24));

    // location 3: pickId — uvec2 (2 x GL_UNSIGNED_INT, offset 40)
    //   uint64_t on CPU is passed as two uint32_t components to the GPU.
    //   Must use glVertexAttribIPointer for integer attributes.
    f->glEnableVertexAttribArray(3);
    f->glVertexAttribIPointer(3, 2, GL_UNSIGNED_INT, stride, reinterpret_cast<const void*>(40));

    // ── IBO ──────────────────────────────────────────────────────────────

    if(m_indexCount > 0) {
        f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        f->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                        static_cast<GLsizeiptr>(m_indexCount * sizeof(uint32_t)),
                        data.m_indices.data(), GL_STATIC_DRAW);
    }

    f->glBindVertexArray(0);

    LOG_DEBUG("GpuBuffer: Uploaded {} vertices, {} indices", m_vertexCount, m_indexCount);
}

// =============================================================================
// Bind / Unbind
// =============================================================================

void GpuBuffer::bindForDraw() {
    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();
    f->glBindVertexArray(m_vao);
}

void GpuBuffer::unbind() {
    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();
    f->glBindVertexArray(0);
}

} // namespace OpenGeoLab::Render
