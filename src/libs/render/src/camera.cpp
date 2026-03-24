/**
 * @file camera.cpp
 * @brief Implements the orbit camera.
 */
#include <opengeolab/render/camera.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace OpenGeoLab::Render {

namespace {

constexpr float kMinPhi = 0.01F;
constexpr float kMinDistance = 0.1F;
constexpr float kPanScale = 0.001F;

[[nodiscard]] float safeAspect(float aspect) { return aspect > 0.0F ? aspect : 1.0F; }

[[nodiscard]] glm::vec3 safeNormalize(const glm::vec3& vector, const glm::vec3& fallback) {
    const float length = glm::length(vector);
    if(length <= std::numeric_limits<float>::epsilon()) {
        return fallback;
    }

    return vector / length;
}

} // namespace

Camera::Camera() { updateFromSpherical(); }

void Camera::setPerspective(float fovDeg, float aspect, float nearPlane, float farPlane) {
    state_.fovDeg = fovDeg;
    state_.aspect = safeAspect(aspect);
    state_.nearPlane = nearPlane;
    state_.farPlane = farPlane;
    aspect_ = state_.aspect;
    state_.orthographic = false;
}

void Camera::setOrthographic(float width, float aspect, float nearPlane, float farPlane) {
    state_.orthoWidth = width;
    state_.aspect = safeAspect(aspect);
    state_.nearPlane = nearPlane;
    state_.farPlane = farPlane;
    aspect_ = state_.aspect;
    state_.orthographic = true;
}

void Camera::orbit(float deltaTheta, float deltaPhi) {
    theta_ += deltaTheta;
    phi_ = std::clamp(phi_ + deltaPhi, kMinPhi, std::numbers::pi_v<float> - kMinPhi);
    updateFromSpherical();
}

void Camera::pan(float dx, float dy) {
    const glm::vec3 forward =
        safeNormalize(state_.target - state_.eye, glm::vec3{0.0F, 0.0F, -1.0F});
    const glm::vec3 right =
        safeNormalize(glm::cross(forward, state_.up), glm::vec3{1.0F, 0.0F, 0.0F});
    const glm::vec3 camera_up = safeNormalize(glm::cross(right, forward), state_.up);
    const glm::vec3 translation = (-right * dx + camera_up * dy) * distance_ * kPanScale;

    state_.target += translation;
    state_.eye += translation;
}

void Camera::zoom(float factor) {
    if(factor <= 0.0F) {
        return;
    }

    distance_ = std::max(kMinDistance, distance_ / factor);
    updateFromSpherical();
}

void Camera::fitAll(const Scene::BoundingBox& bbox) {
    if(!bbox.isValid()) {
        return;
    }

    state_.target = bbox.center();

    const glm::vec3 size = bbox.size();
    const glm::vec3 half_size = size * 0.5F;
    const float radius = std::max(glm::length(half_size), kMinDistance);
    const float vertical_half_fov = glm::radians(std::max(state_.fovDeg, 1.0F)) * 0.5F;
    const float horizontal_half_fov = std::atan(std::tan(vertical_half_fov) * safeAspect(aspect_));
    const float limiting_half_fov =
        std::max(std::min(vertical_half_fov, horizontal_half_fov), kMinPhi);
    distance_ = std::max(radius / std::sin(limiting_half_fov), kMinDistance);

    if(state_.orthographic) {
        state_.orthoWidth = std::max({size.x, size.y * safeAspect(aspect_), kMinDistance});
    }

    updateFromSpherical();
}

void Camera::setAspect(float aspect) {
    aspect_ = safeAspect(aspect);
    state_.aspect = aspect_;
}

glm::mat4 Camera::viewMatrix() const { return glm::lookAt(state_.eye, state_.target, state_.up); }

glm::mat4 Camera::projectionMatrix() const {
    if(state_.orthographic) {
        const float half_width = state_.orthoWidth * 0.5F;
        const float half_height = half_width / safeAspect(aspect_);
        return glm::ortho(-half_width, half_width, -half_height, half_height, state_.nearPlane,
                          state_.farPlane);
    }

    return glm::perspective(glm::radians(state_.fovDeg), safeAspect(aspect_), state_.nearPlane,
                            state_.farPlane);
}

glm::vec3 Camera::position() const { return state_.eye; }

CameraState Camera::captureState() const { return state_; }

void Camera::restoreState(const CameraState& state) {
    state_ = state;

    const glm::vec3 offset = state_.eye - state_.target;
    const float offset_length = glm::length(offset);
    if(offset_length <= std::numeric_limits<float>::epsilon()) {
        aspect_ = safeAspect(state_.aspect);
        theta_ = 0.0F;
        phi_ = 0.7F;
        distance_ = 12.0F;
        updateFromSpherical();
        return;
    }

    aspect_ = safeAspect(state_.aspect);
    distance_ = std::max(offset_length, kMinDistance);
    theta_ = std::atan2(offset.x, offset.z);
    phi_ = std::clamp(std::acos(std::clamp(offset.y / distance_, -1.0F, 1.0F)), kMinPhi,
                      std::numbers::pi_v<float> - kMinPhi);
}

Ray Camera::screenToWorldRay(float screenX,
                             float screenY,
                             int viewportWidth,
                             int viewportHeight) const {
    if(viewportWidth <= 0 || viewportHeight <= 0) {
        return Ray{state_.eye,
                   safeNormalize(state_.target - state_.eye, glm::vec3{0.0F, 0.0F, -1.0F})};
    }

    const float ndc_x = (2.0F * screenX) / static_cast<float>(viewportWidth) - 1.0F;
    const float ndc_y = 1.0F - (2.0F * screenY) / static_cast<float>(viewportHeight);
    const glm::mat4 inverse_vp = glm::inverse(projectionMatrix() * viewMatrix());

    const glm::vec4 near_clip{ndc_x, ndc_y, -1.0F, 1.0F};
    const glm::vec4 far_clip{ndc_x, ndc_y, 1.0F, 1.0F};
    const glm::vec4 near_world4 = inverse_vp * near_clip;
    const glm::vec4 far_world4 = inverse_vp * far_clip;

    const glm::vec3 near_world = glm::vec3{near_world4} / near_world4.w;
    const glm::vec3 far_world = glm::vec3{far_world4} / far_world4.w;

    const glm::vec3 origin = state_.orthographic ? near_world : state_.eye;
    return Ray{origin, safeNormalize(far_world - near_world, glm::vec3{0.0F, 0.0F, -1.0F})};
}

void Camera::updateFromSpherical() {
    const float sin_phi = std::sin(phi_);
    const float cos_phi = std::cos(phi_);
    const float sin_theta = std::sin(theta_);
    const float cos_theta = std::cos(theta_);

    const glm::vec3 offset{distance_ * sin_phi * sin_theta, distance_ * cos_phi,
                           distance_ * sin_phi * cos_theta};
    state_.eye = state_.target + offset;
    state_.up = glm::vec3{0.0F, 1.0F, 0.0F};
}

} // namespace OpenGeoLab::Render
