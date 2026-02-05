#pragma once

#include "render/render_scene_controller.hpp"

#include <QPointF>
#include <QQuaternion>
#include <QSizeF>
#include <QVector2D>
#include <QVector3D>

namespace OpenGeoLab::Render {

class TrackballController {
public:
    enum class Mode {
        None,
        Orbit,
        Pan,
        Zoom,
    };

    TrackballController();

    void setViewportSize(const QSizeF& size);
    void setSpeed(float speed);

    void syncFromCamera(const Render::CameraState& camera);

    bool isActive() const;
    Mode mode() const;

    void begin(const QPointF& pos, Mode mode, const Render::CameraState& camera);
    void update(const QPointF& pos, Render::CameraState& camera);
    void end();

    void wheelZoom(float steps, Render::CameraState& camera);

private:
    static QVector3D normalizedOrZero(const QVector3D& v);
    static float clampMin(float v, float min_v);

    void computePointOnSphere(const QVector2D& p, QVector3D& out) const;
    QQuaternion rotationBetweenVectors(const QVector3D& u, const QVector3D& v) const;

    void updateCameraEyeUp(bool update_eye, bool update_up, Render::CameraState& camera);
    QVector3D computeCameraEye(const Render::CameraState& camera);
    QVector3D computeCameraUp();
    QVector3D computePan(const Render::CameraState& camera, const QVector2D& delta) const;

    void applyOrbit(Render::CameraState& camera);
    void applyPan(const QVector2D& delta, Render::CameraState& camera);
    void applyZoomFromDelta(const QVector2D& delta, Render::CameraState& camera);

    void addScrollImpulse(float steps);

    void freezeFromCamera(const Render::CameraState& camera);

private:
    QSizeF m_viewportSize{1.0, 1.0};
    float m_speed{1.0f};

    Mode m_mode{Mode::None};
    bool m_dragging{false};

    QVector2D m_clickPos{0.0f, 0.0f};
    QVector2D m_prevPos{0.0f, 0.0f};

    QVector3D m_startVec{0.0f, 0.0f, 1.0f};
    QVector3D m_stopVec{0.0f, 0.0f, 1.0f};

    QQuaternion m_rotation;
    QQuaternion m_rotationSum;

    float m_translateLength{50.0f};

    float m_orbitScale{2.2f};
    float m_panScale{0.0015f};
    float m_zoomSpeed{1.5f};
    float m_zoomBase{0.90f};
    float m_zoomPixelsPerStep{60.0f};

    float m_zoomSum{0.0f};
};

} // namespace OpenGeoLab::Render
