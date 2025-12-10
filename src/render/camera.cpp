/**
 * @file camera.cpp
 * @brief Implementation of Camera class for 3D scene navigation
 */

#include <render/camera.hpp>

#include <QtMath>
#include <cmath>

namespace OpenGeoLab {
namespace Rendering {

Camera::Camera(QObject* parent) : QObject(parent) { updateCameraVectors(); }

QMatrix4x4 Camera::viewMatrix() const {
    QMatrix4x4 view;
    view.lookAt(position(), m_target, m_upVector);
    return view;
}

QMatrix4x4 Camera::projectionMatrix(float aspect_ratio) const {
    QMatrix4x4 projection;
    projection.perspective(m_fov, aspect_ratio, m_nearPlane, m_farPlane);
    return projection;
}

QMatrix4x4 Camera::viewProjectionMatrix(float aspect_ratio) const {
    return projectionMatrix(aspect_ratio) * viewMatrix();
}

QVector3D Camera::position() const {
    // Calculate camera position based on orbit parameters
    // Using spherical coordinates: distance, yaw (azimuth), pitch (elevation)
    float yaw_rad = qDegreesToRadians(m_yaw);
    float pitch_rad = qDegreesToRadians(m_pitch);

    float cos_pitch = std::cos(pitch_rad);
    float sin_pitch = std::sin(pitch_rad);
    float cos_yaw = std::cos(yaw_rad);
    float sin_yaw = std::sin(yaw_rad);

    // Camera position relative to target
    QVector3D offset(m_distance * cos_pitch * sin_yaw, m_distance * sin_pitch,
                     m_distance * cos_pitch * cos_yaw);

    return m_target + offset;
}

void Camera::setTarget(const QVector3D& target) {
    if(m_target != target) {
        m_target = target;
        emit cameraChanged();
    }
}

void Camera::orbit(float delta_yaw, float delta_pitch) {
    m_yaw += delta_yaw;
    m_pitch += delta_pitch;

    // Normalize yaw to [0, 360)
    while(m_yaw >= 360.0f) {
        m_yaw -= 360.0f;
    }
    while(m_yaw < 0.0f) {
        m_yaw += 360.0f;
    }

    // Allow full vertical rotation but keep pitch in reasonable range
    // This allows viewing from any angle including top/bottom
    while(m_pitch > 180.0f) {
        m_pitch -= 360.0f;
    }
    while(m_pitch < -180.0f) {
        m_pitch += 360.0f;
    }

    emit cameraChanged();
}

void Camera::setOrbitAngles(float yaw, float pitch) {
    m_yaw = yaw;
    m_pitch = pitch;

    // Normalize yaw
    while(m_yaw >= 360.0f) {
        m_yaw -= 360.0f;
    }
    while(m_yaw < 0.0f) {
        m_yaw += 360.0f;
    }

    // Normalize pitch
    while(m_pitch > 180.0f) {
        m_pitch -= 360.0f;
    }
    while(m_pitch < -180.0f) {
        m_pitch += 360.0f;
    }

    emit cameraChanged();
}

void Camera::zoom(float factor) {
    // Zoom by adjusting distance
    // factor > 1 means zoom in (closer), factor < 1 means zoom out (farther)
    m_distance /= factor;
    m_distance = qBound(MIN_DISTANCE, m_distance, MAX_DISTANCE);
    emit cameraChanged();
}

void Camera::setDistance(float distance) {
    m_distance = qBound(MIN_DISTANCE, distance, MAX_DISTANCE);
    emit cameraChanged();
}

void Camera::pan(float delta_x, float delta_y) {
    // Calculate right and up vectors in world space based on current view
    float yaw_rad = qDegreesToRadians(m_yaw);
    float pitch_rad = qDegreesToRadians(m_pitch);

    // Right vector (perpendicular to view direction in horizontal plane)
    QVector3D right(std::cos(yaw_rad), 0.0f, -std::sin(yaw_rad));

    // Up vector (perpendicular to both view and right)
    // For orbit camera, we use world up projected onto the view plane
    QVector3D forward = (m_target - position()).normalized();
    QVector3D up = QVector3D::crossProduct(right, forward).normalized();

    // Scale pan amount by distance for consistent feel at all zoom levels
    float pan_scale = m_distance * 0.001f;

    // Move target (and camera follows)
    m_target += right * delta_x * pan_scale;
    m_target += up * delta_y * pan_scale;

    emit cameraChanged();
}

void Camera::fitToBounds(const QVector3D& min_point, const QVector3D& max_point, float margin) {
    // Calculate bounding box center and size
    QVector3D center = (min_point + max_point) * 0.5f;
    QVector3D size = max_point - min_point;
    float max_size = qMax(qMax(size.x(), size.y()), size.z());

    // Set target to center of bounds
    m_target = center;

    // Calculate required distance to fit the object in view
    // Using FOV and the maximum dimension
    if(max_size > 0.0001f) {
        float half_fov_rad = qDegreesToRadians(m_fov * 0.5f);
        m_distance = (max_size * 0.5f * margin) / std::tan(half_fov_rad);
        m_distance = qBound(MIN_DISTANCE, m_distance, MAX_DISTANCE);
    }

    // Update clipping planes based on model size
    m_nearPlane = qMax(0.001f, m_distance * 0.001f);
    m_farPlane = qMax(m_distance * 10.0f, max_size * 20.0f);

    emit cameraChanged();
}

void Camera::reset() {
    m_yaw = 0.0f;
    m_pitch = 30.0f;
    m_distance = 5.0f;
    m_target = QVector3D(0, 0, 0);
    m_fov = 45.0f;
    m_nearPlane = 0.01f;
    m_farPlane = 10000.0f;

    emit cameraChanged();
}

void Camera::setFieldOfView(float fov) {
    m_fov = qBound(1.0f, fov, 179.0f);
    emit cameraChanged();
}

void Camera::setNearPlane(float near_plane) {
    m_nearPlane = qMax(0.0001f, near_plane);
    emit cameraChanged();
}

void Camera::setFarPlane(float far_plane) {
    m_farPlane = qMax(m_nearPlane + 0.001f, far_plane);
    emit cameraChanged();
}

void Camera::updateCameraVectors() {
    // This method can be used for additional calculations if needed
    // Currently, position is calculated on-demand in position()
}

} // namespace Rendering
} // namespace OpenGeoLab
