/**
 * @file render_sceneImpl.hpp
 * @brief Internal implementation of IRenderScene — owns the render passes
 *        and orchestrates per-frame rendering and GPU picking.
 *
 * Pass architecture:
 *   OpaquePass      — opaque surfaces (geometry + mesh)
 *   TransparentPass — transparent/X-ray surfaces
 *   WireframePass   — edges and points
 *   HighlightPass   — selected/hovered entity overdraw
 *   SelectionPass   — offscreen FBO picking
 *   PostProcessPass — future post-processing (stub)
 *   UIPass          — future UI overlay (stub)
 */

#pragma once

#include "render/core/gpu_buffer.hpp"
#include "render/pick_resolver.hpp"
#include "render/render_scene.hpp"

#include "render/pass/highlight_pass.hpp"
#include "render/pass/opaque_pass.hpp"
#include "render/pass/post_process_pass.hpp"
#include "render/pass/selection_pass.hpp"
#include "render/pass/transparent_pass.hpp"
#include "render/pass/ui_pass.hpp"
#include "render/pass/wireframe_pass.hpp"

namespace OpenGeoLab::Render {

/** @brief Concrete IRenderScene with modular render pass architecture. */
class RenderSceneImpl : public IRenderScene {
public:
    RenderSceneImpl();
    ~RenderSceneImpl() override;

    void initialize() override;
    [[nodiscard]] bool isInitialized() const override;
    void setViewportSize(const QSize& size) override;
    void synchronize(const SceneFrameState& state) override;
    void processPicking(const PickingInput& input) override;
    void render() override;
    void cleanup() override;
    void processHover(int pixel_x, int pixel_y) override;

private:
    // --- GPU Buffers (shared across passes) ---
    GpuBuffer m_geometryBuffer; ///< Geometry (CAD) vertex/index data
    GpuBuffer m_meshBuffer;     ///< Mesh (FEM) vertex data

    // --- Render Passes ---
    OpaquePass m_opaquePass;
    TransparentPass m_transparentPass;
    WireframePass m_wireframePass;
    SelectionPass m_selectionPass;
    HighlightPass m_highlightPass;
    PostProcessPass m_postProcessPass;
    UIPass m_uiPass;

    // --- Pick Resolution ---
    PickResolver m_pickResolver;

    // --- Cached Frame State ---
    SceneFrameState m_frameState;

    // --- Pre-built draw ranges (copied from RenderData during synchronize) ---
    std::vector<DrawRangeEx> m_geometryTriangleRanges;
    std::vector<DrawRangeEx> m_geometryLineRanges;
    std::vector<DrawRangeEx> m_geometryPointRanges;

    // --- Mesh topology counts ---
    uint32_t m_meshSurfaceCount{0};
    uint32_t m_meshWireframeCount{0};
    uint32_t m_meshNodeCount{0};

    // --- Display state ---
    RenderDisplayModeMask m_meshDisplayMode{RenderDisplayModeMask::None};

    QSize m_viewportSize;
    bool m_initialized{false};
    bool m_selectionPassInitialized{false};
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
