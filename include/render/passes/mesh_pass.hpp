/**
 * @file mesh_pass.hpp
 * @brief Mesh rendering pass for FEM mesh elements and nodes
 *
 * Handles rendering of mesh wireframe edges and node points separately
 * from the geometry pass, allowing independent visibility control and
 * preventing interference with geometry picking and highlighting.
 *
 * Uses batched rendering: one draw call per category for base mesh,
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
 * @brief Renders FEM mesh elements (wireframe) and nodes (points) with hover and selection support.
 *
 * This pass is separate from GeometryPass to allow independent visibility
 * control and to prevent mesh rendering from interfering with geometry
 * picking and highlighting.
 *
 * With batched buffers, base rendering uses drawAll() per category.
 * Hover/selection uses sub-draw for affected entities.
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
     * @param uid56 56-bit entity uid
     * @param type Render entity type for hover matching
     */
    void setHighlightedEntity(uint64_t uid56, RenderEntityType type);

private:
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

    RenderEntityType m_hoverType{RenderEntityType::None};
    uint64_t m_hoverUid56{0};
};

} // namespace OpenGeoLab::Render
