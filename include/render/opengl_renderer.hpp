/**
 * @file opengl_renderer.hpp
 * @brief OpenGL renderer for 3D triangle mesh rendering
 *
 * Provides modular OpenGL rendering functionality with support for:
 * - Custom geometry data (position, normal, color)
 * - Flexible camera system with orbit, zoom, and pan
 * - Multi-light lighting environment
 * - Material properties
 */

#pragma once

#include "render/camera.hpp"
#include "render/lighting.hpp"
#include "render/render_data.hpp"
#include "render/trackball.hpp"

#include <QColor>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QQuaternion>
#include <QQuickWindow>
#include <QtCore/QObject>
#include <memory>

namespace OpenGeoLab {
namespace Render {

/**
 * @brief OpenGL renderer for 3D geometry
 *
 * Modular renderer with separate camera and lighting systems.
 * Supports:
 * - Custom geometry data (vertices with position, normal, color)
 * - Flexible orbit camera with proper zoom and pan
 * - Multi-light environment with ambient, diffuse, and specular
 * - Material properties (color override, shininess)
 * - Trackball rotation for intuitive model manipulation
 */
class OpenGLRenderer : public QObject, protected QOpenGLFunctions {
    Q_OBJECT

public:
    OpenGLRenderer();
    ~OpenGLRenderer() override;

    /**
     * @brief Set render geometry data
     * @param geometry Shared pointer to render geometry
     */
    void setRenderGeometry(std::shared_ptr<RenderGeometry> geometry);

    /**
     * @brief Get current render geometry
     * @return Shared pointer to current geometry
     */
    std::shared_ptr<RenderGeometry> renderGeometry() const { return m_renderGeometry; }

    /**
     * @brief Get camera for external manipulation
     * @return Pointer to camera object
     */
    Camera* camera() { return m_camera.get(); }
    const Camera* camera() const { return m_camera.get(); }

    /**
     * @brief Get trackball for model rotation
     * @return Pointer to trackball object
     */
    Trackball* trackball() { return &m_trackball; }
    const Trackball* trackball() const { return &m_trackball; }

    /**
     * @brief Get lighting environment
     * @return Reference to lighting environment
     */
    LightingEnvironment& lighting() { return m_lighting; }
    const LightingEnvironment& lighting() const { return m_lighting; }

    /**
     * @brief Set color override for the entire geometry
     * @param color Color to use (overrides per-vertex colors if alpha > 0)
     */
    void setColorOverride(const QColor& color);

    /**
     * @brief Get current color override
     * @return Current color (alpha = 0 means using per-vertex colors)
     */
    QColor colorOverride() const { return m_colorOverride; }

    /**
     * @brief Set material properties
     * @param material Material to use
     */
    void setMaterial(const Material& material);

    /**
     * @brief Get current material
     * @return Current material
     */
    const Material& material() const { return m_material; }

    /**
     * @brief Set viewport size for rendering
     * @param size Viewport size in pixels
     */
    void setViewportSize(const QSize& size);

    /**
     * @brief Set viewport offset (for rendering in part of window)
     * @param offset Offset from window origin
     */
    void setViewportOffset(const QPoint& offset) { m_viewportOffset = offset; }

    /**
     * @brief Set the Qt Quick window for rendering
     * @param window Window to render to
     */
    void setWindow(QQuickWindow* window) { m_window = window; }

    /**
     * @brief Set background clear color
     * @param color Background color
     */
    void setBackgroundColor(const QColor& color) { m_backgroundColor = color; }

    /**
     * @brief Get background color
     * @return Background color
     */
    QColor backgroundColor() const { return m_backgroundColor; }

    /**
     * @brief Rotate the model by a quaternion
     * @param rotation Quaternion representing the incremental rotation
     */
    void rotateModelByQuaternion(const QQuaternion& rotation);

    /**
     * @brief Reset model rotation to identity
     */
    void resetModelRotation();

    /**
     * @brief Get model transformation matrix
     * @return Current model matrix including rotation
     */
    QMatrix4x4 modelMatrix() const;

    /**
     * @brief Set model center point for rotation
     * @param center Center point of the model
     */
    void setModelCenter(const QVector3D& center);

    /**
     * @brief Get model center point
     * @return Current model center
     */
    QVector3D modelCenter() const { return m_modelCenter; }

    /**
     * @brief Fit view to geometry bounds
     */
    void fitToView();

public slots:
    /**
     * @brief Initialize OpenGL resources
     */
    void init();

    /**
     * @brief Render the scene
     */
    void paint();

private:
    void createShaderProgram();
    void createBuffers();
    void setupVertexAttributes();
    void uploadLightingUniforms();

    static const char* vertexShaderSource();
    static const char* fragmentShaderSource();

    bool m_initialized = false;
    bool m_needsBufferUpdate = false;

    std::unique_ptr<Camera> m_camera;
    Trackball m_trackball;
    LightingEnvironment m_lighting;
    Material m_material;

    QOpenGLShaderProgram* m_program = nullptr;
    QOpenGLBuffer m_vbo;
    QOpenGLBuffer m_ebo;

    std::shared_ptr<RenderGeometry> m_renderGeometry;

    QColor m_colorOverride = QColor(0, 0, 0, 0);
    QColor m_backgroundColor = QColor(45, 50, 56);
    QSize m_viewportSize;
    QPoint m_viewportOffset;
    QQuickWindow* m_window = nullptr;

    QQuaternion m_modelRotation;
    QVector3D m_modelCenter = QVector3D(0, 0, 0);

    static constexpr int MAX_LIGHTS = 4;
};

} // namespace Render
} // namespace OpenGeoLab
