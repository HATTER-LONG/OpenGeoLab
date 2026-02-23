#include "app/opengl_viewport_render.hpp"
#include "util/logger.hpp"

#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QWheelEvent>
#include <QtCore/QMetaObject>
#include <QtMath>
#include <qopenglfunctions.h>

namespace OpenGeoLab::App {
GLViewportRender::GLViewportRender(const GLViewport* viewport) : m_viewport(viewport) {
    LOG_TRACE("GLViewportRenderer created");
}

GLViewportRender::~GLViewportRender() { LOG_TRACE("GLViewportRenderer destroyed"); }
void GLViewportRender::render() {
    // For now, just clear the viewport with a solid color.
    auto* ctx = QOpenGLContext::currentContext();
    if(!ctx) {
        LOG_WARN("GLViewportRender::render: no current OpenGL context");
        return;
    }
    QOpenGLFunctions* f = ctx->functions();
    if(!f || !framebufferObject()) {
        return;
    }

    f->glViewport(0, 0, framebufferObject()->width(), framebufferObject()->height());
    f->glClearColor(0.0f, 0.9f, 0.0f, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
} // namespace OpenGeoLab::App