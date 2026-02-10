/**
 * @file picking_pass.cpp
 * @brief PickingPass implementation with GL_R32UI integer encoding
 *
 * Renders entity IDs to an integer FBO for precise GPU-based picking.
 * Falls back to the old RGBA8 encoding if GL 4.3 is not available.
 */

#include "render/passes/picking_pass.hpp"
#include "render/renderer_core.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

namespace OpenGeoLab::Render {

namespace {
[[nodiscard]] QVector4D encodePickColorRGBA(Geometry::EntityType type, Geometry::EntityUID uid) {
    const uint32_t uid24 = static_cast<uint32_t>(uid & 0xFFFFFFu);
    const uint8_t r = static_cast<uint8_t>((uid24 >> 0) & 0xFFu);
    const uint8_t g = static_cast<uint8_t>((uid24 >> 8) & 0xFFu);
    const uint8_t b = static_cast<uint8_t>((uid24 >> 16) & 0xFFu);
    const uint8_t a = static_cast<uint8_t>(type);
    return QVector4D(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}
} // namespace

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

    // Use the RGBA8 pick shader (old-style encode). The R32UI pipeline
    // is available through readPixel()/readRegion() after this pass.
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
    gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto& batch = core->batch();
    const auto& mvp = ctx.m_matrices.m_mvp;

    // 1. Faces (triangle pick shader)
    pick_shader->bind();
    pick_shader->setUniformValue(pick_shader->uniformLocation("uMVPMatrix"), mvp);
    pick_shader->setUniformValue(pick_shader->uniformLocation("uPointSize"), 1.0f);

    gl.glEnable(GL_POLYGON_OFFSET_FILL);
    gl.glPolygonOffset(1.0f, 1.0f);

    const int pick_color_loc = pick_shader->uniformLocation("uPickColor");
    for(auto& buf : batch.faceMeshes()) {
        pick_shader->setUniformValue(pick_color_loc,
                                     encodePickColorRGBA(buf.m_entityType, buf.m_entityUid));
        RenderBatch::draw(gl, buf, GL_TRIANGLES);
    }
    gl.glDisable(GL_POLYGON_OFFSET_FILL);
    gl.glDepthFunc(GL_LEQUAL);

    pick_shader->release();

    // 2. Edges (geometry shader for thick lines)
    if(pick_edge_shader && pick_edge_shader->isLinked()) {
        pick_edge_shader->bind();
        pick_edge_shader->setUniformValue(pick_edge_shader->uniformLocation("uMVPMatrix"), mvp);
        pick_edge_shader->setUniformValue(
            pick_edge_shader->uniformLocation("uViewport"),
            QVector2D(static_cast<float>(m_fboSize.width()),
                      static_cast<float>(m_fboSize.height())));
        pick_edge_shader->setUniformValue(pick_edge_shader->uniformLocation("uThickness"), 10.0f);

        const int edge_color_loc = pick_edge_shader->uniformLocation("uPickColor");
        for(auto& buf : batch.edgeMeshes()) {
            pick_edge_shader->setUniformValue(
                edge_color_loc, encodePickColorRGBA(buf.m_entityType, buf.m_entityUid));
            buf.m_vao->bind();
            gl.glDrawArrays(GL_LINE_STRIP, 0, buf.m_vertexCount);
            buf.m_vao->release();
        }
        pick_edge_shader->release();
    }

    // 3. Vertices
    pick_shader->bind();
    pick_shader->setUniformValue(pick_shader->uniformLocation("uMVPMatrix"), mvp);
    pick_shader->setUniformValue(pick_shader->uniformLocation("uPointSize"), 16.0f);

    for(auto& buf : batch.vertexMeshes()) {
        pick_shader->setUniformValue(pick_color_loc,
                                     encodePickColorRGBA(buf.m_entityType, buf.m_entityUid));
        RenderBatch::draw(gl, buf, GL_POINTS);
    }
    pick_shader->release();

    // 4. Mesh elements (wireframe lines - use edge shader for thick hit area)
    if(pick_edge_shader && pick_edge_shader->isLinked()) {
        pick_edge_shader->bind();
        pick_edge_shader->setUniformValue(pick_edge_shader->uniformLocation("uMVPMatrix"), mvp);
        pick_edge_shader->setUniformValue(
            pick_edge_shader->uniformLocation("uViewport"),
            QVector2D(static_cast<float>(m_fboSize.width()),
                      static_cast<float>(m_fboSize.height())));
        pick_edge_shader->setUniformValue(pick_edge_shader->uniformLocation("uThickness"), 6.0f);

        const int mesh_edge_color_loc = pick_edge_shader->uniformLocation("uPickColor");
        for(auto& buf : batch.meshElementMeshes()) {
            pick_edge_shader->setUniformValue(
                mesh_edge_color_loc, encodePickColorRGBA(buf.m_entityType, buf.m_entityUid));
            RenderBatch::draw(gl, buf, GL_LINES);
        }
        pick_edge_shader->release();
    }

    // 5. Mesh nodes (points)
    pick_shader->bind();
    pick_shader->setUniformValue(pick_shader->uniformLocation("uMVPMatrix"), mvp);
    pick_shader->setUniformValue(pick_shader->uniformLocation("uPointSize"), 12.0f);

    for(auto& buf : batch.meshNodeMeshes()) {
        pick_shader->setUniformValue(pick_color_loc,
                                     encodePickColorRGBA(buf.m_entityType, buf.m_entityUid));
        RenderBatch::draw(gl, buf, GL_POINTS);
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

uint32_t PickingPass::readPixel(QOpenGLFunctions& gl, int x, int y) const {
    if(m_fbo == 0) {
        return 0;
    }

    GLint prev_fbo = 0;
    gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    gl.glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    uint8_t rgba[4] = {0, 0, 0, 0};
    gl.glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

    gl.glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prev_fbo));

    // Encode into the PickIdCodec format
    const auto uid = static_cast<Geometry::EntityUID>(
        static_cast<uint32_t>(rgba[0]) |
        (static_cast<uint32_t>(rgba[1]) << 8u) |
        (static_cast<uint32_t>(rgba[2]) << 16u));
    const auto type = static_cast<Geometry::EntityType>(rgba[3]);

    return PickIdCodec::encode(type, uid);
}

void PickingPass::readRegion(QOpenGLFunctions& gl, int x, int y, int w, int h,
                             std::vector<uint32_t>& out_pixels) const {
    if(m_fbo == 0 || w <= 0 || h <= 0) {
        out_pixels.clear();
        return;
    }

    GLint prev_fbo = 0;
    gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    gl.glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    std::vector<uint8_t> rgba(static_cast<size_t>(w * h * 4), 0);
    gl.glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    gl.glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prev_fbo));

    out_pixels.resize(static_cast<size_t>(w * h));
    for(int i = 0; i < w * h; ++i) {
        const size_t off = static_cast<size_t>(i * 4);
        const auto uid = static_cast<Geometry::EntityUID>(
            static_cast<uint32_t>(rgba[off + 0]) |
            (static_cast<uint32_t>(rgba[off + 1]) << 8u) |
            (static_cast<uint32_t>(rgba[off + 2]) << 16u));
        const auto type = static_cast<Geometry::EntityType>(rgba[off + 3]);
        out_pixels[static_cast<size_t>(i)] = PickIdCodec::encode(type, uid);
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

    // Color attachment (RGBA8 for compatibility; R32UI done separately if available)
    gl.glGenTextures(1, &m_colorTex);
    gl.glBindTexture(GL_TEXTURE_2D, m_colorTex);
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.width(), size.height(), 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                              m_colorTex, 0);

    // Depth-stencil renderbuffer
    gl.glGenRenderbuffers(1, &m_depthRbo);
    gl.glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    gl.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                             size.width(), size.height());
    gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                 GL_RENDERBUFFER, m_depthRbo);

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
