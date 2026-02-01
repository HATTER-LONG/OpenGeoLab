/**
 * @file opengl_viewport.hpp
 * @brief QQuickFramebufferObject-based OpenGL viewport for geometry rendering
 *
 * GLViewport provides a QML-integrable 3D viewport that renders geometry
 * using OpenGL. It supports camera manipulation (rotate, pan, zoom) and
 * integrates with RenderService for scene management. The actual rendering
 * is delegated to SceneRenderer from the render module.
 */

#pragma once

#include "app/render_service.hpp"
#include "render/render_data.hpp"
#include "render/scene_renderer.hpp"

#include <QMatrix4x4>
#include <QQuickFramebufferObject>
#include <QtQml/qqml.h>
#include <memory>

namespace OpenGeoLab::App {

class GLViewportRenderer;

/**
 * @brief QML-integrable OpenGL viewport for 3D geometry visualization
 *
 * GLViewport provides:
 * - OpenGL rendering of geometry from RenderService via SceneRenderer
 * - Mouse-based camera manipulation (orbit, pan, zoom)
 * - Integration with the application's render service
 *
 * @note This class handles QML integration and input; rendering logic is
 *       delegated to Render::SceneRenderer.
 */
class GLViewport : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Render::RenderService* renderService READ renderService WRITE setRenderService NOTIFY
                   renderServiceChanged)

public:
    explicit GLViewport(QQuickItem* parent = nullptr);
    ~GLViewport() override;

    /**
     * @brief Create the renderer for this viewport
     * @return New renderer instance
     */
    Renderer* createRenderer() const override;

    /**
     * @brief Get the associated render service
     * @return Pointer to the render service
     */
    [[nodiscard]] Render::RenderService* renderService() const;

    /**
     * @brief Set the render service
     * @param service Render service to use
     */
    void setRenderService(Render::RenderService* service);

    /**
     * @brief Get current camera state for rendering
     * @return Camera state reference
     */
    [[nodiscard]] const Render::CameraState& cameraState() const;

    /**
     * @brief Get render data for rendering
     * @return Document render data reference
     */
    [[nodiscard]] const Render::DocumentRenderData& renderData() const;

signals:
    void renderServiceChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onSceneNeedsUpdate();

private:
    /**
     * @brief Perform orbit camera rotation
     * @param dx Horizontal mouse delta
     * @param dy Vertical mouse delta
     */
    void orbitCamera(float dx, float dy);

    /**
     * @brief Perform camera panning
     * @param dx Horizontal mouse delta
     * @param dy Vertical mouse delta
     */
    void panCamera(float dx, float dy);

    /**
     * @brief Perform camera zoom
     * @param delta Zoom amount
     */
    void zoomCamera(float delta);

private:
    Render::RenderService* m_renderService{nullptr}; ///< Associated render service
    Render::CameraState m_cameraState;               ///< Local camera state copy

    QPointF m_lastMousePos;                   ///< Last mouse position for delta calculation
    Qt::MouseButtons m_pressedButtons;        ///< Currently pressed mouse buttons
    Qt::KeyboardModifiers m_pressedModifiers; ///< Currently pressed keyboard modifiers
};

/**
 * @brief OpenGL renderer for GLViewport
 *
 * Handles the actual OpenGL rendering of geometry meshes using SceneRenderer.
 * Created by GLViewport::createRenderer().
 */
class GLViewportRenderer : public QQuickFramebufferObject::Renderer {
public:
    explicit GLViewportRenderer(const GLViewport* viewport);
    ~GLViewportRenderer() override;

    /**
     * @brief Create the framebuffer object
     * @param size Framebuffer size
     * @return New framebuffer object
     */
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;

    /**
     * @brief Render the scene
     */
    void render() override;

    /**
     * @brief Synchronize with the viewport item
     * @param item The viewport item
     */
    void synchronize(QQuickFramebufferObject* item) override;

private:
    const GLViewport* m_viewport{nullptr};                  ///< Parent viewport item
    std::unique_ptr<Render::SceneRenderer> m_sceneRenderer; ///< Rendering component

    bool m_needsDataUpload{false}; ///< Whether mesh data needs uploading

    // Cached render data
    Render::DocumentRenderData m_renderData;
    Render::CameraState m_cameraState;
    QSize m_viewportSize;
};

} // namespace OpenGeoLab::App
