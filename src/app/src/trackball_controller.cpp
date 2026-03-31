/**
 * @file trackball_controller.cpp
 * @brief TrackballController implementation — orbit, pan, zoom and view preset logic
 */

#include <opengeolab/app/trackball_controller.hpp>

#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>

namespace OpenGeoLab::App {

void TrackballController::setViewportSize(float width, float height) {
    m_viewportWidth = std::max(width, 1.0F);
    m_viewportHeight = std::max(height, 1.0F);
}

void TrackballController::begin(float x, float y, Mode mode, const CameraState& state) {
    m_mode = mode;
    m_lastX = x;
    m_lastY = y;
    m_startDistance = state.distance();
}

void TrackballController::update(float x, float y, CameraState& state) {
    switch(m_mode) {
    case Mode::Orbit:
        applyOrbit(x, y, state);
        break;
    case Mode::Pan:
        applyPan(x, y, state);
        break;
    case Mode::Zoom:
        applyZoom(x, y, state);
        break;
    case Mode::None:
        break;
    }
    m_lastX = x;
    m_lastY = y;
}

void TrackballController::end() { m_mode = Mode::None; }

glm::vec3 TrackballController::projectToSphere(float x, float y) const {
    const float normalized_x = (2.0F * x / m_viewportWidth - 1.0F);
    const float normalized_y = (1.0F - 2.0F * y / m_viewportHeight);

    const float r2 = normalized_x * normalized_x + normalized_y * normalized_y;

    float z = 0.0F;
    if(r2 <= 0.5F) {
        z = std::sqrt(1.0F - r2);
    } else {
        z = 0.5F / std::sqrt(r2);
    }

    const float inv_norm = 1.0F / std::sqrt(r2 + z * z);
    return {normalized_x * inv_norm, normalized_y * inv_norm, z * inv_norm};
}

void TrackballController::applyOrbit(float x, float y, CameraState& state) {
    const glm::vec3 from = projectToSphere(m_lastX, m_lastY);
    const glm::vec3 to = projectToSphere(x, y);

    glm::vec3 axis = glm::cross(from, to);
    const float axis_length = glm::length(axis);
    if(axis_length < 1.0e-6F) {
        return;
    }

    axis /= axis_length;
    // Negate so the scene follows the cursor (camera orbits opposite to drag).
    const float angle = -ORBIT_SCALE * std::asin(std::clamp(axis_length, -1.0F, 1.0F));

    const glm::vec3 view_direction = glm::normalize(state.position - state.target);
    const glm::vec3 right = glm::normalize(glm::cross(state.up, view_direction));
    const glm::vec3 true_up = glm::cross(view_direction, right);
    const glm::mat3 view_to_world(right, true_up, view_direction);
    const glm::vec3 world_axis = view_to_world * axis;
    const glm::quat world_rotation = glm::angleAxis(angle, world_axis);

    const float camera_distance = state.distance();
    glm::vec3 offset = state.position - state.target;
    offset = world_rotation * offset;
    state.position = state.target + glm::normalize(offset) * camera_distance;
    state.up = world_rotation * state.up;
    state.updateClipping();
}

void TrackballController::applyPan(float x, float y, CameraState& state) {
    const float delta_x = x - m_lastX;
    const float delta_y = y - m_lastY;

    const glm::vec3 view_direction = glm::normalize(state.target - state.position);
    const glm::vec3 right = glm::normalize(glm::cross(view_direction, state.up));
    const glm::vec3 up = glm::cross(right, view_direction);

    const float scale = state.distance() * PAN_SCALE;
    const glm::vec3 pan = (-delta_x * right + delta_y * up) * scale;
    state.position += pan;
    state.target += pan;
}

void TrackballController::applyZoom(float /*x*/, float y, CameraState& state) {

    const float delta_y = y - m_lastY;
    const float steps = -delta_y / ZOOM_PIXELS_PER_STEP;
    const float factor = std::pow(ZOOM_BASE, steps * ZOOM_SPEED);

    float distance = state.distance() * factor;
    distance = std::max(distance, MIN_DISTANCE);

    const glm::vec3 direction = glm::normalize(state.position - state.target);
    state.position = state.target + direction * distance;
    state.updateClipping();
}

void TrackballController::wheelZoom(float steps, CameraState& state) {
    const float factor = std::pow(ZOOM_BASE, steps * ZOOM_SPEED);
    float distance = state.distance() * factor;
    distance = std::max(distance, MIN_DISTANCE);

    const glm::vec3 direction = glm::normalize(state.position - state.target);
    state.position = state.target + direction * distance;
    state.updateClipping();
}

void TrackballController::fitToScene(const Scene::BoundingBox3D& bounds, CameraState& state) {
    state.fitToBoundingBox(bounds);
}

void TrackballController::setViewPreset(ViewPreset preset, CameraState& state) {
    const float distance = state.distance();
    glm::vec3 direction;
    glm::vec3 up_direction{0.0F, 1.0F, 0.0F};

    switch(preset) {
    case ViewPreset::Front:
        direction = {0.0F, 0.0F, 1.0F};
        break;
    case ViewPreset::Back:
        direction = {0.0F, 0.0F, -1.0F};
        break;
    case ViewPreset::Top:
        direction = {0.0F, 1.0F, 0.0F};
        up_direction = {0.0F, 0.0F, -1.0F};
        break;
    case ViewPreset::Bottom:
        direction = {0.0F, -1.0F, 0.0F};
        up_direction = {0.0F, 0.0F, 1.0F};
        break;
    case ViewPreset::Left:
        direction = {-1.0F, 0.0F, 0.0F};
        break;
    case ViewPreset::Right:
        direction = {1.0F, 0.0F, 0.0F};
        break;
    case ViewPreset::Isometric:
        direction = glm::normalize(glm::vec3{1.0F, 1.0F, 1.0F});
        break;
    }

    state.position = state.target + direction * distance;
    state.up = up_direction;
    state.updateClipping();
}

} // namespace OpenGeoLab::App
