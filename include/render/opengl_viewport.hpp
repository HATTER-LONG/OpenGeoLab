/**
 * @file opengl_viewport.hpp
 * @brief OpenGL viewport QQuickItem for rendering geometry
 *
 * Provides a QQuickItem-based OpenGL viewport that can be embedded in QML
 * for rendering 3D geometry. Displays a default triangle when no model is loaded.
 */

#pragma once

#include "render/render_data.hpp"

#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QQuickFramebufferObject>
#include <QVector3D>
#include <QtQml/qqmlregistration.h>

#include <memory>

namespace OpenGeoLab::Render {

class OpenGLViewportRenderer;

/**
 * @brief QML-exposed OpenGL viewport for 3D geometry rendering
 *
 * This QQuickFramebufferObject provides an OpenGL rendering surface that can
 * be used in QML. It supports camera manipulation and displays geometry from
 * the geometry layer.
 */
class OpenGLViewport : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVector3D cameraPosition READ cameraPosition WRITE setCameraPosition NOTIFY
                   cameraPositionChanged)
    Q_PROPERTY(
        QVector3D cameraTarget READ cameraTarget WRITE setCameraTarget NOTIFY cameraTargetChanged)
    Q_PROPERTY(float fieldOfView READ fieldOfView WRITE setFieldOfView NOTIFY fieldOfViewChanged)
    Q_PROPERTY(bool hasModel READ hasModel NOTIFY hasModelChanged)

public:
    explicit OpenGLViewport(QQuickItem* parent = nullptr);
    ~OpenGLViewport() override;

    /**
     * @brief Create the renderer for this item
     * @return New renderer instance
     */
    [[nodiscard]] Renderer* createRenderer() const override;

    // Camera properties
    [[nodiscard]] QVector3D cameraPosition() const { return m_cameraPosition; }
    void setCameraPosition(const QVector3D& position);

    [[nodiscard]] QVector3D cameraTarget() const { return m_cameraTarget; }
    void setCameraTarget(const QVector3D& target);

    [[nodiscard]] float fieldOfView() const { return m_fieldOfView; }
    void setFieldOfView(float fov);

    [[nodiscard]] bool hasModel() const { return m_hasModel; }

    /**
     * @brief Set render scene data
     * @param scene Scene data to render
     * @note Thread-safe, triggers update on the renderer
     */
    void setRenderScene(const RenderScene& scene);

    /**
     * @brief Clear current model and show default triangle
     */
    Q_INVOKABLE void clearModel();

    /**
     * @brief Reset camera to default view
     */
    Q_INVOKABLE void resetCamera();

    /**
     * @brief Fit camera to view all geometry
     */
    Q_INVOKABLE void fitToView();

signals:
    void cameraPositionChanged();
    void cameraTargetChanged();
    void fieldOfViewChanged();
    void hasModelChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    friend class OpenGLViewportRenderer;

    QVector3D m_cameraPosition{0.0f, 0.0f, 5.0f};
    QVector3D m_cameraTarget{0.0f, 0.0f, 0.0f};
    float m_fieldOfView{45.0f};
    bool m_hasModel{false};

    RenderScene m_renderScene;
    bool m_sceneNeedsUpdate{true};

    // Mouse interaction state
    QPointF m_lastMousePos;
    bool m_rotating{false};
    bool m_panning{false};
};

/**
 * @brief OpenGL renderer for the viewport
 *
 * Handles actual OpenGL rendering in a separate thread context.
 */
class OpenGLViewportRenderer : public QQuickFramebufferObject::Renderer,
                               protected QOpenGLFunctions {
public:
    explicit OpenGLViewportRenderer();
    ~OpenGLViewportRenderer() override;

    /**
     * @brief Create the framebuffer object
     * @param size Size of the framebuffer
     * @return New framebuffer object
     */
    [[nodiscard]] QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;

    /**
     * @brief Render the scene
     */
    void render() override;

    /**
     * @brief Synchronize state from the item
     * @param item The viewport item
     */
    void synchronize(QQuickFramebufferObject* item) override;

private:
    void initializeGL();
    void initializeDefaultTriangle();
    void initializeShaders();
    void updateBuffers();
    void renderDefaultTriangle();
    void renderScene();

private:
    bool m_initialized{false};
    bool m_buffersNeedUpdate{true};

    // Shader program
    std::unique_ptr<QOpenGLShaderProgram> m_shaderProgram;

    // Default triangle buffers
    QOpenGLVertexArrayObject m_defaultVAO;
    QOpenGLBuffer m_defaultVBO{QOpenGLBuffer::VertexBuffer};

    // Scene buffers
    QOpenGLVertexArrayObject m_sceneVAO;
    QOpenGLBuffer m_sceneVBO{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer m_sceneIBO{QOpenGLBuffer::IndexBuffer};

    // Scene data
    RenderScene m_renderScene;
    bool m_hasModel{false};

    // Camera matrices
    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_modelMatrix;

    // Camera parameters
    QVector3D m_cameraPosition;
    QVector3D m_cameraTarget;
    float m_fieldOfView{45.0f};

    // Viewport size
    QSize m_viewportSize;
};

} // namespace OpenGeoLab::Render
