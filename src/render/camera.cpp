/**
 * @file camera.cpp
 * @brief Implementation of camera system for 3D rendering
 */

#include "render/camera.hpp"

#include <QtMath>
#include <algorithm>

namespace OpenGeoLab {
namespace Render {

Camera::Camera(QObject* parent) : QObject(parent) { updateCameraVectors(); }

QMatrix4x4 Camera::viewMatrix() const {
    QMatrix4x4 view;
    view.lookAt(position(), m_target, m_upVector);
    return view;
}

QMatrix4x4 Camera::projectionMatrix(float aspectRatio) const {
    QMatrix4x4 projection;
    projection.perspective(m_fov, aspectRatio, m_nearPlane, m_farPlane);
    return projection;
}

QMatrix4x4 Camera::viewProjectionMatrix(float aspectRatio) const {
    return projectionMatrix(aspectRatio) * viewMatrix();
}

QVector3D Camera::position() const {
    // Calculate camera position from orbit parameters
    float yawRad = qDegreesToRadians(m_yaw);
    float pitchRad = qDegreesToRadians(m_pitch);

    float x = m_distance * qCos(pitchRad) * qSin(yawRad);
    float y = m_distance * qSin(pitchRad);
    float z = m_distance * qCos(pitchRad) * qCos(yawRad);

    return m_target + QVector3D(x, y, z);
}

void Camera::setTarget(const QVector3D& target) {
    m_target = target;
    emit cameraChanged();
}

void Camera::orbit(float deltaYaw, float deltaPitch) {
    m_yaw += deltaYaw;
    m_pitch = std::clamp(m_pitch + deltaPitch, MIN_PITCH, MAX_PITCH);

    // Normalize yaw to [0, 360)
    while(m_yaw >= 360.0f) {
        m_yaw -= 360.0f;
    }
    while(m_yaw < 0.0f) {
        m_yaw += 360.0f;
    }

    emit cameraChanged();
}

void Camera::setOrbitAngles(float yaw, float pitch) {
    m_yaw = yaw;
    m_pitch = std::clamp(pitch, MIN_PITCH, MAX_PITCH);
    emit cameraChanged();
}

void Camera::zoom(float factor) {
    m_distance = std::clamp(m_distance / factor, MIN_DISTANCE, MAX_DISTANCE);
    updateClippingPlanes();
    emit cameraChanged();
}

void Camera::setDistance(float distance) {
    m_distance = std::clamp(distance, MIN_DISTANCE, MAX_DISTANCE);
    updateClippingPlanes();
    emit cameraChanged();
}

void Camera::pan(float deltaX, float deltaY) {
    // Calculate right and up vectors in view space
    float yawRad = qDegreesToRadians(m_yaw);
    float pitchRad = qDegreesToRadians(m_pitch);

    QVector3D forward(qCos(pitchRad) * qSin(yawRad), qSin(pitchRad), qCos(pitchRad) * qCos(yawRad));
    forward.normalize();

    QVector3D right = QVector3D::crossProduct(forward, m_upVector).normalized();
    QVector3D up = QVector3D::crossProduct(right, forward).normalized();

    // Pan based on distance for consistent feel
    float panScale = m_distance * 0.002f;
    m_target += right * (deltaX * panScale) + up * (deltaY * panScale);

    emit cameraChanged();
}

void Camera::panByPixel(int prevX, int prevY, int currX, int currY) {
    float deltaX = static_cast<float>(currX - prevX);
    float deltaY = static_cast<float>(prevY - currY); // Invert Y for screen coordinates

    pan(deltaX, deltaY);
}

void Camera::setViewportSize(int width, int height) {
    m_viewportWidth = width;
    m_viewportHeight = height;
}

void Camera::fitToBounds(const QVector3D& minPoint, const QVector3D& maxPoint, float margin) {
    QVector3D center = (minPoint + maxPoint) * 0.5f;
    QVector3D size = maxPoint - minPoint;
    float maxSize = std::max({size.x(), size.y(), size.z()});

    m_target = center;

    // Calculate distance to fit the geometry in view
    float halfFov = qDegreesToRadians(m_fov * 0.5f);
    m_distance = (maxSize * margin * 0.5f) / qTan(halfFov);
    m_distance = std::clamp(m_distance, MIN_DISTANCE, MAX_DISTANCE);

    updateClippingPlanes();
    emit cameraChanged();
}

void Camera::reset() {
    m_yaw = 45.0f;
    m_pitch = 30.0f;
    m_distance = 5.0f;
    m_target = QVector3D(0, 0, 0);
    m_fov = 45.0f;
    updateClippingPlanes();
    emit cameraChanged();
}

void Camera::setFieldOfView(float fov) {
    m_fov = std::clamp(fov, 1.0f, 179.0f);
    emit cameraChanged();
}

void Camera::setNearPlane(float nearPlane) {
    m_nearPlane = std::max(0.0001f, nearPlane);
    emit cameraChanged();
}

void Camera::setFarPlane(float farPlane) {
    m_farPlane = farPlane;
    emit cameraChanged();
}

void Camera::updateCameraVectors() {
    // Update up vector if needed (for future extensions)
}

void Camera::updateClippingPlanes() {
    // Automatically adjust clipping planes based on distance
    m_nearPlane = std::max(0.001f, m_distance * 0.001f);
    m_farPlane = std::max(m_distance * 100.0f, 10000.0f);
}

} // namespace Render
} // namespace OpenGeoLab
