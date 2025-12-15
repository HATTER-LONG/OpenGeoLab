/**
 * @file camera.hpp
 * @brief Camera system for 3D rendering
 *
 * Provides a modular camera implementation supporting:
 * - Orbit rotation around a target point
 * - Zoom in/out with proper distance calculation
 * - Pan translation in view space
 * - Auto-fit to geometry bounds
 */

#pragma once

#include <QMatrix4x4>
#include <QVector3D>
#include <QtCore/QObject>

namespace OpenGeoLab {
namespace Rendering {

/**
 * @brief Camera class for 3D scene navigation
 *
 * Implements an orbit-style camera that rotates around a target point.
 * Supports smooth zooming, panning, and automatic view fitting.
 */
class Camera : public QObject {
    Q_OBJECT

public:
    explicit Camera(QObject* parent = nullptr);
    ~Camera() override = default;

    // ========================================================================
    // View Matrix Calculation
    // ========================================================================

    /**
     * @brief Get the view matrix for rendering
     * @return View transformation matrix
     */
    QMatrix4x4 viewMatrix() const;

    /**
     * @brief Get the projection matrix
     * @param aspect_ratio Viewport aspect ratio (width / height)
     * @return Projection matrix
     */
    QMatrix4x4 projectionMatrix(float aspect_ratio) const;

    /**
     * @brief Get combined view-projection matrix
     * @param aspect_ratio Viewport aspect ratio
     * @return Combined VP matrix
     */
    QMatrix4x4 viewProjectionMatrix(float aspect_ratio) const;

    // ========================================================================
    // Camera Position and Orientation
    // ========================================================================

    /**
     * @brief Get camera position in world space
     * @return Camera position
     */
    QVector3D position() const;

    /**
     * @brief Get camera target point
     * @return Target position the camera looks at
     */
    QVector3D target() const { return m_target; }

    /**
     * @brief Set camera target point
     * @param target New target position
     */
    void setTarget(const QVector3D& target);

    /**
     * @brief Get camera up vector
     * @return Up direction vector
     */
    QVector3D upVector() const { return m_upVector; }

    // ========================================================================
    // Orbit Controls
    // ========================================================================

    /**
     * @brief Rotate camera around target (orbit)
     * @param delta_yaw Horizontal rotation in degrees
     * @param delta_pitch Vertical rotation in degrees
     */
    void orbit(float delta_yaw, float delta_pitch);

    /**
     * @brief Get current yaw angle (horizontal rotation)
     * @return Yaw angle in degrees
     */
    float yaw() const { return m_yaw; }

    /**
     * @brief Get current pitch angle (vertical rotation)
     * @return Pitch angle in degrees
     */
    float pitch() const { return m_pitch; }

    /**
     * @brief Set orbit angles directly
     * @param yaw Horizontal angle in degrees
     * @param pitch Vertical angle in degrees
     */
    void setOrbitAngles(float yaw, float pitch);

    // ========================================================================
    // Zoom Controls
    // ========================================================================

    /**
     * @brief Zoom camera by factor
     * @param factor Zoom multiplier (> 1 zooms in, < 1 zooms out)
     */
    void zoom(float factor);

    /**
     * @brief Set camera distance from target
     * @param distance Distance value
     */
    void setDistance(float distance);

    /**
     * @brief Get current camera distance from target
     * @return Distance value
     */
    float distance() const { return m_distance; }

    // ========================================================================
    // Pan Controls
    // ========================================================================

    /**
     * @brief Pan camera in view space (legacy method)
     * @param delta_x Horizontal pan amount
     * @param delta_y Vertical pan amount
     */
    void pan(float delta_x, float delta_y);

    /**
     * @brief Pan camera using pixel coordinates (improved method)
     * @param prev_x Previous mouse x in pixels
     * @param prev_y Previous mouse y in pixels
     * @param curr_x Current mouse x in pixels
     * @param curr_y Current mouse y in pixels
     *
     * This method calculates pan amount based on camera FOV and distance,
     * providing consistent feel regardless of zoom level.
     */
    void panByPixel(int prev_x, int prev_y, int curr_x, int curr_y);

    /**
     * @brief Set viewport size for pan calculations
     * @param width Viewport width in pixels
     * @param height Viewport height in pixels
     */
    void setViewportSize(int width, int height);

    // ========================================================================
    // View Fitting
    // ========================================================================

    /**
     * @brief Fit view to show bounding box
     * @param min_point Minimum corner of bounding box
     * @param max_point Maximum corner of bounding box
     * @param margin Additional margin factor (1.0 = no margin)
     */
    void fitToBounds(const QVector3D& min_point, const QVector3D& max_point, float margin = 1.2f);

    /**
     * @brief Reset camera to default view
     */
    void reset();

    // ========================================================================
    // Projection Settings
    // ========================================================================

    /**
     * @brief Set field of view angle
     * @param fov Field of view in degrees
     */
    void setFieldOfView(float fov);

    /**
     * @brief Get field of view angle
     * @return FOV in degrees
     */
    float fieldOfView() const { return m_fov; }

    /**
     * @brief Set near clipping plane distance
     * @param near_plane Near plane distance
     */
    void setNearPlane(float near_plane);

    /**
     * @brief Set far clipping plane distance
     * @param far_plane Far plane distance
     */
    void setFarPlane(float far_plane);

signals:
    /**
     * @brief Emitted when camera parameters change
     */
    void cameraChanged();

private:
    /**
     * @brief Update internal state after parameter changes
     */
    void updateCameraVectors();

    /**
     * @brief Update near/far clipping planes based on current distance
     */
    void updateClippingPlanes();

    // Orbit parameters
    float m_yaw = 0.0f;      // Horizontal rotation (degrees)
    float m_pitch = 30.0f;   // Vertical rotation (degrees), default slightly above
    float m_distance = 5.0f; // Distance from target

    // Target and orientation
    QVector3D m_target = QVector3D(0, 0, 0);
    QVector3D m_upVector = QVector3D(0, 1, 0);

    // Projection parameters
    float m_fov = 45.0f;
    float m_nearPlane = 0.01f;
    float m_farPlane = 10000.0f;

    // Viewport size for pan calculations
    int m_viewportWidth = 800;
    int m_viewportHeight = 600;

    // Constraints
    static constexpr float MIN_PITCH = -89.0f;
    static constexpr float MAX_PITCH = 89.0f;
    static constexpr float MIN_DISTANCE = 0.001f;
    static constexpr float MAX_DISTANCE = 100000.0f;
};

} // namespace Rendering
} // namespace OpenGeoLab
