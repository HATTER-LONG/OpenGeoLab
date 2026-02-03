/**
 * @file trackball.hpp
 * @brief Virtual trackball implementation for intuitive 3D rotation
 *
 * Based on the classic SGI trackball algorithm. Projects mouse positions
 * onto a virtual sphere/hyperbolic sheet and computes rotation quaternions
 * from the arc between two points.
 *
 * This provides intuitive "grab and rotate" behavior where the surface
 * under the cursor follows the mouse movement.
 */

#pragma once

#include <QQuaternion>
#include <QSize>

namespace OpenGeoLab {
namespace Rendering {

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
    /**
     * @brief Construct a trackball with default settings
     */
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
     *
     * This method also updates the internal state, so subsequent calls
     * will compute rotation from the new position.
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
     *
     * Larger values make the trackball more sensitive to mouse movement.
     * Values around 0.8-1.0 work well for most cases.
     */
    void setTrackballSize(float size) { m_trackballSize = size; }

    /**
     * @brief Get current trackball size
     * @return Trackball size
     */
    float trackballSize() const { return m_trackballSize; }

private:
    /**
     * @brief Normalize pixel coordinates to [-1, 1] range
     * @param x Pixel x coordinate
     * @param y Pixel y coordinate
     * @param nx Output normalized x
     * @param ny Output normalized y
     */
    void normalizeCoordinates(int x, int y, float& nx, float& ny) const;

    /**
     * @brief Project point onto trackball surface (sphere or hyperbola)
     * @param x Normalized x coordinate [-1, 1]
     * @param y Normalized y coordinate [-1, 1]
     * @return Z coordinate on the trackball surface
     */
    float projectToSphere(float x, float y) const;

    /**
     * @brief Compute rotation quaternion between two points on trackball
     * @param p1x First point x
     * @param p1y First point y
     * @param p2x Second point x
     * @param p2y Second point y
     * @return Quaternion representing rotation from p1 to p2
     */
    QQuaternion computeRotation(float p1x, float p1y, float p2x, float p2y) const;

    // Viewport dimensions
    QSize m_viewportSize = QSize(800, 600);

    // Last mouse position (normalized)
    float m_lastX = 0.0f;
    float m_lastY = 0.0f;

    // Accumulated rotation
    QQuaternion m_rotation;

    // Trackball size parameter (affects sensitivity)
    float m_trackballSize = 0.8f;
};

} // namespace Rendering
} // namespace OpenGeoLab
