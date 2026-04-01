/**
 * @file gl_viewport_renderer.hpp
 * @brief Render-thread renderer for GLViewport
 */

#pragma once

#include <opengeolab/app/gl_viewport.hpp>
#include <opengeolab/render/frame_state.hpp>
#include <opengeolab/render/pick_mask.hpp>
#include <opengeolab/render/render_pipeline.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>

#include <QPointer>
#include <QQuickFramebufferObject>

#include <cstdint>
#include <vector>

namespace OpenGeoLab::App {

/**
 * @brief Performs OpenGL rendering on the Qt render thread
 *
 * Created by GLViewport::createRenderer(). Initializes glad on first
 * createFramebufferObject() call, then synchronizes scene state and
 * renders each frame through RenderPipeline.
 */
class GLViewportRenderer : public QQuickFramebufferObject::Renderer {
public:
    GLViewportRenderer();
    ~GLViewportRenderer() override;

    [[nodiscard]] QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;
    void synchronize(QQuickFramebufferObject* item) override;
    void render() override;

private:
    [[nodiscard]] bool ensureGladInitialized();
    [[nodiscard]] Render::PickMask pickMask() const;
    [[nodiscard]] Render::PickResult pickAtItemPosition(float x, float y) const;
    void dispatchPickResult(const Render::PickResult& result, Core::PickAction action) const;
    void dispatchHoverResult(const Render::PickResult& result) const;
    void dispatchBoxSelectResults(const GLViewport::PendingBoxSelect& box) const;

    QPointer<GLViewport> m_viewport;
    Render::RenderPipeline m_pipeline;
    Render::FrameState m_frameState;
    GLViewport::PendingPick m_pendingPick;
    GLViewport::PendingPick m_hoverPick;
    GLViewport::PendingBoxSelect m_pendingBoxSelect;
    bool m_pickingEnabled{true};
    Render::PickMode m_pickMode{Render::PickMode::VEF};
    bool m_gladInitialized{false};
    bool m_pipelineInitialized{false};

    uint64_t m_cachedSelectionVersion{0};
    uint64_t m_cachedHoverVersion{0};
    std::vector<Scene::DrawRange> m_resolvedSelectedRanges;
    std::vector<Scene::DrawRange> m_resolvedHoveredRanges;
};

} // namespace OpenGeoLab::App
