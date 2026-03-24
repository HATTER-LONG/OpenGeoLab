/**
 * @file viewport_controller.cpp
 * @brief Implements the ViewportController mouse-to-camera translator.
 */
#include <opengeolab/app/viewport_controller.hpp>

namespace OpenGeoLab::App {

ViewportController::ViewportController(Render::Camera& camera, QObject* parent)
    : QObject(parent), camera_(camera) {}

void ViewportController::onMousePress(const QPointF& pos,
                                      Qt::MouseButtons buttons,
                                      Qt::KeyboardModifiers modifiers) {
    lastPos_ = pos;

    if(buttons & Qt::MiddleButton) {
        if(modifiers & Qt::ShiftModifier) {
            panning_ = true;
            orbiting_ = false;
        } else {
            orbiting_ = true;
            panning_ = false;
        }
    }
}

void ViewportController::onMouseMove(const QPointF& pos,
                                     Qt::MouseButtons buttons,
                                     Qt::KeyboardModifiers modifiers) {
    if(!(buttons & Qt::MiddleButton)) {
        orbiting_ = false;
        panning_ = false;
        return;
    }

    const float dx = static_cast<float>(pos.x() - lastPos_.x());
    const float dy = static_cast<float>(pos.y() - lastPos_.y());
    lastPos_ = pos;

    if(modifiers & Qt::ShiftModifier) {
        panning_ = true;
        orbiting_ = false;
    } else if(orbiting_ || !panning_) {
        orbiting_ = true;
        panning_ = false;
    }

    if(orbiting_) {
        constexpr float kOrbitSensitivity = 0.005F;
        camera_.orbit(-dx * kOrbitSensitivity, -dy * kOrbitSensitivity);
        emit cameraChanged();
    } else if(panning_) {
        constexpr float kPanSensitivity = 0.01F;
        camera_.pan(-dx * kPanSensitivity, dy * kPanSensitivity);
        emit cameraChanged();
    }
}

void ViewportController::onMouseRelease(const QPointF& /*pos*/) {
    orbiting_ = false;
    panning_ = false;
}

void ViewportController::onWheel(float angleDelta) {
    constexpr float kZoomStep = 1.1F;
    if(angleDelta > 0.0F) {
        camera_.zoom(kZoomStep);
    } else if(angleDelta < 0.0F) {
        camera_.zoom(1.0F / kZoomStep);
    }

    emit cameraChanged();
}

} // namespace OpenGeoLab::App
