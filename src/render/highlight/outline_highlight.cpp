/**
 * @file outline_highlight.cpp
 * @brief OutlineHighlight implementation - stencil-based outline rendering
 */

#include "render/highlight/outline_highlight.hpp"
#include "render/renderer_core.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Render {

void OutlineHighlight::initialize(QOpenGLFunctions& gl) {
    (void)gl;
    LOG_DEBUG("OutlineHighlight: Initialized");
}

void OutlineHighlight::resize(QOpenGLFunctions& gl, const QSize& size) {
    (void)gl;
    (void)size;
}

void OutlineHighlight::cleanup(QOpenGLFunctions& gl) {
    (void)gl;
    m_outlineShader.reset();
    LOG_DEBUG("OutlineHighlight: Cleanup");
}

void OutlineHighlight::render(QOpenGLFunctions& gl,
                              const RenderPassContext& ctx,
                              RenderBatch& batch,
                              const HighlightSet& highlights) {
    if(highlights.empty()) {
        return;
    }

    // Render selection outlines
    for(const auto& entry : highlights.m_selected) {
        renderOutline(gl, ctx, batch, entry, m_selectionColor);
    }

    // Render hover outlines (on top of selection)
    for(const auto& entry : highlights.m_hover) {
        renderOutline(gl, ctx, batch, entry, m_hoverColor);
    }
}

void OutlineHighlight::renderOutline(QOpenGLFunctions& gl,
                                     const RenderPassContext& ctx,
                                     RenderBatch& batch,
                                     const HighlightSet::Entry& entry,
                                     const QVector4D& color) {
    auto* core = ctx.m_core;
    if(!core) {
        return;
    }

    auto* outline_shader = core->shader("outline");
    if(!outline_shader || !outline_shader->isLinked()) {
        return;
    }

    if(m_outlineMvpLoc < 0) {
        m_outlineMvpLoc = outline_shader->uniformLocation("uMVPMatrix");
        m_outlineColorLoc = outline_shader->uniformLocation("uColor");
        m_outlineScaleLoc = outline_shader->uniformLocation("uScale");
        m_outlineCenterLoc = outline_shader->uniformLocation("uCenter");
    }

    // Stencil-based outline:
    // 1. Draw the entity to stencil buffer at original position
    // 2. Draw a slightly scaled version (from centroid), only where stencil != 1

    gl.glEnable(GL_STENCIL_TEST);
    gl.glEnable(GL_DEPTH_TEST);

    // Lambda that draws matching buffers and sets per-buffer centroid uniform
    auto drawMatchingWithCenter = [&](auto& buffers, float scale) {
        for(auto& buf : buffers) {
            bool matches = false;
            if(entry.m_type == buf.m_entityType) {
                matches = (static_cast<uint32_t>(buf.m_entityUid) & 0xFFFFFFu) ==
                          (static_cast<uint32_t>(entry.m_uid) & 0xFFFFFFu);
            } else if(entry.m_type == Geometry::EntityType::Part) {
                matches = (static_cast<uint32_t>(buf.m_owningPartUid) & 0xFFFFFFu) ==
                          (static_cast<uint32_t>(entry.m_uid) & 0xFFFFFFu);
            } else if(entry.m_type == Geometry::EntityType::Solid) {
                matches = (static_cast<uint32_t>(buf.m_owningSolidUid) & 0xFFFFFFu) ==
                          (static_cast<uint32_t>(entry.m_uid) & 0xFFFFFFu);
            }

            if(matches) {
                outline_shader->setUniformValue(m_outlineCenterLoc, buf.m_centroid);
                outline_shader->setUniformValue(m_outlineScaleLoc, scale);
                RenderBatch::draw(gl, buf);
            }
        }
    };

    // Pass 1: Write 1 to stencil where the entity is drawn
    gl.glStencilFunc(GL_ALWAYS, 1, 0xFF);
    gl.glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    gl.glStencilMask(0xFF);
    gl.glClear(GL_STENCIL_BUFFER_BIT);
    gl.glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    outline_shader->bind();
    outline_shader->setUniformValue(m_outlineMvpLoc, ctx.m_matrices.m_mvp);
    outline_shader->setUniformValue(m_outlineColorLoc, color);

    drawMatchingWithCenter(batch.faceMeshes(), 1.0f);

    // Pass 2: Draw scaled version where stencil != 1
    gl.glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    gl.glStencilMask(0x00);
    gl.glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    // Keep depth testing enabled so outlines are depth-tested against scene
    // but avoid writing to the depth buffer so the outline doesn't block other draws
    gl.glEnable(GL_DEPTH_TEST);
    gl.glDepthMask(GL_FALSE);

    // Reduce the scale multiplier to make outline thinner
    const float scale = 1.0f + m_outlineWidth * 0.005f;
    outline_shader->setUniformValue(m_outlineColorLoc, color);

    drawMatchingWithCenter(batch.faceMeshes(), scale);

    // Restore depth write state
    gl.glDepthMask(GL_TRUE);

    outline_shader->release();

    gl.glStencilMask(0xFF);
    gl.glStencilFunc(GL_ALWAYS, 0, 0xFF);
    gl.glDisable(GL_STENCIL_TEST);
    gl.glEnable(GL_DEPTH_TEST);
}

} // namespace OpenGeoLab::Render
