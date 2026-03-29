/**
 * @file camera.hpp
 * @brief Eye-target-up camera with projection and view matrices.
 */

#pragma once

#include <opengeolab/render/render_export.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace OpenGeoLab::Render {

/// Eye-target-up orthographic camera.
///
/// Provides view and orthographic projection matrices for rendering.
/// Ortho bounds scale with eye-target distance: closer = zoomed in.
/// Used together with TrackballController for interactive camera control.
class OPENGEOLAB_RENDER_EXPORT Camera {
public:
    /// @return View matrix (glm::lookAt).
    [[nodiscard]] glm::mat4 viewMatrix() const;

    /// @return Orthographic projection matrix.
    /// @param aspect_ratio Viewport width / height.
    [[nodiscard]] glm::mat4 projectionMatrix(float aspect_ratio) const;

    [[nodiscard]] const glm::vec3& position() const;
    [[nodiscard]] const glm::vec3& target() const;
    [[nodiscard]] const glm::vec3& up() const;
    [[nodiscard]] float fov() const;
    [[nodiscard]] float nearPlane() const;
    [[nodiscard]] float farPlane() const;

    void setPosition(const glm::vec3& pos);
    void setTarget(const glm::vec3& tgt);
    void setUp(const glm::vec3& u);
    void setFov(float degrees);

    /// Auto-adjust near/far planes based on eye-target distance.
    void updateClipping(float distance);

    /// Position the camera so the given bounding sphere fills the viewport.
    void fitToBoundingBox(const glm::vec3& center, float radius);

    /// Reset to default state.
    void reset();

    // ── View presets ────────────────────────────────────────────────
    void setFrontView();  ///< +Z looking at target
    void setBackView();   ///< -Z looking at target
    void setLeftView();   ///< -X looking at target
    void setRightView();  ///< +X looking at target
    void setTopView();    ///< +Y looking down
    void setBottomView(); ///< -Y looking up

private:
    glm::vec3 m_position{0.f, 0.f, 50.f};
    glm::vec3 m_target{0.f, 0.f, 0.f};
    glm::vec3 m_up{0.f, 1.f, 0.f};
    float m_fov{45.f};
    float m_nearPlane{0.1f};
    float m_farPlane{10000.f};
};

} // namespace OpenGeoLab::Render
