/**
 * @file geometry3d.cpp
 * @brief Implementation of Geometry3D QML item
 */

#include <core/logger.hpp>
#include <geometry/geometry.hpp>
#include <geometry3d.hpp>

#include <QMouseEvent>
#include <QtCore/QRunnable>
#include <QtQuick/qquickwindow.h>

namespace OpenGeoLab {
namespace UI {

// ============================================================================
// Geometry3D QML Item Implementation
// ============================================================================

Geometry3D::Geometry3D() {
    LOG_DEBUG("Geometry3D constructor called");

    // Enable mouse event handling
    setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
    setAcceptHoverEvents(true);

    connect(this, &QQuickItem::windowChanged, this, &Geometry3D::handleWindowChanged);
}

void Geometry3D::setColor(const QColor& color) {
    if(m_color != color) {
        m_color = color;

        // Update renderer if it exists
        if(m_renderer) {
            m_renderer->setColorOverride(color);
        }

        emit colorChanged();
        LOG_INFO("Geometry3D color changed to: ({}, {}, {}, {})", color.red(), color.green(),
                 color.blue(), color.alpha());
    }
}

void Geometry3D::setCustomGeometry(std::shared_ptr<Geometry::GeometryData> geometry_data) {
    if(m_renderer && geometry_data) {
        m_renderer->setGeometryData(geometry_data);
        LOG_INFO("Custom geometry set: {} vertices, {} indices", geometry_data->vertexCount(),
                 geometry_data->indexCount());

        // Cache bounding box
        float min_point[3], max_point[3];
        if(geometry_data->getBoundingBox(min_point, max_point)) {
            m_hasBounds = true;
            m_boundsMin = QVector3D(min_point[0], min_point[1], min_point[2]);
            m_boundsMax = QVector3D(max_point[0], max_point[1], max_point[2]);

            // Set model center for rotation pivot
            QVector3D center = (m_boundsMin + m_boundsMax) * 0.5f;
            m_renderer->setModelCenter(center);

            // Reset model rotation when loading new geometry
            m_renderer->resetModelRotation();

            // Auto-fit the view to show the entire model
            fitToView();
        }

        // Trigger repaint
        if(window()) {
            window()->update();
        }
    } else if(!m_renderer) {
        LOG_DEBUG("Cannot set custom geometry: renderer not initialized");
    } else {
        LOG_DEBUG("Cannot set custom geometry: null geometry data provided");
    }
}

// ============================================================================
// View Control Methods
// ============================================================================

void Geometry3D::zoomIn(qreal factor) {
    if(m_renderer && m_renderer->camera()) {
        m_renderer->camera()->zoom(static_cast<float>(factor));
        if(window()) {
            window()->update();
        }
        LOG_DEBUG("Zoomed in by factor {}", factor);
    }
}

void Geometry3D::zoomOut(qreal factor) {
    if(m_renderer && m_renderer->camera()) {
        m_renderer->camera()->zoom(1.0f / static_cast<float>(factor));
        if(window()) {
            window()->update();
        }
        LOG_DEBUG("Zoomed out by factor {}", factor);
    }
}

void Geometry3D::fitToView() {
    if(!m_renderer || !m_renderer->camera()) {
        LOG_DEBUG("Cannot fit to view: renderer or camera not initialized");
        return;
    }

    if(m_hasBounds) {
        m_renderer->camera()->fitToBounds(m_boundsMin, m_boundsMax, 1.5f);
        LOG_INFO("Fit to view: bounds=[({}, {}, {}), ({}, {}, {})]", m_boundsMin.x(),
                 m_boundsMin.y(), m_boundsMin.z(), m_boundsMax.x(), m_boundsMax.y(),
                 m_boundsMax.z());
    } else {
        // No bounds available, reset to default
        m_renderer->camera()->reset();
        LOG_DEBUG("Fit to view: no bounds, reset to default");
    }

    if(window()) {
        window()->update();
    }
}

void Geometry3D::resetView() {
    if(m_renderer && m_renderer->camera()) {
        m_renderer->camera()->reset();
        m_renderer->resetModelRotation(); // Also reset model rotation
        if(window()) {
            window()->update();
        }
        LOG_DEBUG("View reset to default");
    }
}

void Geometry3D::setViewFront() {
    if(m_renderer) {
        // Set model rotation to show front view
        m_renderer->setModelRotation(0.0f, 0.0f);
        if(window()) {
            window()->update();
        }
        LOG_DEBUG("Set view to front");
    }
}

void Geometry3D::setViewBack() {
    if(m_renderer) {
        // Set model rotation to show back view (rotate 180 degrees around Y)
        m_renderer->setModelRotation(180.0f, 0.0f);
        if(window()) {
            window()->update();
        }
        LOG_DEBUG("Set view to back");
    }
}

void Geometry3D::setViewTop() {
    if(m_renderer) {
        // Set model rotation to show top view (rotate -90 degrees around X)
        m_renderer->setModelRotation(0.0f, -90.0f);
        if(window()) {
            window()->update();
        }
        LOG_DEBUG("Set view to top");
    }
}

void Geometry3D::setViewBottom() {
    if(m_renderer) {
        // Set model rotation to show bottom view (rotate 90 degrees around X)
        m_renderer->setModelRotation(0.0f, 90.0f);
        if(window()) {
            window()->update();
        }
        LOG_DEBUG("Set view to bottom");
    }
}

void Geometry3D::setViewLeft() {
    if(m_renderer) {
        // Set model rotation to show left view (rotate 90 degrees around Y)
        m_renderer->setModelRotation(90.0f, 0.0f);
        if(window()) {
            window()->update();
        }
        LOG_DEBUG("Set view to left");
    }
}

void Geometry3D::setViewRight() {
    if(m_renderer) {
        // Set model rotation to show right view (rotate -90 degrees around Y)
        m_renderer->setModelRotation(-90.0f, 0.0f);
        if(window()) {
            window()->update();
        }
        LOG_DEBUG("Set view to right");
    }
}

void Geometry3D::setViewIsometric() {
    if(m_renderer) {
        // Set model rotation to show isometric view
        // Standard isometric: 45 degrees yaw, ~35.264 degrees pitch
        m_renderer->setModelRotation(-45.0f, -35.264f);
        if(window()) {
            window()->update();
        }
        LOG_DEBUG("Set view to isometric");
    }
}

// ============================================================================
// Window and Renderer Setup
// ============================================================================

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

void Geometry3D::sync() {
    if(!m_renderer) {
        LOG_INFO("Creating new OpenGLRenderer");

        m_renderer = new Rendering::OpenGLRenderer();

        // Set initial color
        m_renderer->setColorOverride(m_color);

        // Connect renderer to scene graph signals
        connect(window(), &QQuickWindow::beforeRendering, m_renderer,
                &Rendering::OpenGLRenderer::init, Qt::DirectConnection);
        connect(window(), &QQuickWindow::beforeRenderPassRecording, m_renderer,
                &Rendering::OpenGLRenderer::paint, Qt::DirectConnection);

        emit rendererReady();
    }

    // Calculate item position and size in window coordinates
    QPointF scene_pos = mapToScene(QPointF(0, 0));
    QPoint offset(static_cast<int>(scene_pos.x() * window()->devicePixelRatio()),
                  static_cast<int>(scene_pos.y() * window()->devicePixelRatio()));
    QSize size(static_cast<int>(width() * window()->devicePixelRatio()),
               static_cast<int>(height() * window()->devicePixelRatio()));

    // Update renderer state
    m_renderer->setViewportSize(size);
    m_renderer->setViewportOffset(offset);
    m_renderer->setWindow(window());

    LOG_TRACE("Geometry3D sync: offset=({},{}), size=({}x{})", offset.x(), offset.y(), size.width(),
              size.height());
}

// ============================================================================
// Mouse Event Handling
// ============================================================================

void Geometry3D::mousePressEvent(QMouseEvent* event) {
    if(event->button() == Qt::LeftButton) {
        // Check if Shift is pressed for panning
        if(event->modifiers() & Qt::ShiftModifier) {
            m_dragMode = DragMode::Pan;
        } else {
            m_dragMode = DragMode::Orbit;
        }
        m_lastMousePos = event->position();
        event->accept();
        LOG_DEBUG("Mouse press at ({}, {}), mode={}", m_lastMousePos.x(), m_lastMousePos.y(),
                  m_dragMode == DragMode::Orbit ? "Orbit" : "Pan");
    } else if(event->button() == Qt::MiddleButton) {
        // Middle button for panning
        m_dragMode = DragMode::Pan;
        m_lastMousePos = event->position();
        event->accept();
    } else if(event->button() == Qt::RightButton) {
        // Right button for panning (alternative)
        m_dragMode = DragMode::Pan;
        m_lastMousePos = event->position();
        event->accept();
    } else {
        QQuickItem::mousePressEvent(event);
    }
}

void Geometry3D::mouseMoveEvent(QMouseEvent* event) {
    if(m_dragMode != DragMode::None && m_renderer && m_renderer->camera()) {
        QPointF current_pos = event->position();
        QPointF delta = current_pos - m_lastMousePos;

        if(m_dragMode == DragMode::Pan) {
            // Pan the camera
            m_renderer->camera()->pan(static_cast<float>(-delta.x()),
                                      static_cast<float>(delta.y()));
            LOG_TRACE("Pan: delta=({}, {})", delta.x(), delta.y());
        } else if(m_dragMode == DragMode::Orbit) {
            // Rotate the model using quaternion-based rotation
            // This avoids gimbal lock and direction inversion issues
            float sensitivity = 0.3f;
            m_renderer->rotateModel(static_cast<float>(delta.x()) * sensitivity,
                                    static_cast<float>(-delta.y()) * sensitivity);
            LOG_TRACE("Model rotate: delta=({}, {})", delta.x(), delta.y());
        }

        m_lastMousePos = current_pos;

        // Trigger repaint
        if(window()) {
            window()->update();
        }

        event->accept();
    } else {
        QQuickItem::mouseMoveEvent(event);
    }
}

void Geometry3D::mouseReleaseEvent(QMouseEvent* event) {
    if(event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton ||
       event->button() == Qt::RightButton) {
        m_dragMode = DragMode::None;
        event->accept();
        LOG_DEBUG("Mouse released");
    } else {
        QQuickItem::mouseReleaseEvent(event);
    }
}

void Geometry3D::wheelEvent(QWheelEvent* event) {
    if(!m_renderer || !m_renderer->camera()) {
        QQuickItem::wheelEvent(event);
        return;
    }

    // Handle mouse wheel for zooming
    QPoint num_degrees = event->angleDelta() / 8;

    if(!num_degrees.isNull()) {
        QPoint num_steps = num_degrees / 15;

        // Calculate zoom factor based on wheel direction
        // Positive delta = wheel up = zoom in
        // Use exponential scaling for smooth zoom
        float zoom_factor = std::pow(1.1f, static_cast<float>(num_steps.y()));
        m_renderer->camera()->zoom(zoom_factor);

        // Trigger repaint
        if(window()) {
            window()->update();
        }

        LOG_DEBUG("Zoom: factor={}, steps={}", zoom_factor, num_steps.y());
        event->accept();
    } else {
        QQuickItem::wheelEvent(event);
    }
}

// ============================================================================
// Cleanup
// ============================================================================

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
    explicit CleanupJob(Rendering::OpenGLRenderer* renderer) : m_renderer(renderer) {
        LOG_DEBUG("CleanupJob created for renderer");
    }

    void run() override {
        LOG_DEBUG("CleanupJob running, deleting renderer");
        delete m_renderer;
    }

private:
    Rendering::OpenGLRenderer* m_renderer;
};

void Geometry3D::releaseResources() {
    LOG_INFO("Geometry3D releasing resources");
    window()->scheduleRenderJob(new CleanupJob(m_renderer), QQuickWindow::BeforeSynchronizingStage);
    m_renderer = nullptr;
}

void Geometry3D::updateCameraFromBounds() {
    // This method can be called when geometry changes to update camera clipping planes
    if(m_renderer && m_renderer->camera() && m_hasBounds) {
        // Update near/far planes based on model size
        QVector3D size = m_boundsMax - m_boundsMin;
        float max_size = qMax(qMax(size.x(), size.y()), size.z());

        if(max_size > 0.0001f) {
            m_renderer->camera()->setNearPlane(max_size * 0.001f);
            m_renderer->camera()->setFarPlane(max_size * 100.0f);
        }
    }
}

} // namespace UI
} // namespace OpenGeoLab
