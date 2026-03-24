/**
 * @file gl_viewport_item.cpp
 * @brief Implements GLViewportItem and its internal Renderer.
 */
#include <opengeolab/app/gl_viewport_item.hpp>

#include <opengeolab/app/viewport_controller.hpp>
#include <opengeolab/render/render_engine.hpp>

#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QQuickWindow>
#include <QWheelEvent>
#include <QtQuick/qquickopenglutils.h>

namespace OpenGeoLab::App {

namespace {

[[nodiscard]] Render::GlFuncPtr loadOpenGLProcAddress(void* userptr, const char* name) {
    auto* context = static_cast<QOpenGLContext*>(userptr);
    if(context == nullptr) {
        return nullptr;
    }

    return reinterpret_cast<Render::GlFuncPtr>(context->getProcAddress(name));
}

class GLViewportRenderer : public QQuickFramebufferObject::Renderer {
public:
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }

    void synchronize(QQuickFramebufferObject* item) override {
        auto* viewport = static_cast<GLViewportItem*>(item);
        engine_ = &viewport->renderEngine();
        window_ = viewport->window();

        const int width = static_cast<int>(viewport->width());
        const int height = static_cast<int>(viewport->height());
        if(width > 0 && height > 0) {
            engine_->resize(width, height);
        }
    }

    void render() override {
        if(engine_ == nullptr) {
            return;
        }

        if(!glad_initialized_) {
            auto* current_context = QOpenGLContext::currentContext();
            if(current_context == nullptr) {
                qWarning("Failed to initialize GLAD: no current OpenGL context");
                return;
            }

            glad_initialized_ = engine_->initialize(
                loadOpenGLProcAddress, static_cast<void*>(current_context));
            if(!glad_initialized_) {
                qWarning("Failed to initialize GLAD");
                return;
            }
        }

        engine_->render();

        if(window_ != nullptr) {
            QQuickOpenGLUtils::resetOpenGLState();
        }

        update();
    }

private:
    Render::RenderEngine* engine_ = nullptr;
    QQuickWindow* window_ = nullptr;
    bool glad_initialized_ = false;
};

} // namespace

GLViewportItem::GLViewportItem(QQuickItem* parent)
    : QQuickFramebufferObject(parent),
      controller_(new ViewportController(renderEngine_.camera(), this)) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemAcceptsInputMethod);
    setMirrorVertically(true);

    connect(controller_, &ViewportController::cameraChanged, this, &QQuickItem::update);
}

QQuickFramebufferObject::Renderer* GLViewportItem::createRenderer() const {
    return new GLViewportRenderer();
}

ViewportController* GLViewportItem::controller() const { return controller_; }

Render::RenderEngine& GLViewportItem::renderEngine() { return renderEngine_; }

void GLViewportItem::mousePressEvent(QMouseEvent* event) {
    controller_->onMousePress(event->position(), event->buttons(), event->modifiers());
    event->accept();
    update();
}

void GLViewportItem::mouseMoveEvent(QMouseEvent* event) {
    controller_->onMouseMove(event->position(), event->buttons(), event->modifiers());
    event->accept();
    update();
}

void GLViewportItem::mouseReleaseEvent(QMouseEvent* event) {
    controller_->onMouseRelease(event->position());
    event->accept();
}

void GLViewportItem::wheelEvent(QWheelEvent* event) {
    controller_->onWheel(static_cast<float>(event->angleDelta().y()));
    event->accept();
    update();
}

} // namespace OpenGeoLab::App
