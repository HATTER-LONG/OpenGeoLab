/**
 * @file geometry3d.hpp
 * @brief QML item for interactive 3D geometry rendering
 *
 * Provides a Qt Quick item that wraps OpenGLRenderer for easy QML integration.
 * Supports mouse interaction (rotation, pan, zoom) and dynamic geometry loading.
 */

#pragma once

#include <render/opengl_renderer.hpp>

#include <QColor>
#include <QtQuick/QQuickItem>

namespace OpenGeoLab {
namespace UI {

/**
 * @brief QML item for 3D geometry rendering
 *
 * This QQuickItem provides a simple interface for rendering 3D geometry in QML.
 * It manages the lifecycle of the OpenGLRenderer and connects it to the
 * Qt Quick scene graph.
 *
 * Features:
 * - Mouse interaction for orbit rotation, pan, and zoom
 * - Dynamic geometry loading from files or programmatic creation
 * - View control methods (zoom in, zoom out, fit to view, reset)
 * - Integrates with Qt Quick scene graph
 */
class Geometry3D : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    QML_ELEMENT

public:
    Geometry3D();
    ~Geometry3D() override = default;

    /**
     * @brief Get current color override
     * @return Color (alpha = 0 means using per-vertex colors)
     */
    QColor color() const { return m_color; }

    /**
     * @brief Set color override for the geometry
     * @param color Color to use (set alpha to 0 to use per-vertex colors)
     */
    void setColor(const QColor& color);

    /**
     * @brief Set custom geometry data from external source
     * @param geometry_data Shared pointer to geometry data
     */
    Q_INVOKABLE void setCustomGeometry(std::shared_ptr<Geometry::GeometryData> geometry_data);

    // ========================================================================
    // View Control Methods
    // ========================================================================

    /**
     * @brief Zoom in the view
     * @param factor Zoom factor (default 1.2 = 20% zoom in)
     */
    Q_INVOKABLE void zoomIn(qreal factor = 1.2);

    /**
     * @brief Zoom out the view
     * @param factor Zoom factor (default 1.2 = 20% zoom out)
     */
    Q_INVOKABLE void zoomOut(qreal factor = 1.2);

    /**
     * @brief Auto-fit the view to show the entire geometry
     */
    Q_INVOKABLE void fitToView();

    /**
     * @brief Reset the view to default camera position
     */
    Q_INVOKABLE void resetView();

    /**
     * @brief Set view to front (looking along -Z axis)
     */
    Q_INVOKABLE void setViewFront();

    /**
     * @brief Set view to back (looking along +Z axis)
     */
    Q_INVOKABLE void setViewBack();

    /**
     * @brief Set view to top (looking along -Y axis)
     */
    Q_INVOKABLE void setViewTop();

    /**
     * @brief Set view to bottom (looking along +Y axis)
     */
    Q_INVOKABLE void setViewBottom();

    /**
     * @brief Set view to left (looking along +X axis)
     */
    Q_INVOKABLE void setViewLeft();

    /**
     * @brief Set view to right (looking along -X axis)
     */
    Q_INVOKABLE void setViewRight();

    /**
     * @brief Set view to isometric (standard CAD view)
     */
    Q_INVOKABLE void setViewIsometric();

signals:
    void colorChanged();
    void rendererReady();
    void modelLoadFailed(const QString& error);

public slots:
    /**
     * @brief Synchronize QML item state with renderer
     * Called before rendering to update renderer parameters
     */
    void sync();

    /**
     * @brief Clean up OpenGL resources
     */
    void cleanup();

private slots:
    void handleWindowChanged(QQuickWindow* win);

protected:
    /**
     * @brief Handle mouse press events
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief Handle mouse move events (for drag rotation/pan)
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * @brief Handle mouse release events
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

    /**
     * @brief Handle mouse wheel events (for zooming)
     */
    void wheelEvent(QWheelEvent* event) override;

private:
    void releaseResources() override;
    void updateCameraFromBounds();

    Rendering::OpenGLRenderer* m_renderer = nullptr;
    QColor m_color = QColor(0, 0, 0, 0); // Default: use vertex colors

    // Mouse interaction state
    enum class DragMode { None, Orbit, Pan };
    DragMode m_dragMode = DragMode::None;
    QPointF m_lastMousePos;

    // Cached bounds for fit-to-view
    bool m_hasBounds = false;
    QVector3D m_boundsMin;
    QVector3D m_boundsMax;
};

} // namespace UI
} // namespace OpenGeoLab
