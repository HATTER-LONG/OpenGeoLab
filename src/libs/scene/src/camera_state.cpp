/**
 * @file camera_state.cpp
 * @brief CameraState implementation — view/projection matrix computation and camera framing
 */

#include <opengeolab/scene/camera_state.hpp>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <algorithm>

namespace OpenGeoLab::Scene {

glm::mat4 CameraState::viewMatrix() const { return glm::lookAt(position, target, up); }

float CameraState::distance() const { return glm::length(position - target); }

glm::mat4 CameraState::projMatrix(float aspect) const {
    const float camera_distance = distance();
    const float half_height = camera_distance * 0.5F;
    const float half_width = half_height * aspect;
    return glm::ortho(-half_width, half_width, -half_height, half_height, nearPlane, farPlane);
}

void CameraState::updateClipping() {
    const float camera_distance = std::max(distance(), 1.0e-4F);
    // Use the larger of camera-based range and scene-based range so the
    // clipping volume never collapses when zooming into a large model.
    const float half_range = std::max(camera_distance * 10.0F, sceneExtent * 2.0F);
    nearPlane = -half_range;
    farPlane = half_range;
}

void CameraState::reset() {
    position = {0.0F, 0.0F, 50.0F};
    target = {0.0F, 0.0F, 0.0F};
    up = {0.0F, 1.0F, 0.0F};
    sceneExtent = 100.0F;
    updateClipping();
}

void CameraState::fitToBoundingBox(const BoundingBox3D& bounds) {
    if(!bounds.isValid()) {
        reset();
        return;
    }

    // Preserve current view direction; only adjust target and distance.
    const float current_dist = distance();
    glm::vec3 view_dir = position - target;
    if(current_dist > 1.0e-6F) {
        view_dir /= current_dist;
    } else {
        view_dir = glm::vec3{0.0F, 0.0F, 1.0F};
    }

    target = bounds.center();
    const float diagonal = bounds.diagonal();
    sceneExtent = diagonal;
    const float fit_distance = diagonal * 1.1F;
    position = target + view_dir * fit_distance;
    updateClipping();
}

} // namespace OpenGeoLab::Scene
