/**
 * @file triangle.h
 * @brief OpenGL triangle rendering implementation for Qt Quick
 *
 * This file contains two main classes:
 * - TriangleRenderer: Pure OpenGL rendering logic, decoupled from Qt Quick
 * - TriangleItem: Qt Quick Item wrapper for QML integration
 */

#pragma once

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

/**
 * @class TriangleRenderer
 * @brief Pure OpenGL renderer class for drawing an animated triangle
 *
 * This class handles the low-level OpenGL rendering logic including:
 * - Shader compilation and linking
 * - Vertex buffer management
 * - Color and rotation transformations
 *
 * It's designed to be decoupled from Qt Quick for better separation of concerns.
 */
class TriangleRenderer : public QObject, protected QOpenGLFunctions {
    Q_OBJECT
public:
    /**
     * @brief Constructs a new TriangleRenderer with default settings
     */
    explicit TriangleRenderer();

    /**
     * @brief Destructor - cleans up OpenGL resources
     */
    ~TriangleRenderer();

    /**
     * @brief Sets the triangle's color by name
     * @param color Color name (e.g., "red", "green", "blue")
     */
    void setColor(const QString& color);

    /**
     * @brief Sets the rotation angle of the triangle
     * @param angle Rotation angle in degrees (0-360)
     */
    void setAngle(qreal angle);

    /**
     * @brief Sets the viewport size for rendering
     * @param size The size of the rendering area
     */
    void setViewportSize(const QSize& size);

    /**
     * @brief Sets the viewport position in the window
     * @param pos The bottom-left position in OpenGL coordinates
     */
    void setViewportPosition(const QPoint& pos);

    /**
     * @brief Sets the associated QQuickWindow for rendering context
     * @param window Pointer to the parent window
     */
    void setWindow(QQuickWindow* window);

public slots:
    /**
     * @brief Initializes OpenGL resources (shaders, buffers)
     * Should be called once before first paint
     */
    void init();

    /**
     * @brief Performs the actual OpenGL rendering
     * Called every frame to draw the triangle
     */
    void paint();

private:
    /**
     * @brief Updates the color uniform based on current color string
     * Converts color name to RGB vector
     */
    void updateColorUniform();

    QSize m_viewportSize;                      ///< Size of the rendering viewport
    QPoint m_viewportPos;                      ///< Position of the viewport in window
    QString m_color;                           ///< Current color name
    qreal m_angle = 0.0;                       ///< Current rotation angle in degrees
    QOpenGLShaderProgram* m_program = nullptr; ///< Shader program
    QQuickWindow* m_window = nullptr;          ///< Associated window
    QOpenGLBuffer m_vbo;                       ///< Vertex buffer object
    QVector3D m_colorVec;                      ///< RGB color vector (0.0-1.0)
};

/**
 * @class TriangleItem
 * @brief Qt Quick Item wrapper for triangle rendering
 *
 * This class provides the QML interface for the triangle renderer:
 * - Exposes properties (color, angle, fps) to QML
 * - Handles Qt Quick scene graph synchronization
 * - Manages the lifecycle of the TriangleRenderer
 * - Calculates and reports rendering FPS
 */
class TriangleItem : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qreal angle READ angle WRITE setAngle NOTIFY angleChanged)
    Q_PROPERTY(int fps READ fps NOTIFY fpsChanged)
    QML_NAMED_ELEMENT(TriangleItem)

public:
    /**
     * @brief Constructs a new TriangleItem
     */
    TriangleItem();

    /**
     * @brief Gets the current color name
     * @return The color as a string
     */
    QString color() const { return m_color; }

    /**
     * @brief Sets a new color for the triangle
     * @param color Color name (e.g., "red", "green", "blue")
     */
    void setColor(const QString& color);

    /**
     * @brief Gets the current rotation angle
     * @return The angle in degrees
     */
    qreal angle() const { return m_angle; }

    /**
     * @brief Sets the rotation angle
     * @param angle Rotation angle in degrees
     */
    void setAngle(qreal angle);

    /**
     * @brief Gets the current rendering FPS
     * @return Frames per second
     */
    int fps() const { return m_fps; }

signals:
    /**
     * @brief Emitted when the color property changes
     */
    void colorChanged();

    /**
     * @brief Emitted when the angle property changes
     */
    void angleChanged();

    /**
     * @brief Emitted when the FPS value is updated (approximately every second)
     */
    void fpsChanged();

public slots:
    /**
     * @brief Synchronizes QML properties with the renderer
     * Called before each frame by Qt Quick scene graph
     */
    void sync();

    /**
     * @brief Cleans up renderer resources
     * Called when the scene graph is invalidated
     */
    void cleanup();

private slots:
    /**
     * @brief Handles window change events
     * @param win The new window (or nullptr if detached)
     */
    void handleWindowChanged(QQuickWindow* win);

private:
    /**
     * @brief Releases OpenGL resources properly
     * Overrides QQuickItem::releaseResources()
     */
    void releaseResources() override;

    /**
     * @brief Updates FPS calculation
     * Called every frame to track rendering performance
     */
    void updateFps();

    QString m_color = "red";                ///< Current triangle color
    qreal m_angle = 0.0;                    ///< Current rotation angle in degrees
    TriangleRenderer* m_renderer = nullptr; ///< OpenGL renderer instance

    // FPS tracking members
    int m_fps = 0;            ///< Current frames per second
    int m_frameCount = 0;     ///< Frame counter for FPS calculation
    qint64 m_lastFpsTime = 0; ///< Last time FPS was calculated (ms since epoch)
};
