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
#include "app/opengl_viewport.hpp"

#include "render/render_select_manager.hpp"

#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQuickWindow>
#include <QWheelEvent>
#include <QtCore/QMetaObject>
#include <QtMath>

namespace OpenGeoLab::App {
GLViewportRender::GLViewportRender()
    : m_renderScene(
          g_ComponentFactory.createObjectWithID<Render::SceneRendererFactory>("SceneRenderer")) {

    assert(m_renderScene);
    LOG_TRACE("GLViewportRenderer created");
}

GLViewportRender::~GLViewportRender() {
    m_renderScene->cleanup();
    LOG_TRACE("GLViewportRenderer destroyed");
}

QOpenGLFramebufferObject* GLViewportRender::createFramebufferObject(const QSize& size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setInternalTextureFormat(GL_RGBA8);
    return new QOpenGLFramebufferObject(size, format);
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
        // when render() processes the pick, it will reset this flag and clear m_pendingPick
        m_hasPendingPick = true;
        m_pendingPick.m_action = action;
        m_pendingPick.m_cursorPos = m_cursorPos;
        m_pendingPick.m_itemSize = m_itemSize;
        m_pendingPick.m_devicePixelRatio = m_devicePixelRatio;
    }

    // Build SceneFrameState from GUI-thread singletons (safe: GUI thread is blocked)
    auto* scene_controller = &Render::RenderSceneController::instance();
    const auto& camera = scene_controller->cameraState();
    const float aspect = static_cast<float>(viewport->width()) /
                         static_cast<float>(std::max(1.0, viewport->height()));

    Render::SceneFrameState frame_state{
        .m_renderData = &scene_controller->renderData(),
        .m_cameraPos = camera.m_position,
        .m_viewMatrix = camera.viewMatrix(),
        .m_projMatrix = camera.projectionMatrix(aspect),
        .m_xRayMode = scene_controller->isXRayMode(),
        .m_meshDisplayMode = scene_controller->meshDisplayMode(),
    };

    m_renderScene->synchronize(frame_state);
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

        // Clear the FBO to background color immediately
        const auto& bg = Util::ColorMap::instance().getBackgroundColor();
        f->glClearColor(bg.m_r, bg.m_g, bg.m_b, 1.0f);
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    const QSize fbo_size = framebufferObject()->size();
    m_renderScene->setViewportSize(fbo_size);

    f->glViewport(0, 0, fbo_size.width(), fbo_size.height());

    m_renderScene->render();

    if(m_hasPendingPick) {
        m_renderScene->processPicking(m_pendingPick);
        m_hasPendingPick = false;
        m_pendingPick = {};

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