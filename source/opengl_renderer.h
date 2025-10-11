// opengl_renderer.h - Basic OpenGL renderer for triangle mesh rendering
// Provides simple rendering functionality for geometry data
#pragma once

#include "geometry.h"
#include <QColor>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QQuickWindow>
#include <QtCore/QObject>
#include <memory>

/**
 * @brief Simple OpenGL renderer for 3D triangle mesh rendering
 *
 * This renderer provides basic functionality for rendering triangle meshes
 * with support for:
 * - Custom geometry data (vertices with position, normal, color)
 * - Color override (optional, overrides per-vertex colors)
 * - Simple lighting (ambient + diffuse)
 * - Static camera view (no rotation/animation)
 */
class OpenGLRenderer : public QObject, protected QOpenGLFunctions {
    Q_OBJECT

public:
    ~OpenGLRenderer();

    /**
     * @brief Set geometry data to render
     * @param geometryData Pointer to geometry data (vertices, normals, colors, indices)
     */
    void setGeometryData(std::shared_ptr<GeometryData> geometryData);

    /**
     * @brief Set color override for the entire geometry
     * @param color Color to use (overrides per-vertex colors if set)
     * If alpha is 0, per-vertex colors will be used instead
     */
    void setColor(const QColor& color);

    /**
     * @brief Get current color override
     * @return Current color (alpha = 0 means using per-vertex colors)
     */
    QColor color() const { return m_color; }

    /**
     * @brief Set rotation angles for the geometry
     * @param rotationX Rotation around X axis in degrees
     * @param rotationY Rotation around Y axis in degrees
     */
    void setRotation(qreal rotationX, qreal rotationY);

    /**
     * @brief Get current rotation angles
     * @return QPair with X and Y rotation angles in degrees
     */
    QPair<qreal, qreal> rotation() const { return qMakePair(m_rotationX, m_rotationY); }

    // Viewport and window management
    void setViewportSize(const QSize& size) { m_viewportSize = size; }
    void setViewportOffset(const QPoint& offset) { m_viewportOffset = offset; }
    void setWindow(QQuickWindow* window) { m_window = window; }

public slots:
    /**
     * @brief Initialize OpenGL resources
     * Must be called before rendering. Typically connected to beforeRendering signal.
     */
    void init();

    /**
     * @brief Render the scene
     * Called every frame. Typically connected to beforeRenderPassRecording signal.
     */
    void paint();

private:
    /**
     * @brief Create and compile shader program
     */
    void createShaderProgram();

    /**
     * @brief Create vertex and index buffers from geometry data
     */
    void createBuffers();

    /**
     * @brief Setup vertex attributes (position, normal, color)
     */
    void setupVertexAttributes();

    /**
     * @brief Calculate MVP (Model-View-Projection) matrix
     * @return Transformation matrix for rendering
     */
    QMatrix4x4 calculateMVPMatrix() const;

    // Rendering state
    bool m_initialized = false;
    bool m_needsBufferUpdate = false;

    // Rotation angles in degrees
    qreal m_rotationX = 0.0;
    qreal m_rotationY = 0.0;

    // OpenGL resources
    QOpenGLShaderProgram* m_program = nullptr;
    QOpenGLBuffer m_vbo;
    QOpenGLBuffer m_ebo;

    // Geometry data
    std::shared_ptr<GeometryData> m_geometryData;

    // Rendering configuration
    QColor m_color = QColor(0, 0, 0, 0); // Alpha = 0 means use vertex colors
    QSize m_viewportSize;
    QPoint m_viewportOffset;
    QQuickWindow* m_window = nullptr;
};
