/**
 * @file render_sceneImpl.hpp
 * @brief Internal implementation of IRenderScene — owns shared GPU resources
 *        and orchestrates the multi-pass rendering pipeline.
 *
 * Pass execution order:
 *   1. OpaquePass / TransparentPass (mutually exclusive)
 *   2. WireframePass
 *   3. HighlightPass
 *   4. PostProcessPass (stub)
 *   5. UIPass (stub)
 *   * SelectionPass runs on-demand from processHover()/processPicking()
 */

#pragma once

#include "render/render_scene.hpp"

#include "core/gpu_buffer.hpp"
#include "pass/highlight_pass.hpp"
#include "pass/opaque_pass.hpp"
#include "pass/post_process_pass.hpp"
#include "pass/selection_pass.hpp"
#include "pass/transparent_pass.hpp"
#include "pass/ui_pass.hpp"
#include "pass/wireframe_pass.hpp"

namespace OpenGeoLab::Render {

/** @brief Concrete IRenderScene composing the multi-pass rendering pipeline. */
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
    void processHover(int pixel_x, int pixel_y) override;
    void cleanup() override;

private:
    // ── Shared GPU resources ────────────────────────────────────────────
    GpuBuffer m_geometryBuffer; ///< Geometry domain VAO/VBO/IBO

    // Cached draw ranges (copied from RenderData during synchronize)
    std::vector<DrawRange> m_triangleRanges;
    std::vector<DrawRange> m_lineRanges;
    std::vector<DrawRange> m_pointRanges;

    // ── Render passes ───────────────────────────────────────────────────
    OpaquePass m_opaquePass;
    TransparentPass m_transparentPass;
    WireframePass m_wireframePass;
    HighlightPass m_highlightPass;
    SelectionPass m_selectionPass;
    PostProcessPass m_postProcessPass;
    UIPass m_uiPass;

    // ── State ───────────────────────────────────────────────────────────
    SceneFrameState m_frameState;
    QSize m_viewportSize;
    bool m_initialized{false};
    bool m_selectionPassReady{false};

    uint64_t m_geometryDataVersion{0}; ///< Last geometry data version uploaded to GPU
    uint64_t m_meshDataVersion{0};     ///< Last mesh data version uploaded to GPU

    // ── Pick resolution helpers ─────────────────────────────────────────
    /** @brief Resolve a pick result to hover state (edge→wire, entity→part). */
    void resolveAndSetHover(const PickAnalysisResult& result);
    /** @brief Resolve a pick result and apply as selection add/remove. */
    void resolveAndApplySelection(const PickAnalysisResult& result, PickAction action);
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
