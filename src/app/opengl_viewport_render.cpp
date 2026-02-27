/**
 * @file opengl_viewport_render.cpp
 * @brief QQuickFramebufferObject::Renderer that drives rendering on
 *        Qt's dedicated render thread.
 *
 * synchronize() runs while the GUI thread is blocked (Qt scene-graph barrier).
 * All GUI-thread state (render data, camera, display modes) is captured into
 * a SceneFrameState and forwarded to IRenderScene::synchronize(). After that,
 * render() uses only the cached state — no GUI-thread singletons are accessed.
 */

#include "app/opengl_viewport_render.hpp"

#include "render/render_scene_controller.hpp"
#include "render/render_select_manager.hpp"
#include "util/color_map.hpp"
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

    // Always capture cursor state for hover processing
    m_cursorPos = viewport->cursorPosition();
    m_itemSize = viewport->size();
    m_devicePixelRatio = viewport->currentDevicePixelRatio();

    // Capture pending pick action (cursor position only — matrices come from SceneFrameState)
    const auto action = viewport->consumePendingPickAction();
    if(action != Render::PickAction::None) {
        m_hasPendingPick = true;
        m_pendingPick.m_action = action;
        m_pendingPick.m_cursorPos = m_cursorPos;
        m_pendingPick.m_itemSize = m_itemSize;
        m_pendingPick.m_devicePixelRatio = m_devicePixelRatio;
    }

    // Build SceneFrameState from GUI-thread singletons (safe: GUI thread is blocked)
    auto& controller = Render::RenderSceneController::instance();
    const auto& camera = controller.cameraState();
    const float aspect = static_cast<float>(viewport->width()) /
                         static_cast<float>(std::max(1.0, viewport->height()));

    Render::SceneFrameState state;
    state.renderData = &controller.renderData();
    state.cameraPos = camera.m_position;
    state.viewMatrix = camera.viewMatrix();
    state.projMatrix = camera.projectionMatrix(aspect);
    state.xRayMode = controller.isXRayMode();
    state.meshDisplayMode = controller.meshDisplayMode();

    // Forward to scene — caches state, updates GPU buffers if dirty
    m_renderScene->synchronize(state);
}

// =============================================================================
// Render — called on the render thread (no GUI-thread singleton access)
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

        // Clear the FBO to background color immediately so the first visible
        // frame is not white.
        const auto& bg = Util::ColorMap::instance().getBackgroundColor();
        f->glClearColor(bg.m_r, bg.m_g, bg.m_b, 1.0f);
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    const QSize fboSize = framebufferObject()->size();
    m_renderScene->setViewportSize(fboSize);

    f->glViewport(0, 0, fboSize.width(), fboSize.height());

    // Main scene render (uses cached SceneFrameState — no controller access)
    m_renderScene->render();

    // Process pending pick action (after rendering so GPU buffers are current)
    if(m_hasPendingPick) {
        m_renderScene->processPicking(m_pendingPick);
        m_hasPendingPick = false;
        m_pendingPick.m_action = Render::PickAction::None;

        // Re-bind the viewport FBO after pick pass used its own FBO
        framebufferObject()->bind();
    }

    // Process hover (every frame when pick is enabled)
    if(Render::RenderSelectManager::instance().isPickEnabled()) {
        const int px = static_cast<int>(m_cursorPos.x() * m_devicePixelRatio);
        const int py =
            static_cast<int>((m_itemSize.height() - m_cursorPos.y()) * m_devicePixelRatio);

        m_renderScene->processHover(px, py);

        // Re-bind the viewport FBO after hover pick pass used its own FBO
        framebufferObject()->bind();
    }
}
} // namespace OpenGeoLab::App
