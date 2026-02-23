/**
 * @file geometry_pass.cpp
 * @brief GeometryPass implementation - batched scene rendering with Phong lighting
 *
 * Uses one draw call per category (faces, edges, vertices) for base rendering,
 * plus sub-draw calls for hover/selection highlighting over the affected ranges.
 */

#include "render/passes/geometry_pass.hpp"
#include "render/renderer_core.hpp"
#include "util/logger.hpp"

#include <QVector4D>

namespace OpenGeoLab::Render {

void GeometryPass::initialize(QOpenGLFunctions& gl) {
    (void)gl;
    LOG_DEBUG("GeometryPass: Initialized");
}

void GeometryPass::resize(QOpenGLFunctions& gl, const QSize& size) {
    (void)gl;
    (void)size;
}

void GeometryPass::execute(QOpenGLFunctions& gl, const RenderPassContext& ctx) {
    auto* core = ctx.m_core;
    if(!core) {
        return;
    }

    auto* mesh_shader = core->shader("mesh");
    if(!mesh_shader || !mesh_shader->isLinked()) {
        return;
    }

    gl.glClearColor(0.18f, 0.18f, 0.22f, 1.0f);
    gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl.glEnable(GL_DEPTH_TEST);
    gl.glEnable(GL_BLEND);
    gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const QMatrix4x4 model; // Identity
    const QMatrix3x3 normal_matrix = model.normalMatrix();

    mesh_shader->bind();

    // Cache uniform locations on first use
    if(m_mvpLoc < 0) {
        m_mvpLoc = mesh_shader->uniformLocation("uMVPMatrix");
        m_modelLoc = mesh_shader->uniformLocation("uModelMatrix");
        m_normalMatLoc = mesh_shader->uniformLocation("uNormalMatrix");
        m_lightPosLoc = mesh_shader->uniformLocation("uLightPos");
        m_viewPosLoc = mesh_shader->uniformLocation("uViewPos");
        m_pointSizeLoc = mesh_shader->uniformLocation("uPointSize");
        m_useLightingLoc = mesh_shader->uniformLocation("uUseLighting");
        m_useOverrideColorLoc = mesh_shader->uniformLocation("uUseOverrideColor");
        m_overrideColorLoc = mesh_shader->uniformLocation("uOverrideColor");
    }

    mesh_shader->setUniformValue(m_mvpLoc, ctx.m_matrices.m_mvp);
    mesh_shader->setUniformValue(m_modelLoc, model);
    mesh_shader->setUniformValue(m_normalMatLoc, normal_matrix);
    mesh_shader->setUniformValue(m_lightPosLoc, ctx.m_cameraPos);
    mesh_shader->setUniformValue(m_viewPosLoc, ctx.m_cameraPos);
    mesh_shader->setUniformValue(m_useLightingLoc, 1);
    mesh_shader->setUniformValue(m_useOverrideColorLoc, 0);

    gl.glEnable(GL_PROGRAM_POINT_SIZE);
    mesh_shader->setUniformValue(m_pointSizeLoc, 1.0f);

    const auto& sm = SelectManager::instance();
    auto& batch = core->batch();

    renderFaces(gl, *mesh_shader, batch, sm);
    renderEdges(gl, *mesh_shader, batch, sm);
    renderVertices(gl, *mesh_shader, batch, sm);

    gl.glDepthFunc(GL_LESS);
    mesh_shader->release();

    gl.glDisable(GL_BLEND);
    gl.glDisable(GL_DEPTH_TEST);
}

void GeometryPass::cleanup(QOpenGLFunctions& gl) {
    (void)gl;
    LOG_DEBUG("GeometryPass: Cleanup");
}

void GeometryPass::setHighlightedEntity(uint64_t uid56, RenderEntityType type) {
    if(type == RenderEntityType::None || uid56 == 0) {
        m_hoverType = RenderEntityType::None;
        m_hoverUid56 = 0;
        return;
    }
    m_hoverType = type;
    m_hoverUid56 = uid56;
}

void GeometryPass::renderFaces(QOpenGLFunctions& gl,
                               QOpenGLShaderProgram& shader,
                               RenderBatch& batch,
                               const SelectManager& sm) {
    auto& buf = batch.faceBuffer();
    if(!buf.isValid()) {
        return;
    }

    // Base draw: all faces with vertex colors + lighting
    gl.glDepthFunc(GL_LESS);
    gl.glEnable(GL_POLYGON_OFFSET_FILL);
    gl.glPolygonOffset(1.0f, 1.0f);

    shader.setUniformValue(m_useLightingLoc, 1);
    setOverrideColor(shader, false);
    RenderBatch::drawAll(gl, buf);

    gl.glDisable(GL_POLYGON_OFFSET_FILL);

    // Hover/selection overlay — sub-draw affected entities with override color
    gl.glDepthFunc(GL_LEQUAL);
    gl.glEnable(GL_POLYGON_OFFSET_FILL);
    gl.glPolygonOffset(0.5f, 0.5f);

    for(const auto& [packed_uid, info] : batch.faceEntities()) {
        const auto entity_type = info.m_uid.type();
        const auto uid56 = info.m_uid.uid56();

        // Check selection (direct entity, owning part, owning solid)
        bool is_selected = sm.containsSelection(uid56, entity_type);
        if(!is_selected && info.m_owningPartUid56 != 0) {
            is_selected = sm.containsSelection(info.m_owningPartUid56, RenderEntityType::Part);
        }
        if(!is_selected && info.m_owningSolidUid56 != 0) {
            is_selected = sm.containsSelection(info.m_owningSolidUid56, RenderEntityType::Solid);
        }

        // Check hover
        bool is_hover = false;
        if(m_hoverType == RenderEntityType::Face) {
            is_hover = (uid56 == m_hoverUid56);
        } else if(m_hoverType == RenderEntityType::Part) {
            is_hover = (info.m_owningPartUid56 == m_hoverUid56 && m_hoverUid56 != 0);
        } else if(m_hoverType == RenderEntityType::Solid) {
            is_hover = (info.m_owningSolidUid56 == m_hoverUid56 && m_hoverUid56 != 0);
        }

        if(is_selected) {
            setOverrideColor(shader, true,
                             QVector4D(info.m_selectedColor[0], info.m_selectedColor[1],
                                       info.m_selectedColor[2], info.m_selectedColor[3]));
            RenderBatch::drawIndexRange(gl, buf, info.m_indexOffset, info.m_indexCount);
        } else if(is_hover) {
            setOverrideColor(shader, true,
                             QVector4D(info.m_hoverColor[0], info.m_hoverColor[1],
                                       info.m_hoverColor[2], info.m_hoverColor[3]));
            RenderBatch::drawIndexRange(gl, buf, info.m_indexOffset, info.m_indexCount);
        }
    }

    gl.glDisable(GL_POLYGON_OFFSET_FILL);
    setOverrideColor(shader, false);
}

void GeometryPass::renderEdges(QOpenGLFunctions& gl,
                               QOpenGLShaderProgram& shader,
                               RenderBatch& batch,
                               const SelectManager& sm) {
    auto& buf = batch.edgeBuffer();
    if(!buf.isValid()) {
        return;
    }

    gl.glDepthFunc(GL_LEQUAL);
    shader.setUniformValue(m_useLightingLoc, 0);

    // Base draw: all edges with vertex colors
    gl.glLineWidth(2.0f);
    setOverrideColor(shader, false);
    RenderBatch::drawAll(gl, buf);

    // Hover/selection overlay
    for(const auto& [packed_uid, info] : batch.edgeEntities()) {
        const auto entity_type = info.m_uid.type();
        const auto uid56 = info.m_uid.uid56();

        bool is_selected = sm.containsSelection(uid56, entity_type);
        if(!is_selected && info.m_owningPartUid56 != 0) {
            is_selected = sm.containsSelection(info.m_owningPartUid56, RenderEntityType::Part);
        }
        if(!is_selected && info.m_owningSolidUid56 != 0) {
            is_selected = sm.containsSelection(info.m_owningSolidUid56, RenderEntityType::Solid);
        }

        bool is_hover = false;
        if(m_hoverType == RenderEntityType::Edge) {
            is_hover = (uid56 == m_hoverUid56);
        } else if(m_hoverType == RenderEntityType::Wire) {
            for(const auto& wire_uid : info.m_owningWireUid56s) {
                if(wire_uid == m_hoverUid56 && m_hoverUid56 != 0) {
                    is_hover = true;
                    break;
                }
            }
        } else if(m_hoverType == RenderEntityType::Part) {
            is_hover = (info.m_owningPartUid56 == m_hoverUid56 && m_hoverUid56 != 0);
        } else if(m_hoverType == RenderEntityType::Solid) {
            is_hover = (info.m_owningSolidUid56 == m_hoverUid56 && m_hoverUid56 != 0);
        }

        if(is_selected || is_hover) {
            gl.glLineWidth(is_hover ? 4.0f : 3.0f);
            const float* c = is_selected ? info.m_selectedColor : info.m_hoverColor;
            setOverrideColor(shader, true, QVector4D(c[0], c[1], c[2], c[3]));
            RenderBatch::drawIndexRange(gl, buf, info.m_indexOffset, info.m_indexCount);
        }
    }

    gl.glLineWidth(1.0f);
    setOverrideColor(shader, false);
}

void GeometryPass::renderVertices(QOpenGLFunctions& gl,
                                  QOpenGLShaderProgram& shader,
                                  RenderBatch& batch,
                                  const SelectManager& sm) {
    auto& buf = batch.vertexBuffer();
    if(!buf.isValid()) {
        return;
    }

    gl.glDepthFunc(GL_LEQUAL);
    shader.setUniformValue(m_useLightingLoc, 0);
    shader.setUniformValue(m_pointSizeLoc, 6.0f);

    // Base draw: all vertices as points
    setOverrideColor(shader, false);
    RenderBatch::drawAll(gl, buf, GL_POINTS);

    // Hover/selection overlay
    for(const auto& [packed_uid, info] : batch.vertexEntities()) {
        const auto entity_type = info.m_uid.type();
        const auto uid56 = info.m_uid.uid56();

        bool is_selected = sm.containsSelection(uid56, entity_type);
        if(!is_selected && info.m_owningPartUid56 != 0) {
            is_selected = sm.containsSelection(info.m_owningPartUid56, RenderEntityType::Part);
        }
        if(!is_selected && info.m_owningSolidUid56 != 0) {
            is_selected = sm.containsSelection(info.m_owningSolidUid56, RenderEntityType::Solid);
        }

        bool is_hover = false;
        if(m_hoverType == RenderEntityType::Vertex) {
            is_hover = (uid56 == m_hoverUid56);
        } else if(m_hoverType == RenderEntityType::Part) {
            is_hover = (info.m_owningPartUid56 == m_hoverUid56 && m_hoverUid56 != 0);
        } else if(m_hoverType == RenderEntityType::Solid) {
            is_hover = (info.m_owningSolidUid56 == m_hoverUid56 && m_hoverUid56 != 0);
        }

        if(is_selected || is_hover) {
            shader.setUniformValue(m_pointSizeLoc, is_hover ? 9.0f : 7.0f);
            const float* c = is_selected ? info.m_selectedColor : info.m_hoverColor;
            setOverrideColor(shader, true, QVector4D(c[0], c[1], c[2], c[3]));
            RenderBatch::drawVertexRange(gl, buf, info.m_vertexOffset, info.m_vertexCount,
                                         GL_POINTS);
        }
    }

    setOverrideColor(shader, false);
    shader.setUniformValue(m_useLightingLoc, 1);
}

void GeometryPass::setOverrideColor(QOpenGLShaderProgram& shader,
                                    bool enabled,
                                    const QVector4D& color) {
    if(enabled) {
        shader.setUniformValue(m_useOverrideColorLoc, 1);
        shader.setUniformValue(m_overrideColorLoc, color);
    } else {
        shader.setUniformValue(m_useOverrideColorLoc, 0);
    }
}

} // namespace OpenGeoLab::Render
