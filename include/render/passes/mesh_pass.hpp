/**
 * @file mesh_pass.hpp
 * @brief Mesh rendering pass for FEM mesh elements and nodes
 *
 * Handles rendering of mesh wireframe edges and node points separately
 * from the geometry pass, allowing independent visibility control and
 * preventing interference with geometry picking and highlighting.
 */

#pragma once

#include "render/render_pass.hpp"
#include "render/renderable.hpp"
#include "render/select_manager.hpp"

#include <QOpenGLShaderProgram>
#include <memory>

namespace OpenGeoLab::Render {

/**
 * @brief Renders FEM mesh elements (wireframe) and nodes (points) with hover and selection support.
 *
 * This pass is separate from GeometryPass to allow independent visibility
 * control and to prevent mesh rendering from interfering with geometry
 * picking and highlighting.
 */
class MeshPass : public RenderPass {
public:
    MeshPass() = default;
    ~MeshPass() override = default;

    [[nodiscard]] const char* name() const override { return "MeshPass"; }

    void initialize(QOpenGLFunctions& gl) override;
    void resize(QOpenGLFunctions& gl, const QSize& size) override;
    void execute(QOpenGLFunctions& gl, const RenderPassContext& ctx) override;
    void cleanup(QOpenGLFunctions& gl) override;

    /**
     * @brief Set the entity to highlight on hover.
     * @param uid Entity UID (lower 24 bits used for matching)
     * @param type Entity type for hover matching
     */
    void setHighlightedEntity(Geometry::EntityUID uid, Geometry::EntityType type);

private:
    [[nodiscard]] bool isMeshEntityHovered(const RenderableBuffer& buf) const;
    [[nodiscard]] bool isMeshSelected(const RenderableBuffer& buf, const SelectManager& sm) const;
    [[nodiscard]] static bool uidMatches24(Geometry::EntityUID a, Geometry::EntityUID b);

    // Uniform locations (cached from "mesh" shader owned by RendererCore)
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
