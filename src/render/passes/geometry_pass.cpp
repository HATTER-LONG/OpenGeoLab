/**
 * @file geometry_pass.cpp
 * @brief GeometryPass implementation - main scene rendering with Phong lighting
 */

#include "render/passes/geometry_pass.hpp"
#include "render/render_scene_controller.hpp"
#include "render/renderer_core.hpp"
#include "util/logger.hpp"

#include <QVector4D>

namespace OpenGeoLab::Render {

namespace {
GLenum toGlPrimitive(RenderPrimitiveType type) {
    switch(type) {
    case RenderPrimitiveType::Points:
        return GL_POINTS;
    case RenderPrimitiveType::Lines:
        return GL_LINES;
    case RenderPrimitiveType::LineStrip:
        return GL_LINE_STRIP;
    case RenderPrimitiveType::Triangles:
        return GL_TRIANGLES;
    case RenderPrimitiveType::TriangleStrip:
        return GL_TRIANGLE_STRIP;
    case RenderPrimitiveType::TriangleFan:
        return GL_TRIANGLE_FAN;
    default:
        return GL_TRIANGLES;
    }
}
} // namespace

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

    auto* shader = core->shader("mesh");
    if(!shader || !shader->isLinked()) {
        return;
    }

    // Cache shader (used by helper methods)
    m_shader.reset(); // We don't own it; RendererCore does
    auto* mesh_shader = shader;

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
    mesh_shader->setUniformValue(m_overrideColorLoc, QVector4D(1.0f, 1.0f, 0.0f, 1.0f));

    gl.glEnable(GL_PROGRAM_POINT_SIZE);
    mesh_shader->setUniformValue(m_pointSizeLoc, 1.0f);

    const auto& sm = SelectManager::instance();
    const auto& scene_ctrl = RenderSceneController::instance();
    auto& batch = core->batch();

    // Store the shader pointer so helper methods can use it
    // This is a borrowed pointer (RendererCore owns the shader)
    struct ShaderContext {
        QOpenGLShaderProgram* m_shader;
        int m_useLightingLoc;
        int m_useOverrideColorLoc;
        int m_overrideColorLoc;
        int m_pointSizeLoc;
    } shader_ctx{mesh_shader, m_useLightingLoc, m_useOverrideColorLoc, m_overrideColorLoc,
                 m_pointSizeLoc};

    // Render faces
    gl.glDepthFunc(GL_LESS);
    mesh_shader->setUniformValue(m_useLightingLoc, 1);
    gl.glEnable(GL_POLYGON_OFFSET_FILL);
    gl.glPolygonOffset(1.0f, 1.0f);

    for(auto& buf : batch.faceMeshes()) {
        if(!scene_ctrl.isPartGeometryVisible(buf.m_owningPartUid)) {
            continue;
        }
        const bool is_selected = isMeshSelected(buf, sm);

        const bool is_hover = isFaceHovered(buf);

        if(is_selected) {
            mesh_shader->setUniformValue(m_useOverrideColorLoc, 1);
            mesh_shader->setUniformValue(m_overrideColorLoc, buf.m_selectedColor);
        } else if(is_hover) {
            mesh_shader->setUniformValue(m_useOverrideColorLoc, 1);
            mesh_shader->setUniformValue(m_overrideColorLoc, buf.m_hoverColor);
        } else {
            mesh_shader->setUniformValue(m_useOverrideColorLoc, 0);
        }

        RenderBatch::draw(gl, buf);
    }
    gl.glDisable(GL_POLYGON_OFFSET_FILL);

    // Render edges
    gl.glDepthFunc(GL_LEQUAL);
    mesh_shader->setUniformValue(m_useLightingLoc, 0);

    for(auto& buf : batch.edgeMeshes()) {
        if(!scene_ctrl.isPartGeometryVisible(buf.m_owningPartUid)) {
            continue;
        }
        const bool is_hover = isEdgeHovered(buf);
        const float line_width = is_hover ? 4.0f : 2.0f;
        gl.glLineWidth(line_width);

        const bool is_selected = isMeshSelected(buf, sm);

        if(is_selected) {
            mesh_shader->setUniformValue(m_useOverrideColorLoc, 1);
            mesh_shader->setUniformValue(m_overrideColorLoc, buf.m_selectedColor);
        } else if(is_hover) {
            mesh_shader->setUniformValue(m_useOverrideColorLoc, 1);
            mesh_shader->setUniformValue(m_overrideColorLoc, buf.m_hoverColor);
        } else {
            mesh_shader->setUniformValue(m_useOverrideColorLoc, 0);
        }

        RenderBatch::draw(gl, buf);
    }

    // Render vertices
    gl.glDepthFunc(GL_LEQUAL);
    mesh_shader->setUniformValue(m_useLightingLoc, 0);

    for(auto& buf : batch.vertexMeshes()) {
        if(!scene_ctrl.isPartGeometryVisible(buf.m_owningPartUid)) {
            continue;
        }
        const bool is_hover = isVertexHovered(buf);
        const bool is_selected = isMeshSelected(buf, sm);

        const float vertex_size = is_hover ? 9.0f : is_selected ? 7.0f : 6.0f;
        mesh_shader->setUniformValue(m_pointSizeLoc, vertex_size);

        if(is_selected) {
            mesh_shader->setUniformValue(m_useOverrideColorLoc, 1);
            mesh_shader->setUniformValue(m_overrideColorLoc, buf.m_selectedColor);
        } else if(is_hover) {
            mesh_shader->setUniformValue(m_useOverrideColorLoc, 1);
            mesh_shader->setUniformValue(m_overrideColorLoc, buf.m_hoverColor);
        } else {
            mesh_shader->setUniformValue(m_useOverrideColorLoc, 0);
        }

        RenderBatch::draw(gl, buf, GL_POINTS);
    }

    gl.glDepthFunc(GL_LESS);
    mesh_shader->release();

    gl.glDisable(GL_BLEND);
    gl.glDisable(GL_DEPTH_TEST);
}

void GeometryPass::cleanup(QOpenGLFunctions& gl) {
    (void)gl;
    m_shader.reset();
    LOG_DEBUG("GeometryPass: Cleanup");
}

void GeometryPass::setHighlightedEntity(Geometry::EntityUID uid, Geometry::EntityType type) {
    if(type == Geometry::EntityType::None || uid == Geometry::INVALID_ENTITY_UID) {
        m_hoverType = Geometry::EntityType::None;
        m_hoverUid = Geometry::INVALID_ENTITY_UID;
        return;
    }
    m_hoverType = type;
    m_hoverUid = static_cast<Geometry::EntityUID>(uid & 0xFFFFFFu);
}

bool GeometryPass::isMeshSelected(const RenderableBuffer& buf, const SelectManager& sm) const {
    bool is_selected =
        sm.containsSelection(Geometry::EntityRef{buf.m_entityUid, buf.m_entityType}) ||
        sm.containsSelection(
            Geometry::EntityRef{buf.m_owningPartUid, Geometry::EntityType::Part}) ||
        sm.containsSelection(
            Geometry::EntityRef{buf.m_owningSolidUid, Geometry::EntityType::Solid});

    if(!is_selected) {
        for(const Geometry::EntityUID& wire_uid : buf.m_owningWireUid) {
            if(sm.containsSelection(Geometry::EntityRef{wire_uid, Geometry::EntityType::Wire})) {
                is_selected = true;
                break;
            }
        }
    }
    return is_selected;
}

bool GeometryPass::isFaceHovered(const RenderableBuffer& buf) const {
    if(m_hoverType == Geometry::EntityType::Face) {
        return uidMatches24(buf.m_entityUid, m_hoverUid);
    }
    if(m_hoverType == Geometry::EntityType::Part) {
        return uidMatches24(buf.m_owningPartUid, m_hoverUid);
    }
    if(m_hoverType == Geometry::EntityType::Solid) {
        return uidMatches24(buf.m_owningSolidUid, m_hoverUid);
    }
    return false;
}

bool GeometryPass::isEdgeHovered(const RenderableBuffer& buf) const {
    return ((m_hoverType == Geometry::EntityType::Edge) && (buf.m_entityType == m_hoverType) &&
            uidMatches24(buf.m_entityUid, m_hoverUid)) ||
           ((m_hoverType == Geometry::EntityType::Wire) &&
            uidMatchesSet24(buf.m_owningWireUid, m_hoverUid)) ||
           ((m_hoverType == Geometry::EntityType::Part) &&
            uidMatches24(buf.m_owningPartUid, m_hoverUid)) ||
           ((m_hoverType == Geometry::EntityType::Solid) &&
            uidMatches24(buf.m_owningSolidUid, m_hoverUid));
}

bool GeometryPass::isVertexHovered(const RenderableBuffer& buf) const {
    return ((m_hoverType == Geometry::EntityType::Vertex) && (buf.m_entityType == m_hoverType) &&
            uidMatches24(buf.m_entityUid, m_hoverUid)) ||
           ((m_hoverType == Geometry::EntityType::Part) &&
            uidMatches24(buf.m_owningPartUid, m_hoverUid)) ||
           ((m_hoverType == Geometry::EntityType::Wire) &&
            uidMatchesSet24(buf.m_owningWireUid, m_hoverUid)) ||
           ((m_hoverType == Geometry::EntityType::Solid) &&
            uidMatches24(buf.m_owningSolidUid, m_hoverUid));
}

bool GeometryPass::uidMatches24(Geometry::EntityUID a, Geometry::EntityUID b) {
    return (static_cast<uint32_t>(a) & 0xFFFFFFu) == (static_cast<uint32_t>(b) & 0xFFFFFFu);
}

bool GeometryPass::uidMatchesSet24(const std::unordered_set<Geometry::EntityUID>& set,
                                   Geometry::EntityUID uid) {
    for(const auto& s : set) {
        if(uidMatches24(s, uid)) {
            return true;
        }
    }
    return false;
}

} // namespace OpenGeoLab::Render
