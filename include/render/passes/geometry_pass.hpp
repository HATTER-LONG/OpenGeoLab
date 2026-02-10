/**
 * @file geometry_pass.hpp
 * @brief Geometry rendering pass (faces, edges, vertices with lighting)
 */

#pragma once

#include "render/render_pass.hpp"
#include "render/renderable.hpp"
#include "render/select_manager.hpp"

#include <QOpenGLShaderProgram>
#include <memory>

namespace OpenGeoLab::Render {

/**
 * @brief Renders the main geometry scene with Phong lighting, hover, and selection highlighting.
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
     * @param uid Entity UID to highlight (lower 24 bits used for matching)
     * @param type Entity type for hover matching
     */
    void setHighlightedEntity(Geometry::EntityUID uid, Geometry::EntityType type);

    /// @overload EntityRef-based convenience.
    void setHighlightedEntity(const Geometry::EntityRef& ref) {
        setHighlightedEntity(ref.m_uid, ref.m_type);
    }

private:
    void renderFaces(QOpenGLFunctions& gl, RenderBatch& batch, const SelectManager& sm);
    void renderEdges(QOpenGLFunctions& gl, RenderBatch& batch, const SelectManager& sm);
    void renderVertices(QOpenGLFunctions& gl, RenderBatch& batch, const SelectManager& sm);

    void setOverrideColor(bool enabled, const QVector4D& color);
    void drawBuffer(QOpenGLFunctions& gl, RenderableBuffer& buf, GLenum primitive);

    [[nodiscard]] bool isMeshSelected(const RenderableBuffer& buf, const SelectManager& sm) const;
    [[nodiscard]] bool isFaceHovered(const RenderableBuffer& buf) const;
    [[nodiscard]] bool isEdgeHovered(const RenderableBuffer& buf) const;
    [[nodiscard]] bool isVertexHovered(const RenderableBuffer& buf) const;

    [[nodiscard]] static bool uidMatches24(Geometry::EntityUID a, Geometry::EntityUID b);
    [[nodiscard]] static bool uidMatchesSet24(const std::unordered_set<Geometry::EntityUID>& set,
                                              Geometry::EntityUID uid);

    std::unique_ptr<QOpenGLShaderProgram> m_shader;

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

    Geometry::EntityType m_hoverType{Geometry::EntityType::None};
    Geometry::EntityUID m_hoverUid{Geometry::INVALID_ENTITY_UID};
};

} // namespace OpenGeoLab::Render
