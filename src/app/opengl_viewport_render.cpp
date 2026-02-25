#include "app/opengl_viewport_render.hpp"
#include "render/render_scene_controller.hpp"
#include "util/logger.hpp"

#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQuickWindow>
#include <QWheelEvent>
#include <QtCore/QMetaObject>
#include <QtMath>

#include <algorithm>

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

void GLViewportRender::render() {
    auto* context = QOpenGLContext::currentContext();
    if(!context || !framebufferObject()) {
        LOG_WARN("GLViewportRender::render: no current OpenGL context or framebuffer");
        return;
    }

    if(!m_renderScene->isInitialized()) {
        m_renderScene->initialize();
    }
    m_renderScene->setViewportSize(framebufferObject()->size());

    const auto& camera_state = Render::RenderSceneController::instance().cameraState();
    const float aspect_ratio = static_cast<float>(framebufferObject()->width()) /
                               static_cast<float>(std::max(1, framebufferObject()->height()));
    const auto view_matrix = camera_state.viewMatrix();
    const auto projection_matrix = camera_state.projectionMatrix(aspect_ratio);

    if(m_viewport) {
        Render::PickingInput picking_input;
        if(m_viewport->consumePendingPickingInput(picking_input)) {
            m_renderScene->processPicking(picking_input);
        }
    }

    m_renderScene->render(camera_state.m_position, view_matrix, projection_matrix);

    QOpenGLFunctions* functions = context->functions();
    if(functions) {
        functions->glFlush();
    }
}
} // namespace OpenGeoLab::App