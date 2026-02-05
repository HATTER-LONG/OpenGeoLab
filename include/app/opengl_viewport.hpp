/**
 * @file opengl_viewport.hpp
 * @brief QQuickFramebufferObject-based OpenGL viewport for geometry rendering
 *
 * GLViewport provides a QML-integrable 3D viewport that renders geometry
 * using OpenGL. It supports camera manipulation (rotate, pan, zoom) and
 * integrates with RenderSceneController for scene management. The actual rendering
 * is delegated to SceneRenderer from the render module.
 */

#pragma once

#include "render/render_data.hpp"
#include "render/render_scene_controller.hpp"
#include "render/scene_renderer.hpp"
#include "render/trackball_controller.hpp"
#include "util/signal.hpp"

#include <QMatrix4x4>
#include <QQuickFramebufferObject>
#include <QSizeF>
#include <QtQml/qqml.h>
#include <memory>

class QHoverEvent;
class QOpenGLFramebufferObject;
namespace OpenGeoLab::App {

class GLViewportRenderer;

/**
 * @brief QML-integrable OpenGL viewport for 3D geometry visualization
 *
 * GLViewport provides:
 * - OpenGL rendering of geometry from RenderSceneController via SceneRenderer
 * - Mouse-based camera manipulation (orbit, pan, zoom)
 * - Integration with the application's render service
 *
 * @note This class handles QML integration and input; rendering logic is
 *       delegated to Render::SceneRenderer.
 */
class GLViewport : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool hasGeometry READ hasGeometry NOTIFY hasGeometryChanged)

public:
    explicit GLViewport(QQuickItem* parent = nullptr);
    ~GLViewport() override;

    /**
     * @brief Create the renderer for this viewport
     * @return New renderer instance
     */
    Renderer* createRenderer() const override;

    [[nodiscard]] bool hasGeometry() const;

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

    [[nodiscard]] QPointF cursorPos() const { return m_cursorPos; }

    [[nodiscard]] QSizeF itemSize() const { return size(); }

    [[nodiscard]] qreal devicePixelRatio() const { return m_devicePixelRatio; }
signals:
    void hasGeometryChanged();
    void geometryChanged();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;
private slots:
    void onSceneNeedsUpdate();
    void onServiceGeometryChanged();

private:
    Render::CameraState m_cameraState;                 ///< Local camera state copy
    Render::TrackballController m_trackballController; ///< Camera manipulation controller
    bool m_hasGeometry{false};

    Util::ScopedConnection m_sceneNeedsUpdateConn;
    Util::ScopedConnection m_cameraChangedConn;
    Util::ScopedConnection m_geometryChangedConn;

    Qt::MouseButtons m_pressedButtons;        ///< Currently pressed mouse buttons
    Qt::KeyboardModifiers m_pressedModifiers; ///< Currently pressed keyboard modifiers

    QPointF m_cursorPos;           ///< Latest cursor position (for hover picking)
    qreal m_devicePixelRatio{1.0}; ///< Cached device pixel ratio
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
    QSizeF m_itemSize;

    QPointF m_cursorPos;
    qreal m_devicePixelRatio{1.0};

    std::unique_ptr<QOpenGLFramebufferObject> m_pickFbo;
    Geometry::EntityType m_lastHoverType{Geometry::EntityType::None};
    Geometry::EntityUID m_lastHoverUid{Geometry::INVALID_ENTITY_UID};
};

} // namespace OpenGeoLab::App
