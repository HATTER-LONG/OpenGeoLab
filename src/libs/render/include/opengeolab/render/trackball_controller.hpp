/**
 * @file trackball_controller.hpp
 * @brief Trackball-style camera controller for viewport interaction.
 *
 * Adapted from the OGL project reference implementation; all Qt types
 * have been migrated to GLM equivalents.
 */

#pragma once

#include <opengeolab/render/camera.hpp>
#include <opengeolab/render/render_export.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace OpenGeoLab::Render {

/// Trackball camera controller providing orbit / pan / zoom.
///
/// Usage: call begin() on mouse-press, update() on mouse-move, end() on
/// mouse-release.  For scroll-wheel, call wheelZoom() directly.
/// Before first use, call syncFromCamera() or begin() to initialize
/// internal orientation from the current Camera state.
class OPENGEOLAB_RENDER_EXPORT TrackballController {
public:
    enum class Mode { None, Orbit, Pan, Zoom };

    TrackballController();

    void setViewportSize(float width, float height);
    void setSpeed(float speed);

    /// Synchronize internal quaternion from current Camera state.
    void syncFromCamera(const Camera& camera);

    [[nodiscard]] bool isActive() const;
    [[nodiscard]] Mode mode() const;

    /// Begin a drag interaction.
    /// @param x, y Cursor position in item coordinates.
    void begin(float x, float y, Mode mode, const Camera& camera);

    /// Update camera during an active drag.
    void update(float x, float y, Camera& camera);

    /// End the active drag interaction.
    void end();

    /// Scroll-wheel zoom.
    /// @param steps Positive = zoom in.
    void wheelZoom(float steps, Camera& camera);

private:
    static glm::vec3 normalizedOrZero(const glm::vec3& v);
    static float clampMin(float v, float min_v);

    void computePointOnSphere(const glm::vec2& p, glm::vec3& out) const;
    [[nodiscard]] glm::quat rotationBetweenVectors(const glm::vec3& u, const glm::vec3& v) const;

    void updateCameraEyeUp(bool update_eye, bool update_up, Camera& camera);
    glm::vec3 computeCameraEye(const Camera& camera);
    glm::vec3 computeCameraUp();
    glm::vec3 computePan(const Camera& camera, const glm::vec2& delta) const;

    void applyOrbit(Camera& camera);
    void applyPan(const glm::vec2& delta, Camera& camera);
    void applyZoomFromDelta(const glm::vec2& delta, Camera& camera);

    void addScrollImpulse(float steps);
    void freezeFromCamera(const Camera& camera);

    float m_viewportWidth{1.f};
    float m_viewportHeight{1.f};
    float m_speed{1.f};

    Mode m_mode{Mode::None};
    bool m_dragging{false};

    glm::vec2 m_clickPos{0.f};
    glm::vec2 m_prevPos{0.f};

    glm::vec3 m_startVec{0.f, 0.f, 1.f};
    glm::vec3 m_stopVec{0.f, 0.f, 1.f};

    glm::quat m_rotation{1.f, 0.f, 0.f, 0.f};
    glm::quat m_rotationSum{1.f, 0.f, 0.f, 0.f};

    float m_translateLength{50.f};

    float m_orbitScale{2.2f};
    float m_panScale{0.0015f};
    float m_zoomSpeed{1.5f};
    float m_zoomBase{0.90f};
    float m_zoomPixelsPerStep{60.f};

    float m_zoomSum{0.f};
};

} // namespace OpenGeoLab::Render
