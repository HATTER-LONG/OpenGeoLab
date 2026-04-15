/**
 * @file camera_state.hpp
 * @brief Cartesian camera state with orthographic projection
 */

#pragma once

#include <opengeolab/scene/bounding_box3d.hpp>
#include <opengeolab/scene/scene_export.hpp>

#include <glm/glm.hpp>

namespace OpenGeoLab::Scene {

/**
 * @brief Cartesian camera model for orthographic rendering
 *
 * Stores position/target/up vectors and symmetric near/far clipping.
 * Provides view and orthographic projection matrices.
 */
struct OPENGEOLAB_SCENE_EXPORT CameraState {
    glm::vec3 position{0.0F, 0.0F, 50.0F}; /**< Eye position */
    glm::vec3 target{0.0F, 0.0F, 0.0F};    /**< Look-at target */
    glm::vec3 up{0.0F, 1.0F, 0.0F};        /**< Up direction */
    float nearPlane{-500.0F};              /**< Near clipping plane */
    float farPlane{500.0F};                /**< Far clipping plane */
    float sceneExtent{100.0F};             /**< Remembered scene diagonal for clipping floor */

    /** @brief Compute the view matrix via glm::lookAt. */
    [[nodiscard]] glm::mat4 viewMatrix() const;

    /**
     * @brief Compute orthographic projection matrix.
     * @param aspect Viewport width / height.
     */
    [[nodiscard]] glm::mat4 projMatrix(float aspect) const;

    /** @brief Get eye position (alias for position). */
    [[nodiscard]] glm::vec3 eyePosition() const { return position; }

    /** @brief Distance from eye to target. */
    [[nodiscard]] float distance() const;

    /** @brief Recalculate near/far planes, floored by scene extent. */
    void updateClipping();

    /** @brief Reset to default values. */
    void reset();

    /**
     * @brief Position camera to view the entire bounding box.
     * @param bounds Scene bounds to frame.
     */
    void fitToBoundingBox(const BoundingBox3D& bounds);
};

} // namespace OpenGeoLab::Scene
