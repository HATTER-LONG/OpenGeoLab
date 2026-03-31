/**
 * @file gl_viewport.hpp
 * @brief QQuickFramebufferObject subclass for 3D rendering viewport
 */

#pragma once

#include <opengeolab/app/camera_state.hpp>
#include <opengeolab/app/trackball_controller.hpp>
#include <opengeolab/render/pick_mask.hpp>
#include <opengeolab/render/pick_result.hpp>

#include <QPointF>
#include <QQuickFramebufferObject>
#include <QtQml/qqmlregistration.h>

namespace OpenGeoLab::Scene {
class SceneGraph;
}

namespace OpenGeoLab::App {

class GLViewportRenderer;

/**
 * @brief Interactive 3D viewport for Qt Quick scene graph
 *
 * Provides orbit/pan/zoom via TrackballController and GPU picking.
 * Rendering is performed on the Qt render thread via GLViewportRenderer.
 */
class GLViewport : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool pickingEnabled READ pickingEnabled WRITE setPickingEnabled NOTIFY
                   pickingEnabledChanged)
    Q_PROPERTY(int pickMode READ pickMode WRITE setPickMode NOTIFY pickModeChanged)
    Q_PROPERTY(bool xRayMode READ xRayMode WRITE setXRayMode NOTIFY xRayModeChanged)

public:
    /**
     * @brief Construct a Qt Quick framebuffer viewport item.
     * @param parent Optional QQuickItem parent.
     */
    explicit GLViewport(QQuickItem* parent = nullptr);

    /**
     * @brief Create the render-thread renderer instance for this viewport.
     * @return Newly allocated renderer owned by Qt Quick.
     */
    [[nodiscard]] Renderer* createRenderer() const override;

    /**
     * @brief Assign the scene graph mirrored into the render pipeline.
     * @param scene Nullable scene graph pointer set by the C++ composition root.
     */
    void setSceneGraph(Scene::SceneGraph* scene);

    /** @brief Get the scene graph associated with this viewport. */
    [[nodiscard]] Scene::SceneGraph* sceneGraph() const { return m_sceneGraph; }

    /** @brief Get the camera state synchronized to the renderer. */
    [[nodiscard]] const CameraState& cameraState() const { return m_camera; }

    /** @brief Get mutable camera state for local interaction updates. */
    [[nodiscard]] CameraState& cameraState() { return m_camera; }

    /** @brief Whether click and hover picking are enabled. */
    [[nodiscard]] bool pickingEnabled() const { return m_pickingEnabled; }

    /** @brief Enable or disable pick requests from user input. */
    void setPickingEnabled(bool enabled);

    /** @brief Get the active pick mode as a QML-friendly integer. */
    [[nodiscard]] int pickMode() const { return static_cast<int>(m_pickMode); }

    /** @brief Set the active pick mode from a QML-friendly integer. */
    void setPickMode(int mode);

    /** @brief Whether x-ray rendering is enabled. */
    [[nodiscard]] bool xRayMode() const { return m_xRayMode; }

    /** @brief Enable or disable x-ray rendering. */
    void setXRayMode(bool enabled);

    /**
     * @brief Pending click pick request consumed by the renderer.
     *
     * Coordinates are in Qt item space with an origin at the top-left corner.
     */
    struct PendingPick {
        bool active{false};
        float x{0.0F};
        float y{0.0F};
    };

    /**
     * @brief Return and clear the pending click pick request.
     * @return Snapshot of the pending click pick, if any.
     */
    [[nodiscard]] PendingPick consumePendingPick();

    /**
     * @brief Forward a resolved click pick result to QML observers.
     * @param result Pick result generated on the render thread.
     */
    void notifyPickResult(const Render::PickResult& result);

    /**
     * @brief Forward a resolved hover pick result to QML observers.
     * @param result Hover result generated on the render thread.
     */
    void notifyHoverResult(const Render::PickResult& result);

    /** @brief Fit the camera to the current scene bounds, if available. */
    Q_INVOKABLE void fitToScene();

    /**
     * @brief Apply one of the TrackballController view presets.
     * @param preset Integer value of TrackballController::ViewPreset.
     */
    Q_INVOKABLE void setViewPreset(int preset);

    /** @brief Toggle x-ray rendering on or off. */
    Q_INVOKABLE void toggleXRay();

    /**
     * @brief Set the visibility of a scene node by its shape (node) id.
     * @param shapeId Integer node id in the scene graph.
     * @param visible Whether the shape should be visible.
     */
    Q_INVOKABLE void setShapeVisible(int shapeId, bool visible);

Q_SIGNALS:
    void pickingEnabledChanged();
    void pickModeChanged();
    void xRayModeChanged();
    void entityPicked(int shapeId, int entityType, int localId);
    void entityHovered(int shapeId, int entityType, int localId);
    void pickCleared();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;

private:
    [[nodiscard]] TrackballController::Mode mapMouseMode(Qt::MouseButtons buttons,
                                                         Qt::KeyboardModifiers modifiers) const;

    friend class GLViewportRenderer;

    Scene::SceneGraph* m_sceneGraph{nullptr};
    CameraState m_camera;
    TrackballController m_trackball;

    bool m_pickingEnabled{true};
    Render::PickMode m_pickMode{Render::PickMode::VEF};
    bool m_xRayMode{false};

    PendingPick m_pendingPick;
    QPointF m_hoverPos;
    bool m_hoverActive{false};

    QPointF m_pressPos;
    bool m_movedSincePress{false};
    Qt::MouseButtons m_pressedButtons{Qt::NoButton};
    Qt::KeyboardModifiers m_pressedModifiers{Qt::NoModifier};
};

} // namespace OpenGeoLab::App
