/**
 * @file geometry_pass.hpp
 * @brief Geometry rendering pass (faces, edges, vertices with lighting)
 *
 * Uses batched rendering: one draw call per category for base geometry,
 * plus sub-draw calls for selected/hovered entity highlighting.
 */

#pragma once

#include "render/render_pass.hpp"
#include "render/render_types.hpp"
#include "render/renderable.hpp"
#include "render/select_manager.hpp"

#include <QOpenGLShaderProgram>
#include <memory>

namespace OpenGeoLab::Render {

/**
 * @brief Renders the main geometry scene with Phong lighting, hover, and selection highlighting.
 *
 * With batched buffers, base rendering uses drawAll() for each category (one draw call).
 * Hover/selection highlighting uses drawIndexRange()/drawVertexRange() to sub-draw
 * only the affected entity's portion with an override color.
 */
class GeometryPass : public RenderPass {
public:
    GeometryPass() = default;
    ~GeometryPass() override = default;

    [[nodiscard]] const char* name() const override { return "GeometryPass"; }

    void initialize(QOpenGLFunctions& gl) override;
    void resize(QOpenGLFunctions& gl, const QSize& size) override;
    void execute(QOpenGLFunctions& gl, const RenderPassContext& ctx) override;
    void cleanup(QOpenGLFunctions& gl) override;

    /**
     * @brief Set the entity to highlight on hover.
     * @param uid56 56-bit entity uid
     * @param type Render entity type for hover matching
     */
    void setHighlightedEntity(uint64_t uid56, RenderEntityType type);

private:
    void renderFaces(QOpenGLFunctions& gl,
                     QOpenGLShaderProgram& shader,
                     RenderBatch& batch,
                     const SelectManager& sm);
    void renderEdges(QOpenGLFunctions& gl,
                     QOpenGLShaderProgram& shader,
                     RenderBatch& batch,
                     const SelectManager& sm);
    void renderVertices(QOpenGLFunctions& gl,
                        QOpenGLShaderProgram& shader,
                        RenderBatch& batch,
                        const SelectManager& sm);

    void setOverrideColor(QOpenGLShaderProgram& shader, bool enabled, const QVector4D& color = {});

    // Uniform locations
    int m_mvpLoc{-1};
    int m_modelLoc{-1};
    int m_normalMatLoc{-1};
    int m_lightPosLoc{-1};
    int m_viewPosLoc{-1};
    int m_pointSizeLoc{-1};
    int m_useLightingLoc{-1};
    int m_useOverrideColorLoc{-1};
    int m_overrideColorLoc{-1};

    RenderEntityType m_hoverType{RenderEntityType::None};
    uint64_t m_hoverUid56{0};
};

} // namespace OpenGeoLab::Render
