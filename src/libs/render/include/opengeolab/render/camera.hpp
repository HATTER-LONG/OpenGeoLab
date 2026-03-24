/**
 * @file camera.hpp
 * @brief Declares the orbit camera with perspective/orthographic support.
 */
#pragma once

#include <opengeolab/render/render_export.hpp>
#include <opengeolab/scene/bounding_box.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace OpenGeoLab::Render {

/**
 * @brief Complete camera state snapshot for recording/replay.
 */
struct CameraState {
    glm::vec3 eye{0.0F, 5.0F, 10.0F};
    glm::vec3 target{0.0F};
    glm::vec3 up{0.0F, 1.0F, 0.0F};
    float fovDeg = 45.0F;
    float aspect = 1.0F;
    float nearPlane = 0.1F;
    float farPlane = 1000.0F;
    bool orthographic = false;
    float orthoWidth = 10.0F;
};

/**
 * @brief World-space ray for picking.
 */
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

/**
 * @brief Orbit camera supporting perspective/orthographic, orbit, pan, zoom.
 *
 * Uses spherical coordinates (theta, phi, distance) around target.
 * All matrix computations use GLM — no OpenGL dependency.
 */
class OPENGEOLAB_RENDER_EXPORT Camera {
public:
    Camera();

    void setPerspective(float fovDeg, float aspect, float nearPlane, float farPlane);
    void setOrthographic(float width, float aspect, float nearPlane, float farPlane);

    /** @brief Orbit: deltaTheta horizontal (rad), deltaPhi vertical (rad). */
    void orbit(float deltaTheta, float deltaPhi);

    /** @brief Pan: dx/dy screen-space offset, converted to world space internally. */
    void pan(float dx, float dy);

    /** @brief Zoom: factor > 1 zooms in, < 1 zooms out. */
    void zoom(float factor);

    /** @brief Fit view to bounding box. */
    void fitAll(const Scene::BoundingBox& bbox);

    void setAspect(float aspect);

    [[nodiscard]] glm::mat4 viewMatrix() const;
    [[nodiscard]] glm::mat4 projectionMatrix() const;
    [[nodiscard]] glm::vec3 position() const;

    [[nodiscard]] CameraState captureState() const;
    void restoreState(const CameraState& state);

    /** @brief Screen coords to world ray (for picking). */
    [[nodiscard]] Ray
    screenToWorldRay(float screenX, float screenY, int viewportWidth, int viewportHeight) const;

private:
    CameraState state_;
    float aspect_ = 1.0F;

    void updateFromSpherical();

    float theta_ = 0.0F; /**< Horizontal angle (radians). */
    float phi_ = 0.7F;   /**< Vertical angle (radians), range (epsilon, pi-epsilon). */
    float distance_ = 12.0F;
};

} // namespace OpenGeoLab::Render
