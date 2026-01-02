/**
 * @file trackball.hpp
 * @brief Virtual trackball implementation for intuitive 3D rotation
 *
 * Based on the classic SGI trackball algorithm. Projects mouse positions
 * onto a virtual sphere/hyperbolic sheet and computes rotation quaternions
 * from the arc between two points.
 */

#pragma once

#include <QQuaternion>
#include <QSize>

namespace OpenGeoLab {
namespace Render {

/**
 * @brief Virtual trackball for intuitive 3D rotation control
 *
 * Implements the classic trackball algorithm that maps 2D mouse movement
 * to 3D rotation. The trackball is a hybrid of a sphere in the center
 * and a hyperbolic sheet at the edges, providing smooth rotation behavior.
 *
 * Usage:
 * 1. Call setViewportSize() when viewport changes
 * 2. Call begin() when mouse button is pressed
 * 3. Call rotate() during mouse drag to get incremental rotation
 * 4. Apply the returned quaternion to your model rotation
 */
class Trackball {
public:
    Trackball();

    /**
     * @brief Set the viewport size for coordinate normalization
     * @param size Viewport size in pixels
     */
    void setViewportSize(const QSize& size);

    /**
     * @brief Begin a rotation operation
     * @param x Mouse x coordinate in pixels
     * @param y Mouse y coordinate in pixels
     */
    void begin(int x, int y);

    /**
     * @brief Calculate rotation from current to new mouse position
     * @param x New mouse x coordinate in pixels
     * @param y New mouse y coordinate in pixels
     * @return Quaternion representing the incremental rotation
     */
    QQuaternion rotate(int x, int y);

    /**
     * @brief Get the accumulated rotation quaternion
     * @return Current total rotation
     */
    QQuaternion rotation() const { return m_rotation; }

    /**
     * @brief Set the rotation quaternion directly
     * @param rotation New rotation quaternion
     */
    void setRotation(const QQuaternion& rotation) { m_rotation = rotation; }

    /**
     * @brief Reset rotation to identity
     */
    void reset();

    /**
     * @brief Set trackball size (affects rotation sensitivity)
     * @param size Trackball size (default 0.8)
     */
    void setTrackballSize(float size) { m_trackballSize = size; }

    /**
     * @brief Get current trackball size
     * @return Trackball size
     */
    float trackballSize() const { return m_trackballSize; }

private:
    void normalizeCoordinates(int x, int y, float& nx, float& ny) const;
    float projectToSphere(float x, float y) const;
    QQuaternion computeRotation(float p1x, float p1y, float p2x, float p2y) const;

    QSize m_viewportSize = QSize(800, 600);
    float m_lastX = 0.0f;
    float m_lastY = 0.0f;
    QQuaternion m_rotation;
    float m_trackballSize = 0.8f;
};

} // namespace Render
} // namespace OpenGeoLab
