/**
 * @file mesh_pass.cpp
 * @brief MeshPass implementation - batched FEM mesh element and node rendering
 *
 * Uses one draw call per category (elements, nodes) for base rendering,
 * plus sub-draw calls for hover/selection highlighting.
 */

#include "render/passes/mesh_pass.hpp"
#include "render/renderer_core.hpp"
#include "util/logger.hpp"

#include <QVector4D>

namespace OpenGeoLab::Render {

void MeshPass::initialize(QOpenGLFunctions& gl) {
    (void)gl;
    LOG_DEBUG("MeshPass: Initialized");
}

void MeshPass::resize(QOpenGLFunctions& gl, const QSize& size) {
    (void)gl;
    (void)size;
}

void MeshPass::execute(QOpenGLFunctions& gl, const RenderPassContext& ctx) {
    auto* core = ctx.m_core;
    if(!core) {
        return;
    }

    auto* shader = core->shader("mesh");
    if(!shader || !shader->isLinked()) {
        return;
    }

    auto& batch = core->batch();
    if(!batch.meshElementBuffer().isValid() && !batch.meshNodeBuffer().isValid()) {
        return;
    }

    // Cache uniform locations on first use
    if(m_mvpLoc < 0) {
        m_mvpLoc = shader->uniformLocation("uMVPMatrix");
        m_modelLoc = shader->uniformLocation("uModelMatrix");
        m_normalMatLoc = shader->uniformLocation("uNormalMatrix");
        m_lightPosLoc = shader->uniformLocation("uLightPos");
        m_viewPosLoc = shader->uniformLocation("uViewPos");
        m_pointSizeLoc = shader->uniformLocation("uPointSize");
        m_useLightingLoc = shader->uniformLocation("uUseLighting");
        m_useOverrideColorLoc = shader->uniformLocation("uUseOverrideColor");
        m_overrideColorLoc = shader->uniformLocation("uOverrideColor");
    }

    gl.glEnable(GL_DEPTH_TEST);
    gl.glEnable(GL_BLEND);
    gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl.glEnable(GL_PROGRAM_POINT_SIZE);

    const QMatrix4x4 model; // Identity
    const QMatrix3x3 normal_matrix = model.normalMatrix();

    shader->bind();
    shader->setUniformValue(m_mvpLoc, ctx.m_matrices.m_mvp);
    shader->setUniformValue(m_modelLoc, model);
    shader->setUniformValue(m_normalMatLoc, normal_matrix);
    shader->setUniformValue(m_lightPosLoc, ctx.m_cameraPos);
    shader->setUniformValue(m_viewPosLoc, ctx.m_cameraPos);
    shader->setUniformValue(m_useLightingLoc, 0); // Mesh uses flat color, no lighting

    const auto& sm = SelectManager::instance();

    // Render mesh elements (wireframe lines) — one drawAll + sub-draws
    auto& elem_buf = batch.meshElementBuffer();
    if(elem_buf.isValid()) {
        gl.glDepthFunc(GL_LEQUAL);
        gl.glLineWidth(1.5f);

        shader->setUniformValue(m_useOverrideColorLoc, 0);
        RenderBatch::drawAll(gl, elem_buf);

        // Hover/selection overlay for elements
        for(const auto& [packed_uid, info] : batch.meshElementEntities()) {
            const auto uid56 = info.m_uid.uid56();
            const auto entity_type = info.m_uid.type();

            const bool is_selected = sm.containsSelection(uid56, entity_type);
            const bool is_hover =
                (m_hoverType == entity_type) && (uid56 == m_hoverUid56) && (m_hoverUid56 != 0);

            if(is_selected) {
                gl.glLineWidth(3.0f);
                shader->setUniformValue(m_useOverrideColorLoc, 1);
                shader->setUniformValue(m_overrideColorLoc,
                                        QVector4D(info.m_selectedColor[0], info.m_selectedColor[1],
                                                  info.m_selectedColor[2],
                                                  info.m_selectedColor[3]));
                RenderBatch::drawIndexRange(gl, elem_buf, info.m_indexOffset, info.m_indexCount);
            } else if(is_hover) {
                gl.glLineWidth(3.0f);
                shader->setUniformValue(m_useOverrideColorLoc, 1);
                shader->setUniformValue(m_overrideColorLoc,
                                        QVector4D(info.m_hoverColor[0], info.m_hoverColor[1],
                                                  info.m_hoverColor[2], info.m_hoverColor[3]));
                RenderBatch::drawIndexRange(gl, elem_buf, info.m_indexOffset, info.m_indexCount);
            }
        }
    }

    // Render mesh nodes (points) — one drawAll + sub-draws
    auto& node_buf = batch.meshNodeBuffer();
    if(node_buf.isValid()) {
        shader->setUniformValue(m_pointSizeLoc, 4.0f);
        shader->setUniformValue(m_useOverrideColorLoc, 0);
        RenderBatch::drawAll(gl, node_buf, GL_POINTS);

        // Hover/selection overlay for nodes
        for(const auto& [packed_uid, info] : batch.meshNodeEntities()) {
            const auto uid56 = info.m_uid.uid56();
            const auto entity_type = info.m_uid.type();

            const bool is_selected = sm.containsSelection(uid56, entity_type);
            const bool is_hover =
                (m_hoverType == entity_type) && (uid56 == m_hoverUid56) && (m_hoverUid56 != 0);

            if(is_selected) {
                shader->setUniformValue(m_pointSizeLoc, 5.0f);
                shader->setUniformValue(m_useOverrideColorLoc, 1);
                shader->setUniformValue(m_overrideColorLoc,
                                        QVector4D(info.m_selectedColor[0], info.m_selectedColor[1],
                                                  info.m_selectedColor[2],
                                                  info.m_selectedColor[3]));
                RenderBatch::drawVertexRange(gl, node_buf, info.m_vertexOffset, info.m_vertexCount,
                                             GL_POINTS);
            } else if(is_hover) {
                shader->setUniformValue(m_pointSizeLoc, 7.0f);
                shader->setUniformValue(m_useOverrideColorLoc, 1);
                shader->setUniformValue(m_overrideColorLoc,
                                        QVector4D(info.m_hoverColor[0], info.m_hoverColor[1],
                                                  info.m_hoverColor[2], info.m_hoverColor[3]));
                RenderBatch::drawVertexRange(gl, node_buf, info.m_vertexOffset, info.m_vertexCount,
                                             GL_POINTS);
            }
        }
    }

    gl.glDepthFunc(GL_LESS);
    shader->release();

    gl.glDisable(GL_BLEND);
    gl.glDisable(GL_DEPTH_TEST);
}

void MeshPass::cleanup(QOpenGLFunctions& gl) {
    (void)gl;
    LOG_DEBUG("MeshPass: Cleanup");
}

void MeshPass::setHighlightedEntity(uint64_t uid56, RenderEntityType type) {
    if(type == RenderEntityType::None || uid56 == 0) {
        m_hoverType = RenderEntityType::None;
        m_hoverUid56 = 0;
        return;
    }
    m_hoverType = type;
    m_hoverUid56 = uid56;
}

} // namespace OpenGeoLab::Render
