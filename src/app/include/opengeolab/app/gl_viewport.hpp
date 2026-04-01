/**
 * @file gl_viewport.hpp
 * @brief QQuickFramebufferObject subclass for 3D rendering viewport
 */

#pragma once

#include <opengeolab/app/camera_state.hpp>
#include <opengeolab/app/trackball_controller.hpp>
#include <opengeolab/core/pick_action.hpp>
#include <opengeolab/render/pick_mask.hpp>
#include <opengeolab/render/pick_result.hpp>

#include <QPointF>
#include <QQuickFramebufferObject>
#include <QRectF>
#include <QtQml/qqmlregistration.h>

namespace OpenGeoLab::Scene {
class SceneGraph;
} // namespace OpenGeoLab::Scene

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
    Q_PROPERTY(bool boxSelectActive READ boxSelectActive NOTIFY boxSelectActiveChanged)
    Q_PROPERTY(QRectF boxSelectRect READ boxSelectRect NOTIFY boxSelectRectChanged)

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
        Core::PickAction action{Core::PickAction::Add};
    };

    /// Pending box-select request consumed by the renderer.
    struct PendingBoxSelect {
        bool active{false};
        float x1{0.0F};
        float y1{0.0F};
        float x2{0.0F};
        float y2{0.0F};
        Core::PickAction action{Core::PickAction::Add};
    };

    /**
     * @brief Return and clear the pending click pick request.
     * @return Snapshot of the pending click pick, if any.
     */
    [[nodiscard]] PendingPick consumePendingPick();

    /**
     * @brief Return and clear the pending box-select request.
     * @return Snapshot of the pending box select, if any.
     */
    [[nodiscard]] PendingBoxSelect consumePendingBoxSelect();

    /** @brief Whether a box-select drag is in progress. */
    [[nodiscard]] bool boxSelectActive() const { return m_boxSelectActive; }

    /** @brief Current rubber-band rectangle in item coordinates. */
    [[nodiscard]] QRectF boxSelectRect() const { return m_boxSelectRect; }

    /** @brief Whether selection mode is active (from SelectionState). */
    [[nodiscard]] bool selectionActive() const { return m_selectionActive; }

    /** @brief Set selection-active flag (called from synchronize()). */
    void setSelectionActive(bool active) { m_selectionActive = active; }

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

Q_SIGNALS:
    void pickingEnabledChanged();
    void pickModeChanged();
    void xRayModeChanged();
    void entityPicked(int shape_id, int entity_type, int local_id);
    void entityHovered(int shape_id, int entity_type, int local_id);
    void pickCleared();
    void boxSelectActiveChanged();
    void boxSelectRectChanged();

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
    PendingBoxSelect m_pendingBoxSelect;
    QPointF m_hoverPos;
    bool m_hoverActive{false};
    bool m_boxSelectActive{false};
    QRectF m_boxSelectRect;
    bool m_selectionActive{false}; ///< Synced from SelectionState::pickEnabled()

    QPointF m_pressPos;
    bool m_movedSincePress{false};
    Qt::MouseButtons m_pressedButtons{Qt::NoButton};
    Qt::KeyboardModifiers m_pressedModifiers{Qt::NoModifier};
};

} // namespace OpenGeoLab::App
