/**
 * @file render_sceneImpl.hpp
 * @brief Internal implementation of IRenderScene — owns the render passes
 *        and orchestrates per-frame rendering and GPU picking.
 */

#pragma once

#include "render/render_scene.hpp"

// #include "render/pick_resolver.hpp"
#include "pass/geometry_pass.hpp"
// #include "render/pass/mesh_pass.hpp"
// #include "render/pass/pick_pass.hpp"

namespace OpenGeoLab::Render {
/** @brief Concrete IRenderScene that composes GeometryPass, MeshPass, and PickPass. */
class RenderSceneImpl : public IRenderScene {
public:
    RenderSceneImpl();
    ~RenderSceneImpl() override;

    /** @brief Initialise geometry, mesh, and pick passes. */
    void initialize() override;
    [[nodiscard]] bool isInitialized() const override;
    /** @brief Resize the pick FBO to match the viewport.
     *  @param size New viewport size in device pixels.
     */
    void setViewportSize(const QSize& size) override;

    /**
     * @brief Synchronize GUI-thread state into the render scene.
     *
     * Caches render data, camera, and display mode. Updates GPU buffers
     * and pick resolver when data is dirty.
     */
    void synchronize(const SceneFrameState& state) override;

    /** @brief Process a mouse-click pick action.
     *  @param input Pick parameters (cursor position, action type).
     */
    void processPicking(const PickingInput& input) override;
    /** @brief Execute all render passes for the current frame using cached state. */
    void render() override;
    /** @brief Release all GPU resources across all passes. */
    void cleanup() override;

    /**
     * @brief Process hover — render to pick FBO and update hover state.
     *
     * Call after main render with current cursor position. Uses region-based
     * picking with priority selection (Vertex > Edge > Face > Part).
     * Camera matrices are obtained from the cached SceneFrameState.
     */
    void processHover(int pixel_x, int pixel_y) override;

private:
    GeometryPass m_geometryPass;
    // MeshPass m_meshPass;
    // PickPass m_pickPass;

    // PickResolver m_pickResolver; ///< GL-free entity hierarchy resolver

    SceneFrameState m_frameState; ///< Cached per-frame state from GUI thread

    QSize m_viewportSize;
    bool m_initialized{false};
    bool m_pickPassInitialized{false};

    uint64_t m_geometryDataVersion{0}; ///< Last geometry data version uploaded to GPU
    uint64_t m_meshDataVersion{0};     ///< Last mesh data version uploaded to GPU
};

// =============================================================================
// SceneRendererFactory implementation
// =============================================================================

/** @brief Factory registered with the component system to create RenderSceneImpl instances. */
class SceneRendererFactoryImpl : public SceneRendererFactory {
public:
    SceneRendererFactoryImpl() = default;
    ~SceneRendererFactoryImpl() = default;

    tObjectPtr create() override { return std::make_unique<RenderSceneImpl>(); }
};
} // namespace OpenGeoLab::Render
