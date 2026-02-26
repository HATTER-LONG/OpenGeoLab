/**
 * @file pick_fbo.cpp
 * @brief PickFbo implementation — offscreen FBO for GPU picking
 */

#include "pick_fbo.hpp"

#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

namespace OpenGeoLab::Render {

// =============================================================================
// Lifecycle
// =============================================================================

PickFbo::~PickFbo() { cleanup(); }

// =============================================================================
// Initialize / Resize / Cleanup
// =============================================================================

bool PickFbo::initialize(int w, int h) {
    if(m_initialized) {
        cleanup();
    }

    QOpenGLExtraFunctions* ef = QOpenGLContext::currentContext()->extraFunctions();

    m_width = w;
    m_height = h;

    // ── Color attachment: RG32UI texture ─────────────────────────────────

    ef->glGenTextures(1, &m_colorTex);
    ef->glBindTexture(GL_TEXTURE_2D, m_colorTex);
    ef->glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, m_width, m_height, 0, GL_RG_INTEGER,
                     GL_UNSIGNED_INT, nullptr);
    ef->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    ef->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ef->glBindTexture(GL_TEXTURE_2D, 0);

    // ── Depth renderbuffer ───────────────────────────────────────────────

    ef->glGenRenderbuffers(1, &m_depthRbo);
    ef->glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    ef->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_width, m_height);
    ef->glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // ── Framebuffer ──────────────────────────────────────────────────────

    ef->glGenFramebuffers(1, &m_fbo);
    ef->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    ef->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);
    ef->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);

    GLenum status = ef->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("PickFbo: Framebuffer incomplete (status=0x{:X})", status);
        ef->glBindFramebuffer(GL_FRAMEBUFFER, 0);
        cleanup();
        return false;
    }

    ef->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_initialized = true;
    LOG_DEBUG("PickFbo: Initialized {}x{} (FBO={}, colorTex={}, depthRbo={})", m_width, m_height,
              m_fbo, m_colorTex, m_depthRbo);
    return true;
}

bool PickFbo::resize(int w, int h) {
    if(w == m_width && h == m_height && m_initialized) {
        return true;
    }
    cleanup();
    return initialize(w, h);
}

void PickFbo::cleanup() {
    if(!m_initialized) {
        return;
    }

    auto* ctx = QOpenGLContext::currentContext();
    if(!ctx) {
        LOG_ERROR("PickFbo: Cannot cleanup — no current GL context");
        return;
    }

    QOpenGLExtraFunctions* ef = ctx->extraFunctions();

    if(m_fbo != 0) {
        ef->glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if(m_colorTex != 0) {
        ef->glDeleteTextures(1, &m_colorTex);
        m_colorTex = 0;
    }
    if(m_depthRbo != 0) {
        ef->glDeleteRenderbuffers(1, &m_depthRbo);
        m_depthRbo = 0;
    }

    m_width = 0;
    m_height = 0;
    m_initialized = false;
    LOG_DEBUG("PickFbo: Cleaned up");
}

// =============================================================================
// Bind / Unbind
// =============================================================================

void PickFbo::bind() {
    QOpenGLExtraFunctions* ef = QOpenGLContext::currentContext()->extraFunctions();
    ef->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    ef->glViewport(0, 0, m_width, m_height);
}

void PickFbo::unbind() {
    QOpenGLExtraFunctions* ef = QOpenGLContext::currentContext()->extraFunctions();
    ef->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// =============================================================================
// Pick-id readback
// =============================================================================

uint64_t PickFbo::readPickId(int x, int y) const {
    if(!m_initialized) {
        return 0;
    }

    QOpenGLExtraFunctions* ef = QOpenGLContext::currentContext()->extraFunctions();

    ef->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    uint32_t data[2] = {0, 0};
    ef->glReadPixels(x, y, 1, 1, GL_RG_INTEGER, GL_UNSIGNED_INT, data);

    ef->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    const uint64_t low = data[0];
    const uint64_t high = data[1];
    return (high << 32u) | low;
}

} // namespace OpenGeoLab::Render
