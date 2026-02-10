/**
 * @file mesh_pass.cpp
 * @brief MeshPass implementation - FEM mesh element and node rendering
 */

#include "render/passes/mesh_pass.hpp"
#include "render/render_scene_controller.hpp"
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
    if(batch.meshElementMeshes().empty() && batch.meshNodeMeshes().empty()) {
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
    const auto& scene_ctrl = RenderSceneController::instance();

    // Render mesh elements (wireframe lines)
    gl.glDepthFunc(GL_LEQUAL);

    for(auto& buf : batch.meshElementMeshes()) {
        if(!scene_ctrl.isPartMeshVisible(buf.m_owningPartUid)) {
            continue;
        }
        const bool is_hover = isMeshEntityHovered(buf);
        const float line_width = is_hover ? 3.0f : 1.5f;
        gl.glLineWidth(line_width);

        const bool is_selected = isMeshSelected(buf, sm);

        if(is_selected) {
            shader->setUniformValue(m_useOverrideColorLoc, 1);
            shader->setUniformValue(m_overrideColorLoc, buf.m_selectedColor);
        } else if(is_hover) {
            shader->setUniformValue(m_useOverrideColorLoc, 1);
            shader->setUniformValue(m_overrideColorLoc, buf.m_hoverColor);
        } else {
            shader->setUniformValue(m_useOverrideColorLoc, 0);
        }

        RenderBatch::draw(gl, buf);
    }

    // Render mesh nodes (points)
    for(auto& buf : batch.meshNodeMeshes()) {
        if(!scene_ctrl.isPartMeshVisible(buf.m_owningPartUid)) {
            continue;
        }
        const bool is_hover = isMeshEntityHovered(buf);
        const bool is_selected = isMeshSelected(buf, sm);

        const float point_size = is_hover ? 7.0f : is_selected ? 5.0f : 4.0f;
        shader->setUniformValue(m_pointSizeLoc, point_size);

        if(is_selected) {
            shader->setUniformValue(m_useOverrideColorLoc, 1);
            shader->setUniformValue(m_overrideColorLoc, buf.m_selectedColor);
        } else if(is_hover) {
            shader->setUniformValue(m_useOverrideColorLoc, 1);
            shader->setUniformValue(m_overrideColorLoc, buf.m_hoverColor);
        } else {
            shader->setUniformValue(m_useOverrideColorLoc, 0);
        }

        RenderBatch::draw(gl, buf, GL_POINTS);
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

void MeshPass::setHighlightedEntity(Geometry::EntityUID uid, Geometry::EntityType type) {
    if(type == Geometry::EntityType::None || uid == Geometry::INVALID_ENTITY_UID) {
        m_hoverType = Geometry::EntityType::None;
        m_hoverUid = Geometry::INVALID_ENTITY_UID;
        return;
    }
    m_hoverType = type;
    m_hoverUid = static_cast<Geometry::EntityUID>(uid & 0xFFFFFFu);
}

bool MeshPass::isMeshEntityHovered(const RenderableBuffer& buf) const {
    return (m_hoverType == buf.m_entityType) && uidMatches24(buf.m_entityUid, m_hoverUid);
}

bool MeshPass::isMeshSelected(const RenderableBuffer& buf, const SelectManager& sm) const {
    return sm.containsSelection(Geometry::EntityRef{buf.m_entityUid, buf.m_entityType});
}

bool MeshPass::uidMatches24(Geometry::EntityUID a, Geometry::EntityUID b) {
    return (static_cast<uint32_t>(a) & 0xFFFFFFu) == (static_cast<uint32_t>(b) & 0xFFFFFFu);
}

} // namespace OpenGeoLab::Render
