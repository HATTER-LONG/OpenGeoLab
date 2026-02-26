#pragma once
#include "render/render_scene.hpp"

#include "opengl_viewport.hpp"
#include <QQuickFramebufferObject>

namespace OpenGeoLab::App {
class GLViewportRender : public QQuickFramebufferObject::Renderer {
public:
    explicit GLViewportRender(const GLViewport* viewport);
    ~GLViewportRender() override;

    void synchronize(QQuickFramebufferObject* item) override;
    void render() override;

private:
    const GLViewport* m_viewport{nullptr};               ///< Parent viewport item
    std::unique_ptr<Render::IRenderScene> m_renderScene; ///< Rendering component

    Render::PickingInput m_pendingPick; ///< Pick input captured during synchronize
    bool m_hasPendingPick{false};       ///< Whether a pick action is pending
};
} // namespace OpenGeoLab::App