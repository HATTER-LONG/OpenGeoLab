/**
 * @file outline_highlight.cpp
 * @brief OutlineHighlight implementation - stencil-based outline rendering
 *
 * Uses batched entity info maps to find matching entities by uid56,
 * then draws their index/vertex ranges for stencil outline generation.
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

    struct MatchInfo {
        uint32_t m_indexOffset;
        uint32_t m_indexCount;
        QVector3D m_centroid;
    };

    // --- Face matching ---
    std::vector<MatchInfo> face_matches;
    auto& face_buf = batch.faceBuffer();
    if(face_buf.isValid()) {
        for(const auto& [packed_uid, info] : batch.faceEntities()) {
            const auto entity_type = info.m_uid.type();
            const auto uid56 = info.m_uid.uid56();

            bool match = false;
            if(entry.m_type == entity_type) {
                match = (uid56 == entry.m_uid56);
            } else if(entry.m_type == RenderEntityType::Part) {
                match = (info.m_owningPartUid56 == entry.m_uid56 && entry.m_uid56 != 0);
            } else if(entry.m_type == RenderEntityType::Solid) {
                match = (info.m_owningSolidUid56 == entry.m_uid56 && entry.m_uid56 != 0);
            }

            if(match && info.m_indexCount > 0) {
                face_matches.push_back(
                    {info.m_indexOffset, info.m_indexCount,
                     QVector3D(info.m_centroid[0], info.m_centroid[1], info.m_centroid[2])});
            }
        }
    }

    // --- Edge matching (for Wire/Edge/Part/Solid selection) ---
    std::vector<MatchInfo> edge_matches;
    auto& edge_buf = batch.edgeBuffer();
    if(edge_buf.isValid()) {
        for(const auto& [packed_uid, info] : batch.edgeEntities()) {
            const auto entity_type = info.m_uid.type();
            const auto uid56 = info.m_uid.uid56();

            bool match = false;
            if(entry.m_type == entity_type) {
                match = (uid56 == entry.m_uid56);
            } else if(entry.m_type == RenderEntityType::Wire) {
                for(const auto& wire_uid : info.m_owningWireUid56s) {
                    if(wire_uid == entry.m_uid56 && entry.m_uid56 != 0) {
                        match = true;
                        break;
                    }
                }
            } else if(entry.m_type == RenderEntityType::Part) {
                match = (info.m_owningPartUid56 == entry.m_uid56 && entry.m_uid56 != 0);
            } else if(entry.m_type == RenderEntityType::Solid) {
                match = (info.m_owningSolidUid56 == entry.m_uid56 && entry.m_uid56 != 0);
            }

            if(match && info.m_indexCount > 0) {
                edge_matches.push_back(
                    {info.m_indexOffset, info.m_indexCount,
                     QVector3D(info.m_centroid[0], info.m_centroid[1], info.m_centroid[2])});
            }
        }
    }

    // --- Vertex matching ---
    struct VertexMatchInfo {
        uint32_t m_vertexOffset;
        uint32_t m_vertexCount;
    };
    std::vector<VertexMatchInfo> vertex_matches;
    auto& vertex_buf = batch.vertexBuffer();
    if(vertex_buf.isValid() && entry.m_type == RenderEntityType::Vertex) {
        for(const auto& [packed_uid, info] : batch.vertexEntities()) {
            const auto uid56 = info.m_uid.uid56();
            if(uid56 == entry.m_uid56 && info.m_vertexCount > 0) {
                vertex_matches.push_back({info.m_vertexOffset, info.m_vertexCount});
            }
        }
    }

    if(face_matches.empty() && edge_matches.empty() && vertex_matches.empty()) {
        return;
    }

    outline_shader->bind();
    outline_shader->setUniformValue(m_outlineMvpLoc, ctx.m_matrices.m_mvp);
    outline_shader->setUniformValue(m_outlineColorLoc, color);

    // --- Draw face outlines (stencil-based) ---
    if(!face_matches.empty()) {
        gl.glEnable(GL_STENCIL_TEST);
        gl.glEnable(GL_DEPTH_TEST);

        // Pass 1: Write 1 to stencil where the entity is drawn
        gl.glStencilFunc(GL_ALWAYS, 1, 0xFF);
        gl.glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        gl.glStencilMask(0xFF);
        gl.glClear(GL_STENCIL_BUFFER_BIT);
        gl.glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        for(const auto& m : face_matches) {
            outline_shader->setUniformValue(m_outlineCenterLoc, m.m_centroid);
            outline_shader->setUniformValue(m_outlineScaleLoc, 1.0f);
            RenderBatch::drawIndexRange(gl, face_buf, m.m_indexOffset, m.m_indexCount);
        }

        // Pass 2: Draw scaled version where stencil != 1
        gl.glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        gl.glStencilMask(0x00);
        gl.glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        gl.glDepthMask(GL_FALSE);

        const float scale = 1.0f + m_outlineWidth * 0.005f;

        for(const auto& m : face_matches) {
            outline_shader->setUniformValue(m_outlineCenterLoc, m.m_centroid);
            outline_shader->setUniformValue(m_outlineScaleLoc, scale);
            RenderBatch::drawIndexRange(gl, face_buf, m.m_indexOffset, m.m_indexCount);
        }

        gl.glDepthMask(GL_TRUE);
        gl.glStencilMask(0xFF);
        gl.glStencilFunc(GL_ALWAYS, 0, 0xFF);
        gl.glDisable(GL_STENCIL_TEST);
    }

    // --- Draw edge highlights (solid color, thick line) ---
    if(!edge_matches.empty()) {
        gl.glEnable(GL_DEPTH_TEST);
        gl.glDepthFunc(GL_LEQUAL);

        outline_shader->setUniformValue(m_outlineScaleLoc, 1.0f);
        outline_shader->setUniformValue(m_outlineCenterLoc, QVector3D(0.0f, 0.0f, 0.0f));
        gl.glLineWidth(4.0f);

        for(const auto& m : edge_matches) {
            RenderBatch::drawIndexRange(gl, edge_buf, m.m_indexOffset, m.m_indexCount);
        }

        gl.glLineWidth(1.0f);
        gl.glDepthFunc(GL_LESS);
    }

    // --- Draw vertex highlights (solid color, large point) ---
    if(!vertex_matches.empty()) {
        gl.glEnable(GL_DEPTH_TEST);
        gl.glDepthFunc(GL_LEQUAL);

        outline_shader->setUniformValue(m_outlineScaleLoc, 1.0f);
        outline_shader->setUniformValue(m_outlineCenterLoc, QVector3D(0.0f, 0.0f, 0.0f));

        for(const auto& vm : vertex_matches) {
            RenderBatch::drawVertexRange(gl, vertex_buf, vm.m_vertexOffset, vm.m_vertexCount,
                                         GL_POINTS);
        }

        gl.glDepthFunc(GL_LESS);
    }

    outline_shader->release();
    gl.glEnable(GL_DEPTH_TEST);
}

} // namespace OpenGeoLab::Render
