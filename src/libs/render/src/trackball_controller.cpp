/**
 * @file trackball_controller.cpp
 * @brief Trackball camera controller — adapted from OGL project reference.
 *
 * Core algorithm (computePointOnSphere → quaternion orbit → freezeFromCamera)
 * is preserved; Qt types have been replaced with GLM equivalents.
 */

#include <opengeolab/render/trackball_controller.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>

namespace OpenGeoLab::Render {

namespace {
constexpr glm::vec3 kAxisX{1.f, 0.f, 0.f};
constexpr glm::vec3 kAxisY{0.f, 1.f, 0.f};
constexpr glm::vec3 kAxisZ{0.f, 0.f, 1.f};

[[nodiscard]] glm::quat quatFromBasis(const glm::vec3& x, const glm::vec3& y, const glm::vec3& z) {
    // Build rotation matrix with columns (x, y, z) → extract quaternion.
    glm::mat3 m;
    m[0] = x; // column 0
    m[1] = y; // column 1
    m[2] = z; // column 2
    return glm::normalize(glm::quat_cast(m));
}
} // namespace

TrackballController::TrackballController() = default;

void TrackballController::setViewportSize(float width, float height) {
    if(width > 1.f && height > 1.f) {
        m_viewportWidth = width;
        m_viewportHeight = height;
    }
}

void TrackballController::setSpeed(float speed) { m_speed = speed; }

void TrackballController::syncFromCamera(const Camera& camera) { freezeFromCamera(camera); }

bool TrackballController::isActive() const { return m_dragging; }

TrackballController::Mode TrackballController::mode() const { return m_mode; }

void TrackballController::begin(float x, float y, Mode mode, const Camera& camera) {
    m_mode = mode;
    m_dragging = (mode != Mode::None);

    m_prevPos = {x, y};
    m_clickPos = m_prevPos;

    freezeFromCamera(camera);

    if(m_dragging && m_mode == Mode::Orbit) {
        computePointOnSphere(m_clickPos, m_startVec);
    }
}

void TrackballController::update(float x, float y, Camera& camera) {
    if(!m_dragging || m_mode == Mode::None) {
        return;
    }

    m_clickPos = {x, y};
    const glm::vec2 delta = m_clickPos - m_prevPos;
    if(std::abs(delta.x) < 1e-6f && std::abs(delta.y) < 1e-6f) {
        return;
    }

    switch(m_mode) {
    case Mode::Orbit: {
        computePointOnSphere(m_clickPos, m_stopVec);
        m_rotation = rotationBetweenVectors(m_startVec, m_stopVec);
        // Invert so scene follows cursor direction.
        m_rotation = glm::conjugate(m_rotation);
        applyOrbit(camera);
        m_startVec = m_stopVec;
        break;
    }
    case Mode::Pan:
        applyPan(delta, camera);
        break;
    case Mode::Zoom:
        applyZoomFromDelta(delta, camera);
        break;
    default:
        break;
    }

    m_prevPos = m_clickPos;
}

void TrackballController::end() {
    m_dragging = false;
    m_mode = Mode::None;
    m_zoomSum = 0.f;
}

void TrackballController::wheelZoom(float steps, Camera& camera) {
    if(std::abs(steps) < 1e-6f) {
        return;
    }
    addScrollImpulse(steps);
    updateCameraEyeUp(true, false, camera);
}

// ── Private helpers ─────────────────────────────────────────────────────────

glm::vec3 TrackballController::normalizedOrZero(const glm::vec3& v) {
    float len2 = glm::dot(v, v);
    if(len2 <= 1e-12f) {
        return glm::vec3{0.f};
    }
    return v / std::sqrt(len2);
}

float TrackballController::clampMin(float v, float min_v) { return (v < min_v) ? min_v : v; }

void TrackballController::computePointOnSphere(const glm::vec2& p, glm::vec3& out) const {
    float nx = (2.f * p.x - m_viewportWidth) / m_viewportWidth;
    float ny = (m_viewportHeight - 2.f * p.y) / m_viewportHeight;

    float r2 = nx * nx + ny * ny;
    float nz = (r2 <= 0.5f) ? std::sqrt(1.f - r2) : (0.5f / std::sqrt(r2));

    float inv_norm = 1.f / std::sqrt(r2 + nz * nz);
    out = {nx * inv_norm, ny * inv_norm, nz * inv_norm};
}

glm::quat TrackballController::rotationBetweenVectors(const glm::vec3& u,
                                                      const glm::vec3& v) const {
    float cos_theta = glm::dot(u, v);
    constexpr float eps = 1e-5f;

    if(cos_theta < -1.f + eps) {
        glm::vec3 axis = glm::cross(kAxisZ, u);
        if(glm::dot(axis, axis) < 1e-2f) {
            axis = glm::cross(kAxisX, u);
        }
        axis = normalizedOrZero(axis);
        return glm::angleAxis(glm::pi<float>(), axis);
    }

    float theta = std::acos(std::clamp(cos_theta, -1.f, 1.f));
    glm::vec3 axis = normalizedOrZero(glm::cross(u, v));

    float angle = theta * m_speed * m_orbitScale;
    return glm::angleAxis(angle, axis);
}

glm::vec3 TrackballController::computeCameraEye(const Camera& camera) {
    glm::vec3 orientation = m_rotationSum * kAxisZ;

    if(std::abs(m_zoomSum) > 1e-6f) {
        float factor = std::pow(m_zoomBase, m_zoomSum);
        m_translateLength *= factor;
        m_translateLength = clampMin(m_translateLength, 0.1f);
        m_zoomSum = 0.f;
    }

    return camera.target() + (orientation * m_translateLength);
}

glm::vec3 TrackballController::computeCameraUp() {
    return normalizedOrZero(m_rotationSum * kAxisY);
}

glm::vec3 TrackballController::computePan(const Camera& camera, const glm::vec2& delta) const {
    glm::vec3 look = camera.position() - camera.target();
    float distance = glm::length(look);

    glm::vec3 right = normalizedOrZero(m_rotationSum * kAxisX);
    glm::vec3 up = normalizedOrZero(camera.up());

    return (up * delta.y + right * -delta.x) * (m_panScale * m_speed * distance);
}

void TrackballController::updateCameraEyeUp(bool update_eye, bool update_up, Camera& camera) {
    if(update_eye) {
        camera.setPosition(computeCameraEye(camera));
    }
    if(update_up) {
        glm::vec3 up = computeCameraUp();
        if(glm::dot(up, up) > 1e-10f) {
            camera.setUp(up);
        }
    }

    float dist = glm::length(camera.position() - camera.target());
    camera.updateClipping(dist);
}

void TrackballController::applyOrbit(Camera& camera) {
    m_rotationSum = m_rotationSum * m_rotation;
    updateCameraEyeUp(true, true, camera);
}

void TrackballController::applyPan(const glm::vec2& delta, Camera& camera) {
    glm::vec3 pan = computePan(camera, delta);
    camera.setTarget(camera.target() + pan);
    camera.setPosition(camera.position() + pan);

    freezeFromCamera(camera);
    float dist = glm::length(camera.position() - camera.target());
    camera.updateClipping(dist);
}

void TrackballController::applyZoomFromDelta(const glm::vec2& delta, Camera& camera) {
    float ax = std::abs(delta.x);
    float ay = std::abs(delta.y);

    float dominant = (ay >= ax) ? -delta.y : -delta.x;
    float steps = dominant / m_zoomPixelsPerStep;
    addScrollImpulse(steps);

    updateCameraEyeUp(true, false, camera);
}

void TrackballController::addScrollImpulse(float steps) {
    m_zoomSum += steps * m_speed * m_zoomSpeed;
}

void TrackballController::freezeFromCamera(const Camera& camera) {
    glm::vec3 z = normalizedOrZero(camera.position() - camera.target());
    if(glm::dot(z, z) <= 1e-10f) {
        m_rotationSum = glm::quat{1.f, 0.f, 0.f, 0.f};
        m_translateLength = 0.1f;
        return;
    }

    glm::vec3 x = glm::cross(camera.up(), z);
    if(glm::dot(x, x) <= 1e-8f) {
        glm::vec3 fallback = (std::abs(z.y) < 0.999f) ? kAxisY : kAxisX;
        x = glm::cross(fallback, z);
    }
    x = normalizedOrZero(x);

    glm::vec3 y = normalizedOrZero(glm::cross(z, x));

    m_rotationSum = quatFromBasis(x, y, z);
    m_translateLength = glm::length(camera.position() - camera.target());
}

} // namespace OpenGeoLab::Render
