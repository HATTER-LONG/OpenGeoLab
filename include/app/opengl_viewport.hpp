
#pragma once

#include "render/render_scene.hpp"
#include "render/render_scene_controller.hpp"
#include "render/render_types.hpp"
#include "render/trackball_controller.hpp"
#include "util/signal.hpp"

#include <QMatrix4x4>
#include <QQuickFramebufferObject>
#include <QSizeF>
#include <QtQml/qqml.h>

namespace OpenGeoLab::App {

class GLViewport : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit GLViewport(QQuickItem* parent = nullptr);
    ~GLViewport() override;

    [[nodiscard]] bool consumePendingPickingInput(Render::PickingInput& input) const;

    /**
     * @brief Create the renderer for this viewport
     * @return New renderer instance
     */
    QQuickFramebufferObject::Renderer* createRenderer() const override;

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
    // void onRenderNeedsUpdate();

private:
    Render::CameraState m_cameraState;                 ///< Local camera state copy
    Render::TrackballController m_trackballController; ///< Camera manipulation controller

    Util::ScopedConnection m_sceneNeedsUpdateConn;
    Util::ScopedConnection m_cameraChangedConn;
    Util::ScopedConnection m_selectionChangedConn;

    QPointF m_cursorPos;           ///< Latest cursor position (for hover picking)
    QPointF m_pressPos;            ///< Position at last mouse press (for click/drag detection)
    bool m_movedSincePress{false}; ///< Whether cursor moved beyond threshold since press

    qreal m_devicePixelRatio{1.0}; ///< Cached device pixel ratio

    Qt::MouseButtons m_pressedButtons;        ///< Currently pressed mouse buttons
    Qt::KeyboardModifiers m_pressedModifiers; ///< Currently pressed keyboard modifiers
    mutable Render::PickAction m_pendingPickAction{
        Render::PickAction::None}; ///< Pending pick action on mouse releasek
};

} // namespace OpenGeoLab::App
