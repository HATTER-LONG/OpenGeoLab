/**
 * @file camera_state.hpp
 * @brief Value object representing the complete state of a 3D camera.
 */

#pragma once

#include <opengeolab/render/render_export.hpp>

#include <nlohmann/json.hpp>

#include <array>

namespace OpenGeoLab::Render {

/**
 * @brief Immutable snapshot of camera parameters, supporting both perspective
 *        and orthographic projections.
 *
 * Used for JSON serialization in the recording/replay protocol and for
 * synchronizing camera state between SceneManager and QML.
 */
struct OPENGEOLAB_RENDER_EXPORT CameraState {
    /// Projection model discriminator.
    enum class ProjectionType { kPerspective, kOrthographic };

    ProjectionType projection = ProjectionType::kPerspective;
    std::array<float, 3> position{0.f, 0.f, 10.f};         ///< Camera world position
    std::array<float, 4> orientation{0.f, 0.f, 0.f, 1.f}; ///< Rotation quaternion (x, y, z, w)
    float near_distance = 0.1f;
    float far_distance = 1000.f;
    float focal_distance = 5.f;
    float height_angle = 0.7854f; ///< Perspective half-angle in radians (~45°)
    float height = 10.f;          ///< Orthographic viewport height

    /// Serialize to a JSON object.
    [[nodiscard]] auto to_json() const -> nlohmann::json;

    /// Deserialize from a JSON object. Missing fields use defaults.
    static auto from_json(const nlohmann::json& j) -> CameraState;
};

} // namespace OpenGeoLab::Render
