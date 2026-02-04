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
#include "util/signal.hpp"

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
 * - OpenGL rendering of geometry from RenderSceneController via SceneRenderer
 * - Mouse-based camera manipulation (orbit, pan, zoom)
 * - Entity picking with type filtering and snap-to-entity
 * - Integration with the application's render service
 *
 * @note This class handles QML integration and input; rendering logic is
 *       delegated to Render::SceneRenderer.
 */
class GLViewport : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool hasGeometry READ hasGeometry NOTIFY hasGeometryChanged)
    Q_PROPERTY(bool pickModeEnabled READ pickModeEnabled WRITE setPickModeEnabled NOTIFY
                   pickModeEnabledChanged)
    Q_PROPERTY(Geometry::EntityType pickEntityType READ pickEntityType WRITE setPickEntityType
                   NOTIFY pickEntityTypeChanged)
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

    // =========================================================
    // Entity picking interface
    // =========================================================

    [[nodiscard]] bool pickModeEnabled() const;

    void setPickModeEnabled(bool enabled);

    [[nodiscard]] Geometry::EntityType pickEntityType() const;

    void setPickEntityType(const Geometry::EntityType& type);

    Q_INVOKABLE void requestPick(int x, int y, int radius = 5);

    [[nodiscard]] const Render::PickPixelResult& lastPickResult() const;

    bool consumePickRequest(int& x, int& y, int& radius, Geometry::EntityType& filter_type);

    void handlePickResult(const Render::PickPixelResult& result);

signals:
    void hasGeometryChanged();
    void geometryChanged();
    void pickModeEnabledChanged();
    void pickEntityTypeChanged();

    void entityPicked(const QString& entity_type, int entity_uid);

    void pickCancelled();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onSceneNeedsUpdate();
    void onServiceGeometryChanged();

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

    void performPick(int x, int y, int radius);

private:
    Render::CameraState m_cameraState; ///< Local camera state copy

    bool m_hasGeometry{false};
    bool m_pickModeEnabled{false};            ///< Whether pick mode is active
    Geometry::EntityType m_pickEntityType;    ///< Entity type filter for picking
    Render::PickPixelResult m_lastPickResult; ///< Last pick operation result

    // Pick request data for communication with renderer
    struct PickRequest {
        bool m_pending{false}; ///< Whether there's a pending pick request
        int m_x{0};            ///< Screen X coordinate
        int m_y{0};            ///< Screen Y coordinate
        int m_radius{5};       ///< Search radius
        Geometry::EntityType m_filterType{Geometry::EntityType::None}; ///< Entity type filter
    };
    PickRequest m_pendingPickRequest; ///< Pending pick request for renderer
    Util::ScopedConnection m_sceneNeedsUpdateConn;
    Util::ScopedConnection m_cameraChangedConn;
    Util::ScopedConnection m_geometryChangedConn;

    QPointF m_lastMousePos;                   ///< Last mouse position for delta calculation
    Qt::MouseButtons m_pressedButtons;        ///< Currently pressed mouse buttons
    Qt::KeyboardModifiers m_pressedModifiers; ///< Currently pressed keyboard modifiers
};

/**
 * @brief OpenGL renderer for GLViewport
 *
 * Handles the actual OpenGL rendering of geometry meshes using SceneRenderer.
 * Also supports ID buffer rendering for entity picking.
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
    [[nodiscard]] Render::PickPixelResult
    pickAt(int x, int y, int radius, Geometry::EntityType filter_type);

private:
    void performPickInRenderThread(const QMatrix4x4& view, const QMatrix4x4& projection);

private:
    const GLViewport* m_viewport{nullptr};                  ///< Parent viewport item
    std::unique_ptr<Render::SceneRenderer> m_sceneRenderer; ///< Rendering component

    bool m_needsDataUpload{false}; ///< Whether mesh data needs uploading

    // Cached render data
    Render::DocumentRenderData m_renderData;
    Render::CameraState m_cameraState;
    QSize m_viewportSize;

    // Pending pick request data
    bool m_pendingPick{false}; ///< Whether there's a pending pick
    int m_pickX{0};            ///< Pick screen X
    int m_pickY{0};            ///< Pick screen Y
    int m_pickRadius{5};       ///< Pick search radius
    Geometry::EntityType m_pickFilterType{Geometry::EntityType::None}; ///< Pick type filter
};

} // namespace OpenGeoLab::App
