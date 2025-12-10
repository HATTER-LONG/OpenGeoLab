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

#include <geometry/geometry.hpp>
#include <render/camera.hpp>
#include <render/lighting.hpp>

#include <QColor>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QQuickWindow>
#include <QtCore/QObject>
#include <memory>

namespace OpenGeoLab {
namespace Rendering {

/**
 * @brief OpenGL renderer for 3D geometry
 *
 * Modular renderer with separate camera and lighting systems.
 * Supports:
 * - Custom geometry data (vertices with position, normal, color)
 * - Flexible orbit camera with proper zoom and pan
 * - Multi-light environment with ambient, diffuse, and specular
 * - Material properties (color override, shininess)
 */
class OpenGLRenderer : public QObject, protected QOpenGLFunctions {
    Q_OBJECT

public:
    OpenGLRenderer();
    ~OpenGLRenderer() override;

    // ========================================================================
    // Geometry Management
    // ========================================================================

    /**
     * @brief Set geometry data to render
     * @param geometry_data Pointer to geometry data (vertices, normals, colors, indices)
     */
    void setGeometryData(std::shared_ptr<Geometry::GeometryData> geometry_data);

    /**
     * @brief Get current geometry data
     * @return Shared pointer to current geometry data
     */
    std::shared_ptr<Geometry::GeometryData> geometryData() const { return m_geometryData; }

    // ========================================================================
    // Camera Access
    // ========================================================================

    /**
     * @brief Get camera for external manipulation
     * @return Pointer to camera object
     */
    Camera* camera() { return m_camera.get(); }
    const Camera* camera() const { return m_camera.get(); }

    // ========================================================================
    // Lighting Access
    // ========================================================================

    /**
     * @brief Get lighting environment
     * @return Reference to lighting environment
     */
    LightingEnvironment& lighting() { return m_lighting; }
    const LightingEnvironment& lighting() const { return m_lighting; }

    // ========================================================================
    // Material Properties
    // ========================================================================

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

    // ========================================================================
    // Viewport Configuration
    // ========================================================================

    /**
     * @brief Set viewport size for rendering
     * @param size Viewport size in pixels
     */
    void setViewportSize(const QSize& size) { m_viewportSize = size; }

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

    // ========================================================================
    // Background Color
    // ========================================================================

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

    // ========================================================================
    // Model Rotation
    // ========================================================================

    /**
     * @brief Rotate the model (instead of rotating camera)
     * @param delta_yaw Horizontal rotation in degrees
     * @param delta_pitch Vertical rotation in degrees
     *
     * Model rotation provides better lighting consistency as lights stay fixed
     * in world space. Also avoids gimbal lock issues at extreme camera angles.
     */
    void rotateModel(float delta_yaw, float delta_pitch);

    /**
     * @brief Set model rotation angles directly
     * @param yaw Horizontal rotation in degrees
     * @param pitch Vertical rotation in degrees
     */
    void setModelRotation(float yaw, float pitch);

    /**
     * @brief Get current model rotation angles
     * @param yaw Output: horizontal rotation in degrees
     * @param pitch Output: vertical rotation in degrees
     */
    void modelRotation(float& yaw, float& pitch) const;

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
     * @brief Upload lighting uniforms to shader
     */
    void uploadLightingUniforms();

    /**
     * @brief Get shader source for vertex shader
     */
    static const char* vertexShaderSource();

    /**
     * @brief Get shader source for fragment shader
     */
    static const char* fragmentShaderSource();

    // ========================================================================
    // Rendering State
    // ========================================================================

    bool m_initialized = false;
    bool m_needsBufferUpdate = false;

    // Camera and lighting
    std::unique_ptr<Camera> m_camera;
    LightingEnvironment m_lighting;
    Material m_material;

    // OpenGL resources
    QOpenGLShaderProgram* m_program = nullptr;
    QOpenGLBuffer m_vbo;
    QOpenGLBuffer m_ebo;

    // Geometry data
    std::shared_ptr<Geometry::GeometryData> m_geometryData;

    // Rendering configuration
    QColor m_colorOverride = QColor(0, 0, 0, 0);   // Alpha = 0 means use vertex colors
    QColor m_backgroundColor = QColor(45, 50, 56); // Modern dark gray
    QSize m_viewportSize;
    QPoint m_viewportOffset;
    QQuickWindow* m_window = nullptr;

    // Model rotation (for rotating model instead of camera)
    float m_modelYaw = 0.0f;   // Horizontal rotation in degrees
    float m_modelPitch = 0.0f; // Vertical rotation in degrees

    // Model center point (for rotation around model center)
    QVector3D m_modelCenter = QVector3D(0, 0, 0);

    // Maximum supported lights in shader
    static constexpr int MAX_LIGHTS = 4;
};

} // namespace Rendering
} // namespace OpenGeoLab
