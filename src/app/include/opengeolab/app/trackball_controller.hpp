/**
 * @file trackball_controller.hpp
 * @brief Quaternion-based trackball controller for orbit/pan/zoom
 */

#pragma once

#include <opengeolab/scene/camera_state.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace OpenGeoLab::App {

using OpenGeoLab::Scene::CameraState;

/**
 * @brief Trackball controller for 3D camera manipulation
 *
 * Supports orbit (virtual sphere), exponential zoom, and view-plane pan.
 * All manipulation is applied to a CameraState reference.
 */
class TrackballController {
public:
    enum class Mode { None, Orbit, Pan, Zoom };

    /** @brief Set viewport size for mouse normalization */
    void setViewportSize(float width, float height);

    /** @brief Check if a drag operation is active */
    [[nodiscard]] bool isActive() const { return m_mode != Mode::None; }

    /** @brief Current interaction mode */
    [[nodiscard]] Mode mode() const { return m_mode; }

    /** @brief Start a drag operation */
    void begin(float x, float y, Mode mode, const CameraState& state);

    /** @brief Update during drag — modifies camera state */
    void update(float x, float y, CameraState& state);

    /** @brief End drag operation */
    void end();

    /** @brief Wheel zoom — modifies camera state */
    void wheelZoom(float steps, CameraState& state);

private:
    /** @brief Project screen point to virtual sphere */
    [[nodiscard]] glm::vec3 projectToSphere(float x, float y) const;

    void applyOrbit(float x, float y, CameraState& state);
    void applyPan(float x, float y, CameraState& state);
    void applyZoom(float x, float y, CameraState& state);

    Mode m_mode{Mode::None};
    float m_viewportWidth{1.0F};
    float m_viewportHeight{1.0F};

    float m_lastX{0.0F};
    float m_lastY{0.0F};
    float m_startDistance{1.0F};

    static constexpr float ORBIT_SCALE = 2.2F;
    static constexpr float PAN_SCALE = 0.0015F;
    static constexpr float ZOOM_BASE = 0.90F;
    static constexpr float ZOOM_SPEED = 1.5F;
    static constexpr float ZOOM_PIXELS_PER_STEP = 60.0F;
    static constexpr float MIN_DISTANCE = 0.1F;
};

} // namespace OpenGeoLab::App
