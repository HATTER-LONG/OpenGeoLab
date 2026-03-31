/**
 * @file camera_state.cpp
 * @brief CameraState implementation — view/projection matrix computation and camera framing
 */

#include <opengeolab/app/camera_state.hpp>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <algorithm>

namespace OpenGeoLab::App {

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
    const float half_range = camera_distance * 10.0F;
    nearPlane = -half_range;
    farPlane = half_range;
}

void CameraState::reset() {
    position = {0.0F, 0.0F, 50.0F};
    target = {0.0F, 0.0F, 0.0F};
    up = {0.0F, 1.0F, 0.0F};
    updateClipping();
}

void CameraState::fitToBoundingBox(const Scene::BoundingBox3D& bounds) {
    if(!bounds.isValid()) {
        reset();
        return;
    }

    target = bounds.center();
    const float diagonal = bounds.diagonal();
    const float fit_distance = diagonal * 1.5F;
    position = target + glm::vec3{0.0F, 0.0F, fit_distance};
    updateClipping();
}

} // namespace OpenGeoLab::App
