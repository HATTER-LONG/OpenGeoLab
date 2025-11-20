// geometry3d.h - QML item for 3D geometry rendering
// Simple wrapper around OpenGLRenderer for QML integration
#pragma once

#include "opengl_renderer.h"
#include <QColor>
#include <QtQuick/QQuickItem>

/**
 * @brief QML item for 3D geometry rendering
 *
 * This QQuickItem provides a simple interface for rendering 3D geometry in QML.
 * It manages the lifecycle of the OpenGLRenderer and connects it to the
 * Qt Quick scene graph.
 *
 * Features:
 * - Automatic geometry loading (default: cube)
 * - Color override support via QML property
 * - Mouse interaction for rotation (drag to rotate)
 * - Integrates with Qt Quick scene graph
 */
class Geometry3D : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(
        QString geometryType READ geometryType WRITE setGeometryType NOTIFY geometryTypeChanged)
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
     * @brief Get current geometry type
     * @return Geometry type ("cube" or "cylinder")
     */
    QString geometryType() const { return m_geometryType; }

    /**
     * @brief Set geometry type to render
     * @param type Geometry type ("cube" or "cylinder")
     */
    Q_INVOKABLE void setGeometryType(const QString& type);

    /**
     * @brief Set custom geometry data from external source
     * @param geometry_data Shared pointer to geometry data
     */
    Q_INVOKABLE void setCustomGeometry(std::shared_ptr<GeometryData> geometry_data);

signals:
    void colorChanged();
    void geometryTypeChanged();
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
     * @brief Handle mouse move events (for drag rotation)
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * @brief Handle mouse release events
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void releaseResources() override;
    void initializeGeometry();

    OpenGLRenderer* m_renderer = nullptr;
    QColor m_color = QColor(0, 0, 0, 0); // Default: use vertex colors
    QString m_geometryType = "cube";     // Default geometry type

    // Mouse interaction state
    bool m_isDragging = false;
    QPointF m_lastMousePos;
    qreal m_rotationX = 0.0; // Rotation around X axis
    qreal m_rotationY = 0.0; // Rotation around Y axis
};
