/**
 * @file viewport_state.cpp
 * @brief ViewportState implementation
 */

#include <opengeolab/scene/viewport_state.hpp>

#include <glm/glm.hpp>

#include <cmath>

namespace OpenGeoLab::Scene {

ViewportState::ViewportState() { m_camera.reset(); }
ViewportState::~ViewportState() = default;

CameraState ViewportState::camera() const {
    const std::lock_guard lock(m_mutex);
    return m_camera;
}

void ViewportState::setCamera(const CameraState& state) {
    {
        const std::lock_guard lock(m_mutex);
        m_camera = state;
        m_cameraVersion.fetch_add(1, std::memory_order_relaxed);
    }
    cameraChanged.emit();
}

uint64_t ViewportState::cameraVersion() const noexcept {
    return m_cameraVersion.load(std::memory_order_relaxed);
}

void ViewportState::fitToBounds(const BoundingBox3D& bounds) {
    {
        const std::lock_guard lock(m_mutex);
        m_camera.fitToBoundingBox(bounds);
        m_cameraVersion.fetch_add(1, std::memory_order_relaxed);
    }
    cameraChanged.emit();
}

void ViewportState::setViewPreset(ViewPreset preset) {
    {
        const std::lock_guard lock(m_mutex);
        const float dist = m_camera.distance();
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

        m_camera.position = m_camera.target + direction * dist;
        m_camera.up = up_direction;
        m_camera.updateClipping();
        m_cameraVersion.fetch_add(1, std::memory_order_relaxed);
    }
    cameraChanged.emit();
}

void ViewportState::requestPickArea(const PendingPickArea& request) {
    {
        const std::lock_guard lock(m_mutex);
        m_pendingPickArea = request;
    }
    pickAreaRequested.emit();
}

std::optional<PendingPickArea> ViewportState::consumePickArea() {
    const std::lock_guard lock(m_mutex);
    auto result = std::move(m_pendingPickArea);
    m_pendingPickArea.reset();
    return result;
}

void ViewportState::requestCapture(PendingCapture request) {
    {
        const std::lock_guard lock(m_mutex);
        m_pendingCapture = std::move(request);
    }
    captureRequested.emit();
}

std::optional<PendingCapture> ViewportState::consumeCapture() {
    const std::lock_guard lock(m_mutex);
    auto result = std::move(m_pendingCapture);
    m_pendingCapture.reset();
    return result;
}

bool ViewportState::xRayMode() const {
    const std::lock_guard lock(m_mutex);
    return m_xRayMode;
}

void ViewportState::setXRayMode(bool enabled) {
    {
        const std::lock_guard lock(m_mutex);
        if(m_xRayMode == enabled) {
            return;
        }
        m_xRayMode = enabled;
    }
    displayModeChanged.emit();
}

bool ViewportState::showTessellation() const {
    const std::lock_guard lock(m_mutex);
    return m_showTessellation;
}

void ViewportState::setShowTessellation(bool enabled) {
    {
        const std::lock_guard lock(m_mutex);
        if(m_showTessellation == enabled) {
            return;
        }
        m_showTessellation = enabled;
    }
    displayModeChanged.emit();
}

} // namespace OpenGeoLab::Scene
