#include "render/trackball_controller.hpp"

#include <QtMath>

namespace OpenGeoLab::Render {
namespace {
constexpr QVector3D K_AXIS_X(1.0f, 0.0f, 0.0f);
constexpr QVector3D K_AXIS_Y(0.0f, 1.0f, 0.0f);
constexpr QVector3D K_AXIS_Z(0.0f, 0.0f, 1.0f);

[[nodiscard]] float lengthSquared(const QVector3D& v) { return QVector3D::dotProduct(v, v); }

[[nodiscard]] QQuaternion quatInverseUnit(const QQuaternion& q) {
    // For unit quaternions, inverse == conjugate.
    return q.conjugated();
}

[[nodiscard]] QQuaternion
quatFromBasis(const QVector3D& x, const QVector3D& y, const QVector3D& z) {
    // Rotation matrix with columns (x, y, z) in world space.
    const float m00 = x.x();
    const float m01 = y.x();
    const float m02 = z.x();

    const float m10 = x.y();
    const float m11 = y.y();
    const float m12 = z.y();

    const float m20 = x.z();
    const float m21 = y.z();
    const float m22 = z.z();

    const float trace = m00 + m11 + m22;

    float qw = 1.0f;
    float qx = 0.0f;
    float qy = 0.0f;
    float qz = 0.0f;

    if(trace > 0.0f) {
        const float s = qSqrt(trace + 1.0f) * 2.0f;
        qw = 0.25f * s;
        qx = (m21 - m12) / s;
        qy = (m02 - m20) / s;
        qz = (m10 - m01) / s;
    } else if((m00 > m11) && (m00 > m22)) {
        const float s = qSqrt(1.0f + m00 - m11 - m22) * 2.0f;
        qw = (m21 - m12) / s;
        qx = 0.25f * s;
        qy = (m01 + m10) / s;
        qz = (m02 + m20) / s;
    } else if(m11 > m22) {
        const float s = qSqrt(1.0f + m11 - m00 - m22) * 2.0f;
        qw = (m02 - m20) / s;
        qx = (m01 + m10) / s;
        qy = 0.25f * s;
        qz = (m12 + m21) / s;
    } else {
        const float s = qSqrt(1.0f + m22 - m00 - m11) * 2.0f;
        qw = (m10 - m01) / s;
        qx = (m02 + m20) / s;
        qy = (m12 + m21) / s;
        qz = 0.25f * s;
    }

    QQuaternion q(qw, qx, qy, qz);
    q.normalize();
    return q;
}

} // namespace

TrackballController::TrackballController()
    : m_rotation(1.0f, 0.0f, 0.0f, 0.0f), m_rotationSum(1.0f, 0.0f, 0.0f, 0.0f) {}

void TrackballController::setViewportSize(const QSizeF& size) {
    if(size.width() > 1.0 && size.height() > 1.0) {
        m_viewportSize = size;
    }
}

void TrackballController::setSpeed(float speed) { m_speed = speed; }

void TrackballController::syncFromCamera(const Render::CameraState& camera) {
    freezeFromCamera(camera);
}

bool TrackballController::isActive() const { return m_dragging; }

TrackballController::Mode TrackballController::mode() const { return m_mode; }

void TrackballController::begin(const QPointF& pos, Mode mode, const Render::CameraState& camera) {
    m_mode = mode;
    m_dragging = (mode != Mode::None);

    m_prevPos = QVector2D(static_cast<float>(pos.x()), static_cast<float>(pos.y()));
    m_clickPos = m_prevPos;

    freezeFromCamera(camera);

    if(m_dragging && m_mode == Mode::Orbit) {
        computePointOnSphere(m_clickPos, m_startVec);
    }
}

void TrackballController::update(const QPointF& pos, Render::CameraState& camera) {
    if(!m_dragging || m_mode == Mode::None) {
        return;
    }

    m_clickPos = QVector2D(static_cast<float>(pos.x()), static_cast<float>(pos.y()));

    const QVector2D delta = m_clickPos - m_prevPos;
    if(qFuzzyIsNull(delta.x()) && qFuzzyIsNull(delta.y())) {
        return;
    }

    switch(m_mode) {
    case Mode::Orbit: {
        computePointOnSphere(m_clickPos, m_stopVec);
        m_rotation = rotationBetweenVectors(m_startVec, m_stopVec);
        // Invert the relative rotation so the scene follows the cursor direction.
        m_rotation = quatInverseUnit(m_rotation);
        applyOrbit(camera);
        m_startVec = m_stopVec;
        break;
    }
    case Mode::Pan:
        applyPan(delta, camera);
        break;
    case Mode::Zoom:
        applyZoomFromDelta(delta, camera);
        break;
    default:
        break;
    }

    m_prevPos = m_clickPos;
}

void TrackballController::end() {
    m_dragging = false;
    m_mode = Mode::None;
    m_zoomSum = 0.0f;
}

void TrackballController::wheelZoom(float steps, Render::CameraState& camera) {
    if(qFuzzyIsNull(steps)) {
        return;
    }

    // Wheel-up typically means zoom in.
    addScrollImpulse(steps);
    updateCameraEyeUp(true, false, camera);
}

QVector3D TrackballController::normalizedOrZero(const QVector3D& v) {
    const float len2 = lengthSquared(v);
    if(len2 <= 1.0e-12f) {
        return QVector3D(0.0f, 0.0f, 0.0f);
    }
    return v / qSqrt(len2);
}

float TrackballController::clampMin(float v, float min_v) { return (v < min_v) ? min_v : v; }

void TrackballController::computePointOnSphere(const QVector2D& p, QVector3D& out) const {
    const float w = static_cast<float>(m_viewportSize.width());
    const float h = static_cast<float>(m_viewportSize.height());

    // Map to [-1, 1] range, origin at center. Qt item coords origin is top-left.
    const float x = (2.0f * p.x() - w) / w;
    const float y = (h - 2.0f * p.y()) / h;

    const float r2 = x * x + y * y;

    float z = 0.0f;
    if(r2 <= 0.5f) {
        z = qSqrt(1.0f - r2);
    } else {
        z = 0.5f / qSqrt(r2);
    }

    const float inv_norm = 1.0f / qSqrt(r2 + z * z);
    out = QVector3D(x * inv_norm, y * inv_norm, z * inv_norm);
}

QQuaternion TrackballController::rotationBetweenVectors(const QVector3D& u,
                                                        const QVector3D& v) const {
    const float cos_theta = QVector3D::dotProduct(u, v);
    constexpr float eps = 1.0e-5f;

    if(cos_theta < -1.0f + eps) {
        QVector3D axis = QVector3D::crossProduct(K_AXIS_Z, u);
        if(lengthSquared(axis) < 1.0e-2f) {
            axis = QVector3D::crossProduct(K_AXIS_X, u);
        }
        axis = normalizedOrZero(axis);
        return QQuaternion::fromAxisAndAngle(axis, 180.0f);
    }

    // when cos_theta is close to 1, the model hasn't moved enough to create a meaningful rotation.
    // if(cos_theta > 1.0f - eps) {
    //     return QQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
    // }

    const float theta = qAcos(qBound(-1.0f, cos_theta, 1.0f));
    QVector3D axis = QVector3D::crossProduct(u, v);
    axis = normalizedOrZero(axis);

    const float deg = qRadiansToDegrees(theta * m_speed * m_orbitScale);
    return QQuaternion::fromAxisAndAngle(axis, deg);
}

QVector3D TrackballController::computeCameraEye(const Render::CameraState& camera) {
    const QVector3D orientation = m_rotationSum.rotatedVector(K_AXIS_Z);

    if(!qFuzzyIsNull(m_zoomSum)) {
        // Exponential zoom feels consistent across scales.
        // steps > 0 => zoom in (distance decreases)
        const float factor = qPow(m_zoomBase, m_zoomSum);
        m_translateLength *= factor;
        m_translateLength = clampMin(m_translateLength, 0.1f);
        m_zoomSum = 0.0f;
    }

    return camera.m_target + (orientation * m_translateLength);
}

QVector3D TrackballController::computeCameraUp() {
    return normalizedOrZero(m_rotationSum.rotatedVector(K_AXIS_Y));
}

QVector3D TrackballController::computePan(const Render::CameraState& camera,
                                          const QVector2D& delta) const {
    const QVector3D look = camera.m_position - camera.m_target;
    const float distance = look.length();

    const QVector3D right = normalizedOrZero(m_rotationSum.rotatedVector(K_AXIS_X));
    const QVector3D up = normalizedOrZero(camera.m_up);

    // Scale pan with distance so it feels consistent.
    // Make model follow mouse: drag right => scene right, so camera pans left.
    const QVector3D pan = (up * delta.y() + right * -delta.x()) * (m_panScale * m_speed * distance);
    return pan;
}

void TrackballController::updateCameraEyeUp(bool update_eye,
                                            bool update_up,
                                            Render::CameraState& camera) {
    if(update_eye) {
        camera.m_position = computeCameraEye(camera);
    }
    if(update_up) {
        const QVector3D up = computeCameraUp();
        if(lengthSquared(up) > 1.0e-10f) {
            camera.m_up = up;
        }
    }

    const float dist = (camera.m_position - camera.m_target).length();
    camera.updateClipping(dist);
}

void TrackballController::applyOrbit(Render::CameraState& camera) {
    m_rotationSum = m_rotationSum * m_rotation;
    updateCameraEyeUp(true, true, camera);
}

void TrackballController::applyPan(const QVector2D& delta, Render::CameraState& camera) {
    const QVector3D pan = computePan(camera, delta);
    camera.m_target += pan;
    camera.m_position += pan;

    freezeFromCamera(camera);
    const float dist = (camera.m_position - camera.m_target).length();
    camera.updateClipping(dist);
}

void TrackballController::applyZoomFromDelta(const QVector2D& delta, Render::CameraState& camera) {
    // Choose dominant axis so diagonal drags still feel like zoom.
    const float ax = qAbs(delta.x());
    const float ay = qAbs(delta.y());

    float dominant = 0.0f;
    if(ay >= ax) {
        // Drag up => zoom in.
        dominant = -delta.y();
    } else {
        dominant = -delta.x();
    }

    const float steps = dominant / m_zoomPixelsPerStep;
    addScrollImpulse(steps);

    updateCameraEyeUp(true, false, camera);
}

void TrackballController::addScrollImpulse(float steps) {
    // steps > 0 means zoom-in.
    m_zoomSum += steps * m_speed * m_zoomSpeed;
}

void TrackballController::freezeFromCamera(const Render::CameraState& camera) {
    const QVector3D z = normalizedOrZero(camera.m_position - camera.m_target);
    if(lengthSquared(z) <= 1.0e-10f) {
        m_rotationSum = QQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
        m_translateLength = 0.1f;
        return;
    }

    QVector3D x = QVector3D::crossProduct(camera.m_up, z);
    if(lengthSquared(x) <= 1.0e-8f) {
        // Fallback if up is parallel to view direction.
        const QVector3D fallback = (qAbs(z.y()) < 0.999f) ? K_AXIS_Y : K_AXIS_X;
        x = QVector3D::crossProduct(fallback, z);
    }
    x = normalizedOrZero(x);

    const QVector3D y = normalizedOrZero(QVector3D::crossProduct(z, x));

    m_rotationSum = quatFromBasis(x, y, z);
    m_translateLength = (camera.m_position - camera.m_target).length();
}

} // namespace OpenGeoLab::Render
