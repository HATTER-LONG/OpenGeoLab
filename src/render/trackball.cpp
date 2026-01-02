/**
 * @file trackball.cpp
 * @brief Implementation of virtual trackball for 3D rotation
 */

#include "render/trackball.hpp"

#include <QVector3D>
#include <QtMath>


namespace OpenGeoLab {
namespace Render {

Trackball::Trackball() = default;

void Trackball::setViewportSize(const QSize& size) { m_viewportSize = size; }

void Trackball::begin(int x, int y) { normalizeCoordinates(x, y, m_lastX, m_lastY); }

QQuaternion Trackball::rotate(int x, int y) {
    float currX, currY;
    normalizeCoordinates(x, y, currX, currY);

    QQuaternion delta = computeRotation(m_lastX, m_lastY, currX, currY);

    // Accumulate rotation (pre-multiply for trackball behavior)
    m_rotation = delta * m_rotation;

    m_lastX = currX;
    m_lastY = currY;

    return delta;
}

void Trackball::reset() { m_rotation = QQuaternion(); }

void Trackball::normalizeCoordinates(int x, int y, float& nx, float& ny) const {
    // Map pixel coordinates to [-1, 1] range
    nx = (2.0f * x - m_viewportSize.width()) / m_viewportSize.width();
    ny = (m_viewportSize.height() - 2.0f * y) / m_viewportSize.height();
}

float Trackball::projectToSphere(float x, float y) const {
    float r = m_trackballSize;
    float d = qSqrt(x * x + y * y);

    if(d < r * 0.70710678118654752440f) {
        // Inside the sphere - use sphere
        return qSqrt(r * r - d * d);
    } else {
        // Outside - use hyperbolic sheet
        float t = r / 1.41421356237309504880f;
        return t * t / d;
    }
}

QQuaternion Trackball::computeRotation(float p1x, float p1y, float p2x, float p2y) const {
    if(qAbs(p1x - p2x) < 0.0001f && qAbs(p1y - p2y) < 0.0001f) {
        return QQuaternion();
    }

    // Get 3D points on trackball surface
    QVector3D p1(p1x, p1y, projectToSphere(p1x, p1y));
    QVector3D p2(p2x, p2y, projectToSphere(p2x, p2y));

    // Rotation axis is cross product of p1 and p2
    QVector3D axis = QVector3D::crossProduct(p1, p2);

    // Rotation angle is based on the arc length
    float t = (p1 - p2).length() / (2.0f * m_trackballSize);
    t = std::clamp(t, -1.0f, 1.0f);
    float angle = qRadiansToDegrees(2.0f * qAsin(t));

    return QQuaternion::fromAxisAndAngle(axis.normalized(), angle);
}

} // namespace Render
} // namespace OpenGeoLab
