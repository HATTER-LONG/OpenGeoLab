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
// Vertex Layout Metadata (compile-time checks)
// =============================================================================

namespace {
// Verify RenderVertex layout at compile time
static_assert(sizeof(RenderVertex) == 48, "RenderVertex must be exactly 48 bytes");
static_assert(offsetof(RenderVertex, m_position) == 0, "position offset must be 0");
static_assert(offsetof(RenderVertex, m_normal) == 12, "normal offset must be 12");
static_assert(offsetof(RenderVertex, m_color) == 24, "color offset must be 24");
static_assert(offsetof(RenderVertex, m_pickId) == 40, "pickId offset must be 40");
} // namespace

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

    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if(!ctx) {
        LOG_ERROR("GpuBuffer: Cannot initialize — no current GL context");
        return;
    }

    QOpenGLExtraFunctions* f = ctx->extraFunctions();

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

bool GpuBuffer::upload(const RenderPassData& data) {
    // Check if re-upload is actually needed (version-based detection)
    if(!data.needsUpload(m_uploadedDataVersion)) {
        LOG_TRACE("GpuBuffer: Skipping upload - data version unchanged");
        return true; // Already uploaded this version
    }

    if(!m_initialized) {
        LOG_ERROR("GpuBuffer: upload() called before initialize()");
        return false;
    }

    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if(!ctx) {
        LOG_ERROR("GpuBuffer: Cannot upload — no current GL context");
        return false;
    }

    QOpenGLExtraFunctions* f = ctx->extraFunctions();

    // Early return for empty data (common for skipped geometry)
    if(data.m_vertices.empty()) {
        LOG_DEBUG("GpuBuffer: Skipping upload of empty vertex data");
        m_vertexCount = 0;
        m_indexCount = 0;
        m_uploadedDataVersion = data.m_version; // Mark as uploaded
        return true;                            // Empty data is valid
    }

    m_vertexCount = static_cast<uint32_t>(data.m_vertices.size());
    m_indexCount = static_cast<uint32_t>(data.m_indices.size());

    f->glBindVertexArray(m_vao);

    // ── VBO ──────────────────────────────────────────────────────────────

    f->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    f->glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vertexCount * sizeof(RenderVertex)),
                    data.m_vertices.data(), GL_STATIC_DRAW);

    // Check for GL error after VBO upload
    GLenum err = f->glGetError();
    if(err != GL_NO_ERROR) {
        LOG_ERROR("GpuBuffer: glBufferData failed (VBO): GL error 0x{:X}", err);
        f->glBindVertexArray(0);
        return false;
    }

    constexpr GLsizei stride = sizeof(RenderVertex); // 48 bytes

    // location 0: position — vec3 (3 x float, offset 0)
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                             reinterpret_cast<const void*>(offsetof(RenderVertex, m_position)));

    // location 1: normal — vec3 (3 x float, offset 12)
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                             reinterpret_cast<const void*>(offsetof(RenderVertex, m_normal)));

    // location 2: color — vec4 (4 x float, offset 24)
    f->glEnableVertexAttribArray(2);
    f->glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
                             reinterpret_cast<const void*>(offsetof(RenderVertex, m_color)));

    // location 3: pickId — uvec2 (2 x GL_UNSIGNED_INT, offset 40)
    //   uint64_t on CPU is passed as two uint32_t components to the GPU.
    //   Must use glVertexAttribIPointer for integer attributes.
    //   Note: This assumes little-endian encoding; on big-endian systems,
    //   the pickId values in the shader would be byte-reversed.
    f->glEnableVertexAttribArray(3);
    f->glVertexAttribIPointer(3, 2, GL_UNSIGNED_INT, stride,
                              reinterpret_cast<const void*>(offsetof(RenderVertex, m_pickId)));

    // Check for GL error after vertex attribute setup
    err = f->glGetError();
    if(err != GL_NO_ERROR) {
        LOG_ERROR("GpuBuffer: Vertex attribute pointer setup failed: GL error 0x{:X}", err);
        f->glBindVertexArray(0);
        return false;
    }

    // ── IBO ──────────────────────────────────────────────────────────────
    //
    // When an IBO is bound to GL_ELEMENT_ARRAY_BUFFER while a VAO is active,
    // the VAO records that binding. This is correct and intentional —
    // leave IBO bound so that subsequent draw calls use indexed geometry.

    if(m_indexCount > 0) {
        f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        f->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                        static_cast<GLsizeiptr>(m_indexCount * sizeof(uint32_t)),
                        data.m_indices.data(), GL_STATIC_DRAW);

        // Check for GL error after IBO upload
        err = f->glGetError();
        if(err != GL_NO_ERROR) {
            LOG_ERROR("GpuBuffer: glBufferData failed (IBO): GL error 0x{:X}", err);
            f->glBindVertexArray(0);
            return false;
        }
    }

    f->glBindVertexArray(0);

    LOG_DEBUG("GpuBuffer: Uploaded {} vertices, {} indices (version={})", m_vertexCount,
              m_indexCount, data.m_version);

    // Mark this version as uploaded
    m_uploadedDataVersion = data.m_version;
    return true;
}

// =============================================================================
// Bind / Unbind
// =============================================================================

void GpuBuffer::bindForDraw() {
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if(!ctx) {
        LOG_ERROR("GpuBuffer: Cannot bindForDraw() — no current GL context");
        return;
    }
    QOpenGLExtraFunctions* f = ctx->extraFunctions();
    f->glBindVertexArray(m_vao);
}

void GpuBuffer::unbind() {
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if(!ctx) {
        LOG_ERROR("GpuBuffer: Cannot unbind() — no current GL context");
        return;
    }
    QOpenGLExtraFunctions* f = ctx->extraFunctions();
    f->glBindVertexArray(0);
}

} // namespace OpenGeoLab::Render
