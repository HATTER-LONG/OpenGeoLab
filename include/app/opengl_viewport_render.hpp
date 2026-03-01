/**
 * @file opengl_viewport_render.hpp
 * @brief Renderer counterpart of GLViewport, executing on the render thread.
 */

#pragma once
#include "render/render_scene.hpp"

#include <QQuickFramebufferObject>

namespace OpenGeoLab::App {

/**
 * @brief Render-thread renderer for GLViewport.
 *
 * Created by GLViewport::createRenderer(). Synchronizes pick/hover input
 * from the GUI thread each frame, then delegates rendering to IRenderScene.
 */
class GLViewportRender : public QQuickFramebufferObject::Renderer {
public:
    explicit GLViewportRender();
    ~GLViewportRender() override;

    /**
     * @brief Copy GUI-thread state (camera, pick input, hover position) into
     *        render-thread locals. Called by Qt before each render().
     */
    void synchronize(QQuickFramebufferObject* item) override;

    /** @brief Execute all render passes for the current frame. */
    void render() override;

private:
    std::unique_ptr<Render::IRenderScene> m_renderScene; ///< Rendering component

    Render::PickingInput m_pendingPick; ///< Pick input captured during synchronize
    bool m_hasPendingPick{false};       ///< Whether a pick action is pending

    // Hover state — synchronized from GUI thread each frame
    QPointF m_cursorPos;           ///< Latest cursor position in item coordinates
    QSizeF m_itemSize;             ///< Item size in logical pixels
    qreal m_devicePixelRatio{1.0}; ///< Device pixel ratio
};
} // namespace OpenGeoLab::App