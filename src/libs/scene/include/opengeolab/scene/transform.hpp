/**
 * @file transform.hpp
 * @brief Affine transform component for scene nodes (TRS decomposition).
 */

#pragma once

#include <opengeolab/scene/scene_export.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace OpenGeoLab::Scene {

/// Affine transform stored as Translation–Rotation–Scale (TRS).
class OPENGEOLAB_SCENE_EXPORT Transform {
public:
    /// Build a 4×4 model matrix from the current TRS state.
    [[nodiscard]] glm::mat4 matrix() const;

    [[nodiscard]] const glm::vec3& position() const;
    [[nodiscard]] const glm::quat& rotation() const;
    [[nodiscard]] const glm::vec3& scale() const;

    void setPosition(const glm::vec3& pos);
    void setRotation(const glm::quat& rot);
    void setScale(const glm::vec3& s);

    /// Reset to identity (origin, no rotation, unit scale).
    void reset();

private:
    glm::vec3 m_position{0.f, 0.f, 0.f};
    glm::quat m_rotation{1.f, 0.f, 0.f, 0.f};
    glm::vec3 m_scale{1.f, 1.f, 1.f};
};

} // namespace OpenGeoLab::Scene
