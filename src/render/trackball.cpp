/**
 * @file trackball.cpp
 * @brief Implementation of virtual trackball for 3D rotation
 *
 * Based on the classic SGI trackball algorithm by Gavin Bell.
 * This implementation uses Qt's QQuaternion for rotation representation.
 */

#include <render/trackball.hpp>

#include <QtMath>
#include <cmath>

namespace OpenGeoLab {
namespace Rendering {

Trackball::Trackball() { reset(); }

void Trackball::setViewportSize(const QSize& size) { m_viewportSize = size; }

void Trackball::begin(int x, int y) { normalizeCoordinates(x, y, m_lastX, m_lastY); }

QQuaternion Trackball::rotate(int x, int y) {
    float nx, ny;
    normalizeCoordinates(x, y, nx, ny);

    // Compute incremental rotation
    QQuaternion delta_rotation = computeRotation(m_lastX, m_lastY, nx, ny);

    // Accumulate rotation (pre-multiply for intuitive behavior)
    m_rotation = delta_rotation * m_rotation;
    m_rotation.normalize();

    // Update last position
    m_lastX = nx;
    m_lastY = ny;

    return delta_rotation;
}

void Trackball::reset() { m_rotation = QQuaternion(); }

void Trackball::normalizeCoordinates(int x, int y, float& nx, float& ny) const {
    // Map pixel coordinates to [-1, 1] range
    // Note: Y is inverted because screen Y increases downward
    nx = (2.0f * x - m_viewportSize.width()) / m_viewportSize.width();
    ny = (m_viewportSize.height() - 2.0f * y) / m_viewportSize.height();
}

float Trackball::projectToSphere(float x, float y) const {
    // Project the point (x, y) onto the trackball surface.
    // The trackball is a deformed sphere: it's a sphere in the center,
    // but transitions to a hyperbolic sheet away from the center.
    // This provides smooth rotation behavior at all angles.

    float r = m_trackballSize;
    float d = std::sqrt(x * x + y * y);

    float z;
    if(d < r * 0.70710678118654752440f) {
        // Inside sphere: use sphere equation
        // z = sqrt(r^2 - d^2)
        z = std::sqrt(r * r - d * d);
    } else {
        // Outside sphere: use hyperbola
        // This provides a smooth transition and avoids the singularity
        // at the edge of the sphere
        float t = r / 1.41421356237309504880f; // r / sqrt(2)
        z = t * t / d;
    }

    return z;
}

QQuaternion Trackball::computeRotation(float p1x, float p1y, float p2x, float p2y) const {
    // Handle zero rotation case
    if(qFuzzyCompare(p1x, p2x) && qFuzzyCompare(p1y, p2y)) {
        return QQuaternion(); // Identity quaternion
    }

    // Project both points onto the trackball surface
    QVector3D p1(p1x, p1y, projectToSphere(p1x, p1y));
    QVector3D p2(p2x, p2y, projectToSphere(p2x, p2y));

    // The rotation axis is the cross product of the two vectors
    // (from origin to p1 and from origin to p2)
    // Use p1 x p2 so that dragging right rotates model right
    QVector3D axis = QVector3D::crossProduct(p1, p2);

    // Handle degenerate case where points are nearly parallel
    if(axis.lengthSquared() < 1e-10f) {
        return QQuaternion();
    }

    axis.normalize();

    // The rotation angle is related to the distance between the points
    QVector3D diff = p1 - p2;
    float t = diff.length() / (2.0f * m_trackballSize);

    // Clamp to avoid numerical issues with asin
    t = qBound(-1.0f, t, 1.0f);

    // Angle is twice the arcsin of half the chord length
    float angle = 2.0f * std::asin(t);

    // Convert to degrees for Qt's quaternion
    float angle_degrees = qRadiansToDegrees(angle);

    return QQuaternion::fromAxisAndAngle(axis, angle_degrees);
}

} // namespace Rendering
} // namespace OpenGeoLab
