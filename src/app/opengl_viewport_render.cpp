#include "app/opengl_viewport_render.hpp"

#include "render/render_scene_controller.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQuickWindow>

namespace OpenGeoLab::App {
GLViewportRender::GLViewportRender(const GLViewport* viewport)
    : m_viewport(viewport),
      m_renderScene(
          g_ComponentFactory.createObjectWithID<Render::SceneRendererFactory>("SceneRenderer")) {

    assert(m_renderScene);
    LOG_TRACE("GLViewportRenderer created");
}

GLViewportRender::~GLViewportRender() {
    m_renderScene->cleanup();
    LOG_TRACE("GLViewportRenderer destroyed");
}

// =============================================================================
// Synchronize — transfer state from GUI thread (GLViewport) to render thread
// =============================================================================

void GLViewportRender::synchronize(QQuickFramebufferObject* item) {
    auto* viewport = static_cast<GLViewport*>(item);

    const auto action = viewport->consumePendingPickAction();
    if(action != Render::PickAction::None) {
        m_hasPendingPick = true;
        m_pendingPick.m_action = action;
        m_pendingPick.m_cursorPos = viewport->cursorPosition();
        m_pendingPick.m_itemSize = viewport->size();
        m_pendingPick.m_devicePixelRatio = viewport->currentDevicePixelRatio();

        auto& controller = Render::RenderSceneController::instance();
        const auto& camera = controller.cameraState();
        const float aspect = static_cast<float>(viewport->width()) /
                             static_cast<float>(std::max(1.0, viewport->height()));
        m_pendingPick.m_viewMatrix = camera.viewMatrix();
        m_pendingPick.m_projectionMatrix = camera.projectionMatrix(aspect);
    }
}

// =============================================================================
// Render — called on the render thread
// =============================================================================

void GLViewportRender::render() {
    auto* ctx = QOpenGLContext::currentContext();
    if(!ctx) {
        LOG_WARN("GLViewportRender::render: no current OpenGL context");
        return;
    }
    QOpenGLFunctions* f = ctx->functions();
    if(!f || !framebufferObject()) {
        return;
    }

    // Lazy initialization
    if(!m_renderScene->isInitialized()) {
        m_renderScene->initialize();
    }

    const QSize fboSize = framebufferObject()->size();
    m_renderScene->setViewportSize(fboSize);

    f->glViewport(0, 0, fboSize.width(), fboSize.height());

    // Build camera matrices
    auto& controller = Render::RenderSceneController::instance();
    const auto& camera = controller.cameraState();
    const float aspect =
        static_cast<float>(fboSize.width()) / static_cast<float>(std::max(fboSize.height(), 1));
    const QMatrix4x4 view = camera.viewMatrix();
    const QMatrix4x4 proj = camera.projectionMatrix(aspect);

    // Main scene render
    m_renderScene->render(camera.m_position, view, proj);

    // Process pending pick action (after rendering so GPU buffers are current)
    if(m_hasPendingPick) {
        m_renderScene->processPicking(m_pendingPick);
        m_hasPendingPick = false;
        m_pendingPick.m_action = Render::PickAction::None;

        // Re-bind the viewport FBO after pick pass used its own FBO
        framebufferObject()->bind();
    }
}
} // namespace OpenGeoLab::App
