/**
 * @file picking_pass.cpp
 * @brief PickingPass implementation with batched vertex-attribute-based picking
 *
 * Entity IDs are baked into each vertex's m_uid attribute. The pick shader
 * reads aUid per-vertex and outputs it as uvec2 to the RG32UI color
 * attachment. Each category needs only one draw call instead of per-entity
 * uniform calls.
 */

#include "render/passes/picking_pass.hpp"
#include "render/renderer_core.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

namespace OpenGeoLab::Render {

void PickingPass::initialize(QOpenGLFunctions& gl) {
    (void)gl;
    LOG_DEBUG("PickingPass: Initialized");
}

void PickingPass::resize(QOpenGLFunctions& gl, const QSize& size) {
    if(size == m_fboSize && m_fbo != 0) {
        return;
    }
    createFbo(gl, size);
}

void PickingPass::execute(QOpenGLFunctions& gl, const RenderPassContext& ctx) {
    auto* core = ctx.m_core;
    if(!core || core->batch().empty()) {
        return;
    }

    auto* pick_shader = core->shader("pick");
    auto* pick_edge_shader = core->shader("pick_edge");
    if(!pick_shader || !pick_shader->isLinked()) {
        return;
    }

    // Bind the pick FBO
    GLint prev_fbo = 0;
    gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    m_prevFbo = prev_fbo;

    if(m_fbo == 0) {
        createFbo(gl, ctx.m_viewportSize);
    }
    gl.glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    gl.glViewport(0, 0, m_fboSize.width(), m_fboSize.height());
    gl.glDisable(GL_BLEND);
    gl.glDisable(GL_MULTISAMPLE);
    gl.glEnable(GL_DEPTH_TEST);
    gl.glDepthMask(GL_TRUE);
    gl.glEnable(GL_PROGRAM_POINT_SIZE);
    gl.glDepthFunc(GL_LESS);

    gl.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl.glClear(GL_DEPTH_BUFFER_BIT);

    // Clear the integer color attachment with zeros (RG32UI requires glClearBufferuiv)
    auto* glExtra = QOpenGLContext::currentContext()->extraFunctions();
    const GLuint clear_color[4] = {0, 0, 0, 0};
    glExtra->glClearBufferuiv(GL_COLOR, 0, clear_color);

    auto& batch = core->batch();
    const auto& mvp = ctx.m_matrices.m_mvp;

    // 1. Faces — single draw call (uid baked in vertex attribute)
    pick_shader->bind();
    pick_shader->setUniformValue(pick_shader->uniformLocation("uMVPMatrix"), mvp);
    pick_shader->setUniformValue(pick_shader->uniformLocation("uPointSize"), 1.0f);

    gl.glEnable(GL_POLYGON_OFFSET_FILL);
    gl.glPolygonOffset(1.0f, 1.0f);

    if(batch.faceBuffer().isValid()) {
        RenderBatch::drawAll(gl, batch.faceBuffer());
    }

    gl.glDisable(GL_POLYGON_OFFSET_FILL);
    gl.glDepthFunc(GL_LEQUAL);

    pick_shader->release();

    // 2. Edges — single draw call (geometry shader for thick lines)
    if(pick_edge_shader && pick_edge_shader->isLinked() && batch.edgeBuffer().isValid()) {
        pick_edge_shader->bind();
        pick_edge_shader->setUniformValue(pick_edge_shader->uniformLocation("uMVPMatrix"), mvp);
        pick_edge_shader->setUniformValue(pick_edge_shader->uniformLocation("uViewport"),
                                          QVector2D(static_cast<float>(m_fboSize.width()),
                                                    static_cast<float>(m_fboSize.height())));
        pick_edge_shader->setUniformValue(pick_edge_shader->uniformLocation("uThickness"), 10.0f);

        RenderBatch::drawAll(gl, batch.edgeBuffer());

        pick_edge_shader->release();
    }

    // 3. Vertices — single draw call
    pick_shader->bind();
    pick_shader->setUniformValue(pick_shader->uniformLocation("uMVPMatrix"), mvp);
    pick_shader->setUniformValue(pick_shader->uniformLocation("uPointSize"), 16.0f);

    if(batch.vertexBuffer().isValid()) {
        RenderBatch::drawAll(gl, batch.vertexBuffer(), GL_POINTS);
    }

    pick_shader->release();

    // 4. Mesh elements — single draw call (use edge shader for thick hit area)
    if(pick_edge_shader && pick_edge_shader->isLinked() && batch.meshElementBuffer().isValid()) {
        pick_edge_shader->bind();
        pick_edge_shader->setUniformValue(pick_edge_shader->uniformLocation("uMVPMatrix"), mvp);
        pick_edge_shader->setUniformValue(pick_edge_shader->uniformLocation("uViewport"),
                                          QVector2D(static_cast<float>(m_fboSize.width()),
                                                    static_cast<float>(m_fboSize.height())));
        pick_edge_shader->setUniformValue(pick_edge_shader->uniformLocation("uThickness"), 6.0f);

        RenderBatch::drawAll(gl, batch.meshElementBuffer());

        pick_edge_shader->release();
    }

    // 5. Mesh nodes — single draw call
    pick_shader->bind();
    pick_shader->setUniformValue(pick_shader->uniformLocation("uMVPMatrix"), mvp);
    pick_shader->setUniformValue(pick_shader->uniformLocation("uPointSize"), 12.0f);

    if(batch.meshNodeBuffer().isValid()) {
        RenderBatch::drawAll(gl, batch.meshNodeBuffer(), GL_POINTS);
    }

    pick_shader->release();

    gl.glDepthFunc(GL_LESS);

    // Restore previous FBO
    gl.glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prev_fbo));
}

void PickingPass::cleanup(QOpenGLFunctions& gl) {
    destroyFbo(gl);
    m_pickShader.reset();
    m_pickEdgeShader.reset();
    LOG_DEBUG("PickingPass: Cleanup");
}

uint64_t PickingPass::readPixel(QOpenGLFunctions& gl, int x, int y) const {
    if(m_fbo == 0) {
        return 0;
    }

    GLint prev_fbo = 0;
    gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    gl.glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    uint32_t rg[2] = {0, 0};
    gl.glReadPixels(x, y, 1, 1, GL_RG_INTEGER, GL_UNSIGNED_INT, rg);

    gl.glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prev_fbo));

    // Reconstruct 64-bit packed value: rg[0] = low 32 bits, rg[1] = high 32 bits
    const uint64_t packed = (static_cast<uint64_t>(rg[1]) << 32) | static_cast<uint64_t>(rg[0]);
    return packed;
}

void PickingPass::readRegion(
    QOpenGLFunctions& gl, int x, int y, int w, int h, std::vector<uint64_t>& out_pixels) const {
    if(m_fbo == 0 || w <= 0 || h <= 0) {
        out_pixels.clear();
        return;
    }

    GLint prev_fbo = 0;
    gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    gl.glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Each pixel has 2 uint32 components (R, G) in RG32UI format
    const size_t pixel_count = static_cast<size_t>(w * h);
    std::vector<uint32_t> rg_data(pixel_count * 2, 0);
    gl.glReadPixels(x, y, w, h, GL_RG_INTEGER, GL_UNSIGNED_INT, rg_data.data());

    gl.glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prev_fbo));

    out_pixels.resize(pixel_count);
    for(size_t i = 0; i < pixel_count; ++i) {
        const uint32_t lo = rg_data[i * 2 + 0];
        const uint32_t hi = rg_data[i * 2 + 1];
        out_pixels[i] = (static_cast<uint64_t>(hi) << 32) | static_cast<uint64_t>(lo);
    }
}

void PickingPass::bindFbo(QOpenGLFunctions& gl) const {
    if(m_fbo != 0) {
        gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_prevFbo);
        gl.glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    }
}

void PickingPass::unbindFbo(QOpenGLFunctions& gl) const {
    gl.glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(m_prevFbo));
    m_prevFbo = 0;
}

void PickingPass::createFbo(QOpenGLFunctions& gl, const QSize& size) {
    destroyFbo(gl);

    m_fboSize = size;

    gl.glGenFramebuffers(1, &m_fbo);
    gl.glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Color attachment (RG32UI for 64-bit pick IDs)
    gl.glGenTextures(1, &m_colorTex);
    gl.glBindTexture(GL_TEXTURE_2D, m_colorTex);
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, size.width(), size.height(), 0, GL_RG_INTEGER,
                    GL_UNSIGNED_INT, nullptr);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);

    // Depth-stencil renderbuffer
    gl.glGenRenderbuffers(1, &m_depthRbo);
    gl.glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    gl.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.width(), size.height());
    gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                 m_depthRbo);

    const GLenum status = gl.glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("PickingPass: FBO incomplete, status={}", status);
    }

    gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);

    LOG_DEBUG("PickingPass: Created FBO {}x{}", size.width(), size.height());
}

void PickingPass::destroyFbo(QOpenGLFunctions& gl) {
    if(m_colorTex) {
        gl.glDeleteTextures(1, &m_colorTex);
        m_colorTex = 0;
    }
    if(m_depthRbo) {
        gl.glDeleteRenderbuffers(1, &m_depthRbo);
        m_depthRbo = 0;
    }
    if(m_fbo) {
        gl.glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    m_fboSize = QSize();
}

} // namespace OpenGeoLab::Render
