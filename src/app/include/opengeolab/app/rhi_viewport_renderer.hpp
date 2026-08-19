/**
 * @file rhi_viewport_renderer.hpp
 * @brief Render-thread renderer for RhiViewport
 */

#pragma once

#include <opengeolab/app/rhi_viewport.hpp>
#include <opengeolab/render/frame_state.hpp>
#include <opengeolab/render/pick_mask.hpp>
#include <opengeolab/render/render_pipeline.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>
#include <opengeolab/scene/viewport_state.hpp>

#include <QPointer>
#include <QQuickRhiItem>

#include <cstdint>
#include <optional>
#include <vector>

namespace OpenGeoLab::App {

/**
 * @brief Records portable QRhi rendering on the Qt Quick render thread
 *
 * Created by RhiViewport::createRenderer(), synchronizes scene state, and
 * records each frame through QRhi on Qt Quick's render thread.
 */
class RhiViewportRenderer : public QQuickRhiItemRenderer {
public:
    RhiViewportRenderer();
    ~RhiViewportRenderer() override;

    void initialize(QRhiCommandBuffer* command_buffer) override;
    void synchronize(QQuickRhiItem* item) override;
    void render(QRhiCommandBuffer* command_buffer) override;

private:
    [[nodiscard]] Render::PickMask pickMask() const;
    [[nodiscard]] Render::PickResult pickAtItemPosition(float x, float y) const;
    void dispatchPickResult(const Render::PickResult& result, Core::PickAction action) const;
    void dispatchHoverResult(const Render::PickResult& result) const;
    void dispatchBoxSelectResults(const RhiViewport::PendingBoxSelect& box) const;
    void dispatchPickAreaResults(const Scene::PendingPickArea& area) const;
    void executeCaptureRequest(const Scene::PendingCapture& capture, QRhiCommandBuffer* command_buffer);

    QPointer<RhiViewport> m_viewport;
    Render::RenderPipeline m_pipeline;
    Render::FrameState m_frameState;
    RhiViewport::PendingPick m_pendingPick;
    RhiViewport::PendingPick m_hoverPick;
    RhiViewport::PendingBoxSelect m_pendingBoxSelect;
    std::optional<Scene::PendingPickArea> m_pendingPickArea;
    std::optional<Scene::PendingCapture> m_pendingCapture;
    bool m_pickingEnabled{true};
    Render::PickMode m_pickMode{Render::PickMode::VEF};
    bool m_pipelineInitialized{false};

    uint64_t m_cachedSceneVersion{0};     ///< Tracks GPU buffer rebuilds.
    uint64_t m_cachedSelectionVersion{0};
    uint64_t m_cachedHoverVersion{0};
    std::vector<Render::HighlightEntry> m_resolvedSelectedEntries;
    std::vector<Render::HighlightEntry> m_resolvedHoveredEntries;
    bool m_selectionActive{false};
    Render::PickMask m_selectionPickMask{Render::PickMask::None};

    uint64_t m_cachedLabelVersion{0};
};

} // namespace OpenGeoLab::App
