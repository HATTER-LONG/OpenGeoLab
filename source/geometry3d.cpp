// geometry3d.cpp - Implementation of Geometry3D QML item
#include "geometry3d.h"
#include "geometry.h"
#include "logger.hpp"

#include <QMouseEvent>
#include <QtCore/QRunnable>
#include <QtQuick/qquickwindow.h>

// ============================================================================
// Geometry3D QML Item Implementation
// ============================================================================

Geometry3D::Geometry3D() {
    LOG_DEBUG("Geometry3D constructor called");

    // Enable mouse event handling
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);

    connect(this, &QQuickItem::windowChanged, this, &Geometry3D::handleWindowChanged);
}

void Geometry3D::setColor(const QColor& color) {
    if(m_color != color) {
        m_color = color;

        // Update renderer if it exists
        if(m_renderer) {
            m_renderer->setColor(color);
        }

        emit colorChanged();
        LOG_INFO("Geometry3D color changed to: ({}, {}, {}, {})", color.red(), color.green(),
                 color.blue(), color.alpha());
    }
}

void Geometry3D::handleWindowChanged(QQuickWindow* win) {
    if(win) {
        LOG_DEBUG("Geometry3D window changed, setting up connections");

        // Connect to Qt Quick scene graph signals
        connect(win, &QQuickWindow::beforeSynchronizing, this, &Geometry3D::sync,
                Qt::DirectConnection);
        connect(win, &QQuickWindow::sceneGraphInvalidated, this, &Geometry3D::cleanup,
                Qt::DirectConnection);

        // Set background color
        win->setColor(Qt::black);

        LOG_INFO("Geometry3D window connections established");
    }
}

void Geometry3D::initializeGeometry() {
    if(!m_renderer) {
        return;
    }

    // Create default cube geometry
    auto cubeData = std::make_shared<CubeData>();
    m_renderer->setGeometryData(cubeData);

    LOG_INFO("Default cube geometry initialized");
}

void Geometry3D::sync() {
    if(!m_renderer) {
        LOG_INFO("Creating new OpenGLRenderer");

        m_renderer = new OpenGLRenderer();

        // Initialize geometry
        initializeGeometry();

        // Set initial color
        m_renderer->setColor(m_color);

        // Connect renderer to scene graph signals
        connect(window(), &QQuickWindow::beforeRendering, m_renderer, &OpenGLRenderer::init,
                Qt::DirectConnection);
        connect(window(), &QQuickWindow::beforeRenderPassRecording, m_renderer,
                &OpenGLRenderer::paint, Qt::DirectConnection);

        emit rendererReady();
    }

    // Calculate item position and size in window coordinates
    QPointF scenePos = mapToScene(QPointF(0, 0));
    QPoint offset(scenePos.x() * window()->devicePixelRatio(),
                  scenePos.y() * window()->devicePixelRatio());
    QSize size(width() * window()->devicePixelRatio(), height() * window()->devicePixelRatio());

    // Update renderer state
    m_renderer->setViewportSize(size);
    m_renderer->setViewportOffset(offset);
    m_renderer->setWindow(window());
    m_renderer->setRotation(m_rotationX, m_rotationY); // Update rotation

    LOG_TRACE("Geometry3D sync: offset=({},{}), size=({}x{})", offset.x(), offset.y(), size.width(),
              size.height());
}

void Geometry3D::mousePressEvent(QMouseEvent* event) {
    if(event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->position();
        event->accept();
        LOG_INFO("Mouse press at ({}, {})", m_lastMousePos.x(), m_lastMousePos.y());
    } else {
        QQuickItem::mousePressEvent(event);
    }
}

void Geometry3D::mouseMoveEvent(QMouseEvent* event) {
    if(m_isDragging) {
        QPointF currentPos = event->position();
        QPointF delta = currentPos - m_lastMousePos;

        // Update rotation based on mouse movement
        // Horizontal movement rotates around Y axis
        // Vertical movement rotates around X axis
        m_rotationY += delta.x() * 0.5; // Scale factor for sensitivity
        m_rotationX += delta.y() * 0.5;

        // Clamp X rotation to avoid gimbal lock
        m_rotationX = qBound(-89.0, m_rotationX, 89.0);

        m_lastMousePos = currentPos;

        // Update renderer rotation
        if(m_renderer) {
            m_renderer->setRotation(m_rotationX, m_rotationY);
        }

        // Trigger repaint
        if(window()) {
            window()->update();
        }

        event->accept();
        LOG_TRACE("Rotation updated: X={}, Y={}", m_rotationX, m_rotationY);
    } else {
        QQuickItem::mouseMoveEvent(event);
    }
}

void Geometry3D::mouseReleaseEvent(QMouseEvent* event) {
    if(event->button() == Qt::LeftButton) {
        m_isDragging = false;
        event->accept();
        LOG_DEBUG("Mouse released");
    } else {
        QQuickItem::mouseReleaseEvent(event);
    }
}

void Geometry3D::cleanup() {
    LOG_INFO("Geometry3D cleanup called");
    delete m_renderer;
    m_renderer = nullptr;
}

/**
 * @brief Helper class for cleaning up renderer in render thread
 */
class CleanupJob : public QRunnable {
public:
    explicit CleanupJob(OpenGLRenderer* renderer) : m_renderer(renderer) {
        LOG_DEBUG("CleanupJob created for renderer");
    }

    void run() override {
        LOG_DEBUG("CleanupJob running, deleting renderer");
        delete m_renderer;
    }

private:
    OpenGLRenderer* m_renderer;
};

void Geometry3D::releaseResources() {
    LOG_INFO("Geometry3D releasing resources");
    window()->scheduleRenderJob(new CleanupJob(m_renderer), QQuickWindow::BeforeSynchronizingStage);
    m_renderer = nullptr;
}
