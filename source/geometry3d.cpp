/**
 * @file geometry3d.cpp
 * @brief Implementation of Geometry3D QML item
 */

#include "geometry3d.h"
#include "geometry.h"
#include "logger.hpp"

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

void Geometry3D::setGeometryType(const QString& type) {
    if(m_geometryType != type) {
        m_geometryType = type;

        // Update geometry if renderer exists
        if(m_renderer) {
            if(type == "cylinder") {
                auto cylinder_data = std::make_shared<Geometry::CylinderData>();
                m_renderer->setGeometryData(cylinder_data);
                LOG_INFO("Geometry changed to cylinder");
            } else {
                auto cube_data = std::make_shared<Geometry::CubeData>();
                m_renderer->setGeometryData(cube_data);
                LOG_INFO("Geometry changed to cube");
            }

            // Trigger repaint
            if(window()) {
                window()->update();
            }
        }

        emit geometryTypeChanged();
    }
}

void Geometry3D::setZoom(qreal zoom) {
    if(m_zoom != zoom) {
        m_zoom = zoom;

        // Update renderer zoom
        if(m_renderer) {
            m_renderer->setZoom(zoom);
        }

        // Trigger repaint
        if(window()) {
            window()->update();
        }

        LOG_DEBUG("Zoom set to: {}", m_zoom);
    }
}

void Geometry3D::fitToView() {
    if(!m_renderer) {
        LOG_DEBUG("Cannot fit to view: renderer not initialized");
        return;
    }

    // Get current geometry data
    auto geometry_data = m_renderer->geometryData();
    if(!geometry_data) {
        LOG_DEBUG("Cannot fit to view: no geometry data");
        return;
    }

    // Get bounding box
    float min_point[3], max_point[3];
    if(!geometry_data->getBoundingBox(min_point, max_point)) {
        LOG_DEBUG("Cannot fit to view: invalid bounding box");
        return;
    }

    // Calculate bounding box size
    float size_x = max_point[0] - min_point[0];
    float size_y = max_point[1] - min_point[1];
    float size_z = max_point[2] - min_point[2];
    float max_size = std::max({size_x, size_y, size_z});

    // Calculate center
    float center_x = (min_point[0] + max_point[0]) * 0.5f;
    float center_y = (min_point[1] + max_point[1]) * 0.5f;

    // Reset rotation
    m_rotationX = 0.0;
    m_rotationY = 0.0;

    // Reset pan to center
    m_panX = -center_x;
    m_panY = -center_y;

    // Calculate appropriate zoom level
    // Assume camera distance is 3.0 at zoom = 1.0
    // We want the model to fit within ~80% of the viewport
    if(max_size > 0.0001f) {
        m_zoom = 2.4f / max_size; // 3.0 * 0.8 = 2.4
        m_zoom = qBound(0.01, m_zoom, 100.0);
    } else {
        m_zoom = 1.0;
    }

    // Update renderer
    if(m_renderer) {
        m_renderer->setRotation(m_rotationX, m_rotationY);
        m_renderer->setZoom(m_zoom);
        m_renderer->setPan(m_panX, m_panY);
    }

    // Trigger repaint
    if(window()) {
        window()->update();
    }

    LOG_INFO("Fit to view: zoom={}, pan=({}, {}), bbox_size={}", m_zoom, m_panX, m_panY, max_size);
}

void Geometry3D::setCustomGeometry(std::shared_ptr<Geometry::GeometryData> geometry_data) {
    if(m_renderer && geometry_data) {
        m_renderer->setGeometryData(geometry_data);
        LOG_INFO("Custom geometry set: {} vertices, {} indices", geometry_data->vertexCount(),
                 geometry_data->indexCount());

        // Auto-fit the view to show the entire model
        fitToView();

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

    // Create geometry based on type
    if(m_geometryType == "cylinder") {
        auto cylinder_data = std::make_shared<Geometry::CylinderData>();
        m_renderer->setGeometryData(cylinder_data);
        LOG_INFO("Default cylinder geometry initialized");
    } else {
        auto cube_data = std::make_shared<Geometry::CubeData>();
        m_renderer->setGeometryData(cube_data);
        LOG_INFO("Default cube geometry initialized");
    }
}

void Geometry3D::sync() {
    if(!m_renderer) {
        LOG_INFO("Creating new OpenGLRenderer");

        m_renderer = new Rendering::OpenGLRenderer();

        // Initialize geometry
        initializeGeometry();

        // Set initial color
        m_renderer->setColor(m_color);

        // Connect renderer to scene graph signals
        connect(window(), &QQuickWindow::beforeRendering, m_renderer,
                &Rendering::OpenGLRenderer::init, Qt::DirectConnection);
        connect(window(), &QQuickWindow::beforeRenderPassRecording, m_renderer,
                &Rendering::OpenGLRenderer::paint, Qt::DirectConnection);

        emit rendererReady();
    }

    // Calculate item position and size in window coordinates
    QPointF scene_pos = mapToScene(QPointF(0, 0));
    QPoint offset(scene_pos.x() * window()->devicePixelRatio(),
                  scene_pos.y() * window()->devicePixelRatio());
    QSize size(width() * window()->devicePixelRatio(), height() * window()->devicePixelRatio());

    // Update renderer state
    m_renderer->setViewportSize(size);
    m_renderer->setViewportOffset(offset);
    m_renderer->setWindow(window());
    m_renderer->setRotation(m_rotationX, m_rotationY);
    m_renderer->setZoom(m_zoom);
    m_renderer->setPan(m_panX, m_panY);

    LOG_TRACE("Geometry3D sync: offset=({},{}), size=({}x{})", offset.x(), offset.y(), size.width(),
              size.height());
}

void Geometry3D::mousePressEvent(QMouseEvent* event) {
    if(event->button() == Qt::LeftButton) {
        // Check if Shift is pressed for panning
        if(event->modifiers() & Qt::ShiftModifier) {
            m_isPanning = true;
        } else {
            m_isDragging = true;
        }
        m_lastMousePos = event->position();
        event->accept();
        LOG_INFO("Mouse press at ({}, {}), panning={}, rotating={}", m_lastMousePos.x(),
                 m_lastMousePos.y(), m_isPanning, m_isDragging);
    } else {
        QQuickItem::mousePressEvent(event);
    }
}

void Geometry3D::mouseMoveEvent(QMouseEvent* event) {
    if(m_isDragging || m_isPanning) {
        QPointF current_pos = event->position();
        QPointF delta = current_pos - m_lastMousePos;

        if(m_isPanning) {
            // Pan the camera (Shift + Left button)
            // Scale pan speed based on zoom level (pan slower when zoomed in)
            float pan_speed = 0.003f / m_zoom; // Increased from 0.001f for better responsiveness
            m_panX -= delta.x() * pan_speed;   // Invert X for natural movement
            m_panY += delta.y() * pan_speed;   // Invert Y for natural movement

            // Update renderer pan
            if(m_renderer) {
                m_renderer->setPan(m_panX, m_panY);
            }

            LOG_TRACE("Pan updated: X={}, Y={}", m_panX, m_panY);
        } else if(m_isDragging) {
            // Rotate the model (Left button only)
            m_rotationY += delta.x() * 0.5; // Scale factor for sensitivity
            m_rotationX += delta.y() * 0.5;

            // Clamp X rotation to avoid gimbal lock
            m_rotationX = qBound(-89.0, m_rotationX, 89.0);

            // Update renderer rotation
            if(m_renderer) {
                m_renderer->setRotation(m_rotationX, m_rotationY);
            }

            LOG_TRACE("Rotation updated: X={}, Y={}", m_rotationX, m_rotationY);
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
    if(event->button() == Qt::LeftButton) {
        m_isDragging = false;
        m_isPanning = false;
        event->accept();
        LOG_DEBUG("Mouse released");
    } else {
        QQuickItem::mouseReleaseEvent(event);
    }
}

void Geometry3D::wheelEvent(QWheelEvent* event) {
    // Handle mouse wheel for zooming
    QPoint num_degrees = event->angleDelta() / 8;

    if(!num_degrees.isNull()) {
        QPoint num_steps = num_degrees / 15; // Each step is typically 15 degrees

        // Zoom in/out based on wheel direction
        // Positive delta = wheel up = zoom in
        // Negative delta = wheel down = zoom out
        float zoom_factor = 1.0f + (num_steps.y() * 0.1f); // 10% per step
        m_zoom *= zoom_factor;

        // Clamp zoom to extended range for better usability
        m_zoom = qBound(0.01, m_zoom, 100.0); // From 0.01x to 100x        // Update renderer
        if(m_renderer) {
            m_renderer->setZoom(m_zoom);
        }

        // Trigger repaint
        if(window()) {
            window()->update();
        }

        LOG_INFO("Zoom updated: {}", m_zoom);
        event->accept();
    } else {
        QQuickItem::wheelEvent(event);
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

} // namespace UI
} // namespace OpenGeoLab
