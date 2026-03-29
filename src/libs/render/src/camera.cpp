#include <opengeolab/render/camera.hpp>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>

namespace OpenGeoLab::Render {

glm::mat4 Camera::viewMatrix() const { return glm::lookAt(m_position, m_target, m_up); }

glm::mat4 Camera::projectionMatrix(float aspect_ratio) const {
    // Orthographic: visible half-height proportional to camera distance
    float distance = glm::length(m_position - m_target);
    float half_height = distance * 0.5f;
    float half_width = half_height * aspect_ratio;
    return glm::ortho(-half_width, half_width, -half_height, half_height, m_nearPlane, m_farPlane);
}

const glm::vec3& Camera::position() const { return m_position; }
const glm::vec3& Camera::target() const { return m_target; }
const glm::vec3& Camera::up() const { return m_up; }
float Camera::fov() const { return m_fov; }
float Camera::nearPlane() const { return m_nearPlane; }
float Camera::farPlane() const { return m_farPlane; }

void Camera::setPosition(const glm::vec3& pos) { m_position = pos; }
void Camera::setTarget(const glm::vec3& tgt) { m_target = tgt; }
void Camera::setUp(const glm::vec3& u) { m_up = u; }
void Camera::setFov(float degrees) { m_fov = degrees; }

void Camera::updateClipping(float distance) {
    // Orthographic: symmetric depth range centered at camera so geometry
    // behind the camera is not clipped. Linear depth in ortho preserves
    // precision even with wide range.
    float d = std::max(distance, 1e-4f);
    float half_range = d * 10.f;
    m_nearPlane = -half_range;
    m_farPlane = half_range;
}

void Camera::fitToBoundingBox(const glm::vec3& center, float radius) {
    if(radius <= 0.f) {
        return;
    }

    // Orthographic: place camera at 2× radius distance so the sphere
    // occupies roughly half the viewport height (half_height = dist * 0.5).
    float dist = radius * 2.f;

    // Place camera along current viewing direction (or default -Z if degenerate)
    glm::vec3 dir = m_position - m_target;
    float len = glm::length(dir);
    if(len < 1e-6f) {
        dir = glm::vec3{0.f, 0.f, 1.f};
    } else {
        dir /= len;
    }

    m_target = center;
    m_position = center + dir * dist;
    updateClipping(dist);
}

void Camera::reset() {
    m_position = {0.f, 0.f, 50.f};
    m_target = {0.f, 0.f, 0.f};
    m_up = {0.f, 1.f, 0.f};
    m_fov = 45.f;
    m_nearPlane = 0.1f;
    m_farPlane = 10000.f;
}

// ── View presets ────────────────────────────────────────────────────────────

void Camera::setFrontView() {
    float dist = glm::length(m_position - m_target);
    m_position = m_target + glm::vec3(0.f, 0.f, dist);
    m_up = {0.f, 1.f, 0.f};
}

void Camera::setBackView() {
    float dist = glm::length(m_position - m_target);
    m_position = m_target + glm::vec3(0.f, 0.f, -dist);
    m_up = {0.f, 1.f, 0.f};
}

void Camera::setLeftView() {
    float dist = glm::length(m_position - m_target);
    m_position = m_target + glm::vec3(-dist, 0.f, 0.f);
    m_up = {0.f, 1.f, 0.f};
}

void Camera::setRightView() {
    float dist = glm::length(m_position - m_target);
    m_position = m_target + glm::vec3(dist, 0.f, 0.f);
    m_up = {0.f, 1.f, 0.f};
}

void Camera::setTopView() {
    float dist = glm::length(m_position - m_target);
    m_position = m_target + glm::vec3(0.f, dist, 0.f);
    m_up = {0.f, 0.f, -1.f};
}

void Camera::setBottomView() {
    float dist = glm::length(m_position - m_target);
    m_position = m_target + glm::vec3(0.f, -dist, 0.f);
    m_up = {0.f, 0.f, 1.f};
}

} // namespace OpenGeoLab::Render
