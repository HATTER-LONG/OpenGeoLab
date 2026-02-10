/**
 * @file scene_renderer.hpp
 * @brief OpenGL scene renderer facade
 *
 * SceneRenderer is now a thin facade over RendererCore + registered RenderPasses.
 * It maintains the original public API for backward compatibility with
 * GLViewportRenderer in opengl_viewport.cpp.
 */

#pragma once

#include "render/passes/geometry_pass.hpp"
#include "render/passes/highlight_pass.hpp"
#include "render/passes/mesh_pass.hpp"
#include "render/passes/picking_pass.hpp"
#include "render/render_data.hpp"
#include "render/renderer_core.hpp"

#include <QMatrix4x4>
#include <QSize>
#include <QVector3D>
#include <memory>

namespace OpenGeoLab::Render {

class SelectManager;

/**
 * @brief Facade for the modular rendering pipeline
 *
 * Delegates to RendererCore for GL resource management and pass scheduling.
 * Keeps the original API surface so existing code can migrate incrementally.
 */
class SceneRenderer {
public:
    SceneRenderer();
    ~SceneRenderer();

    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;
    SceneRenderer(SceneRenderer&&) = delete;
    SceneRenderer& operator=(SceneRenderer&&) = delete;

    void initialize();
    [[nodiscard]] bool isInitialized() const;

    void setViewportSize(const QSize& size);
    void uploadMeshData(const DocumentRenderData& render_data);

    /**
     * @brief Render the picking pass and populate the internal pick FBO.
     */
    void renderPicking(const QMatrix4x4& view_matrix, const QMatrix4x4& projection_matrix);

    /**
     * @brief Set the entity to be highlighted on hover.
     */
    void setHighlightedEntity(Geometry::EntityUID uid, Geometry::EntityType type);

    /// @overload EntityRef-based convenience.
    void setHighlightedEntity(const Geometry::EntityRef& ref) {
        setHighlightedEntity(ref.m_uid, ref.m_type);
    }

    /**
     * @brief Render the complete scene (delegates to RendererCore passes).
     */
    void render(const QVector3D& camera_pos,
                const QMatrix4x4& view_matrix,
                const QMatrix4x4& projection_matrix);

    void cleanup();

    // -------------------------------------------------------------------------
    // Access to individual passes (for advanced use)
    // -------------------------------------------------------------------------

    [[nodiscard]] RendererCore& core() { return m_core; }
    [[nodiscard]] const RendererCore& core() const { return m_core; }

    [[nodiscard]] GeometryPass* geometryPass() const { return m_geometryPass; }
    [[nodiscard]] MeshPass* meshPass() const { return m_meshPass; }
    [[nodiscard]] PickingPass* pickingPass() const { return m_pickingPass.get(); }
    [[nodiscard]] HighlightPass* highlightPass() const { return m_highlightPass; }

private:
    RendererCore m_core;

    // Non-owning pointers to registered passes (RendererCore owns them)
    GeometryPass* m_geometryPass{nullptr};
    MeshPass* m_meshPass{nullptr};
    HighlightPass* m_highlightPass{nullptr};

    // PickingPass is owned directly by SceneRenderer (not RendererCore)
    // because it is executed on-demand, not as part of the main pass list.
    std::unique_ptr<PickingPass> m_pickingPass;
};

} // namespace OpenGeoLab::Render
