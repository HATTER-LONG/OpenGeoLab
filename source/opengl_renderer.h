// opengl_renderer.h - Base class for OpenGL rendering
// Provides a reusable framework for rendering different geometries
#pragma once

#include "geometry.h"
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QQuickWindow>
#include <QtCore/QObject>
#include <memory>

/**
 * @brief Base class for OpenGL rendering operations
 *
 * This class provides common functionality for OpenGL-based rendering,
 * including shader management, buffer operations, and viewport handling.
 * Derived classes should implement their specific rendering logic.
 */
class OpenGLRendererBase : public QObject, protected QOpenGLFunctions {
    Q_OBJECT

public:
    virtual ~OpenGLRendererBase() = default;

    // Viewport and window management
    void setViewportSize(const QSize& size) { m_viewportSize = size; }
    void setViewportOffset(const QPoint& offset) { m_viewportOffset = offset; }
    void setWindow(QQuickWindow* window) { m_window = window; }

    QSize viewportSize() const { return m_viewportSize; }
    QPoint viewportOffset() const { return m_viewportOffset; }
    QQuickWindow* window() const { return m_window; }

public slots:
    /**
     * @brief Initialize OpenGL resources
     * Must be called before rendering. Typically connected to beforeRendering signal.
     */
    virtual void init() = 0;

    /**
     * @brief Render the scene
     * Called every frame. Typically connected to beforeRenderPassRecording signal.
     */
    virtual void paint() = 0;

protected:
    /**
     * @brief Helper to create and compile shader program
     * @param vertexShader Vertex shader source code
     * @param fragmentShader Fragment shader source code
     * @return Compiled shader program or nullptr on failure
     */
    QOpenGLShaderProgram* createShaderProgram(const char* vertexShader, const char* fragmentShader);

    /**
     * @brief Setup vertex attribute pointers
     * @param index Attribute location
     * @param size Number of components per vertex (1-4)
     * @param stride Byte offset between consecutive vertices
     * @param offset Offset to first component
     */
    void setupVertexAttribPointer(GLuint index, GLint size, GLsizei stride, const void* offset);

    // Viewport configuration
    QSize m_viewportSize;
    QPoint m_viewportOffset;

    // Qt Quick window reference
    QQuickWindow* m_window = nullptr;
};

/**
 * @brief OpenGL renderer for 3D geometry with custom data
 *
 * This renderer supports indexed drawing with vertex buffers,
 * making it suitable for complex 3D geometries.
 */
class OpenGL3DRenderer : public OpenGLRendererBase {
    Q_OBJECT

public:
    virtual ~OpenGL3DRenderer();

    /**
     * @brief Set geometry data to render
     * @param geometryData Pointer to geometry data (vertices, normals, colors, indices)
     */
    void setGeometryData(std::shared_ptr<GeometryData> geometryData);

    void setRotation(qreal rotation) { m_rotation = rotation; }
    qreal rotation() const { return m_rotation; }

public slots:
    void init() override;
    void paint() override;

protected:
    /**
     * @brief Create vertex and index buffers from geometry data
     */
    virtual void createBuffers();

    /**
     * @brief Setup vertex attributes (position, normal, color, etc.)
     */
    virtual void setupVertexAttributes();

    /**
     * @brief Calculate MVP (Model-View-Projection) matrix
     * @return Transformation matrix for rendering
     */
    virtual QMatrix4x4 calculateMVPMatrix();

    /**
     * @brief Get vertex shader source code
     */
    virtual const char* getVertexShaderSource();

    /**
     * @brief Get fragment shader source code
     */
    virtual const char* getFragmentShaderSource();

    // Rendering state
    qreal m_rotation = 0.0;
    bool m_initialized = false;

    // OpenGL resources
    QOpenGLShaderProgram* m_program = nullptr;
    QOpenGLBuffer m_vbo;
    QOpenGLBuffer m_ebo;

    // Geometry data
    std::shared_ptr<GeometryData> m_geometryData;
};
