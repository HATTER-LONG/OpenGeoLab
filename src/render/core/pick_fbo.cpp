/**
 * @file pick_fbo.cpp
 * @brief PickFBO implementation — GL_RG32UI framebuffer for GPU picking
 */

#include "pick_fbo.hpp"

#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

#include <algorithm>

namespace OpenGeoLab::Render {

PickFBO::~PickFBO() { cleanup(); }

void PickFBO::initialize(int width, int height) {
    if(m_initialized) {
        return;
    }

    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if(!ctx) {
        LOG_ERROR("PickFBO: Cannot initialize — no current GL context");
        return;
    }

    m_width = width;
    m_height = height;

    QOpenGLExtraFunctions* f = ctx->extraFunctions();
    f->glGenFramebuffers(1, &m_fbo);

    createAttachments();

    m_initialized = true;
    LOG_DEBUG("PickFBO: Initialized (FBO={}, {}x{})", m_fbo, m_width, m_height);
}

void PickFBO::resize(int width, int height) {
    if(width == m_width && height == m_height) {
        return;
    }
    if(!m_initialized) {
        initialize(width, height);
        return;
    }

    m_width = width;
    m_height = height;

    destroyAttachments();
    createAttachments();

    LOG_DEBUG("PickFBO: Resized to {}x{}", m_width, m_height);
}

void PickFBO::cleanup() {
    if(!m_initialized) {
        return;
    }

    auto* ctx = QOpenGLContext::currentContext();
    if(!ctx) {
        LOG_ERROR("PickFBO: Cannot cleanup — no current GL context");
        return;
    }

    destroyAttachments();

    QOpenGLExtraFunctions* f = ctx->extraFunctions();
    if(m_fbo != 0) {
        f->glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }

    m_initialized = false;
    LOG_DEBUG("PickFBO: Cleaned up");
}

void PickFBO::createAttachments() {
    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();

    f->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Color attachment: GL_RG32UI texture for pickId storage
    f->glGenTextures(1, &m_colorTex);
    f->glBindTexture(GL_TEXTURE_2D, m_colorTex);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, m_width, m_height, 0, GL_RG_INTEGER,
                    GL_UNSIGNED_INT, nullptr);
    // Integer textures require GL_NEAREST filtering
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);

    // Depth attachment: renderbuffer for correct occlusion
    f->glGenRenderbuffers(1, &m_depthRbo);
    f->glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    f->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_width, m_height);
    f->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);

    // Verify completeness
    GLenum status = f->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("PickFBO: Framebuffer incomplete (status=0x{:X})", status);
    }

    f->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PickFBO::destroyAttachments() {
    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();

    if(m_colorTex != 0) {
        f->glDeleteTextures(1, &m_colorTex);
        m_colorTex = 0;
    }
    if(m_depthRbo != 0) {
        f->glDeleteRenderbuffers(1, &m_depthRbo);
        m_depthRbo = 0;
    }
}

void PickFBO::bind() {
    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();

    f->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    f->glViewport(0, 0, m_width, m_height);

    // Clear to zero (background = no entity)
    const GLuint clear_color[2] = {0, 0};
    f->glClearBufferuiv(GL_COLOR, 0, clear_color);
    f->glClear(GL_DEPTH_BUFFER_BIT);
}

void PickFBO::unbind() {
    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();
    f->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::vector<PickPixel> PickFBO::readRegion(int x, int y, int w, int h) const {
    // Clamp region to FBO bounds
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(m_width, x + w);
    int y1 = std::min(m_height, y + h);
    int clamped_w = x1 - x0;
    int clamped_h = y1 - y0;

    if(clamped_w <= 0 || clamped_h <= 0) {
        return {};
    }

    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();

    // Each pixel has 2 uint32 components (RG)
    std::vector<PickPixel> pixels(static_cast<size_t>(clamped_w * clamped_h));

    f->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    f->glReadPixels(x0, y0, clamped_w, clamped_h, GL_RG_INTEGER, GL_UNSIGNED_INT, pixels.data());
    f->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return pixels;
}

} // namespace OpenGeoLab::Render
