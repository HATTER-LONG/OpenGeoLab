/**
 * @file viewport_controller.hpp
 * @brief Declares the mouse-to-camera interaction translator.
 */
#pragma once

#include <opengeolab/render/camera.hpp>

#include <QObject>
#include <QPointF>
#include <QtQml/qqml.h>

namespace OpenGeoLab::App {

/**
 * @brief Translates mouse events into camera operations.
 *
 * Middle-drag -> orbit, Shift+middle-drag -> pan, wheel -> zoom.
 * Left button reserved for picking (Phase 3), right for context menu.
 */
class ViewportController : public QObject {
    Q_OBJECT

public:
    explicit ViewportController(Render::Camera& camera, QObject* parent = nullptr);

    void
    onMousePress(const QPointF& pos, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void onMouseMove(const QPointF& pos, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void onMouseRelease(const QPointF& pos);
    void onWheel(float angleDelta);

signals:
    /** @brief Camera state changed, viewport should refresh. */
    void cameraChanged();

private:
    Render::Camera& camera_;
    QPointF lastPos_;
    bool orbiting_ = false;
    bool panning_ = false;
};

} // namespace OpenGeoLab::App
