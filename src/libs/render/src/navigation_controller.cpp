#include <opengeolab/render/navigation_controller.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

auto vec_add(const std::array<float, 3>& lhs, const std::array<float, 3>& rhs)
    -> std::array<float, 3> {
    return {lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2]};
}

auto vec_subtract(const std::array<float, 3>& lhs, const std::array<float, 3>& rhs)
    -> std::array<float, 3> {
    return {lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2]};
}

auto vec_scale(const std::array<float, 3>& value, float scale) -> std::array<float, 3> {
    return {value[0] * scale, value[1] * scale, value[2] * scale};
}

auto vec_dot(const std::array<float, 3>& lhs, const std::array<float, 3>& rhs) -> float {
    return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}

auto vec_cross(const std::array<float, 3>& lhs, const std::array<float, 3>& rhs)
    -> std::array<float, 3> {
    return {
        lhs[1] * rhs[2] - lhs[2] * rhs[1],
        lhs[2] * rhs[0] - lhs[0] * rhs[2],
        lhs[0] * rhs[1] - lhs[1] * rhs[0],
    };
}

auto vec_length(const std::array<float, 3>& value) -> float {
    return std::sqrt(vec_dot(value, value));
}

auto vec_normalize(const std::array<float, 3>& value) -> std::array<float, 3> {
    const float length = vec_length(value);
    if(length <= 1e-6f) {
        return {0.f, 0.f, 0.f};
    }

    return vec_scale(value, 1.f / length);
}

auto quat_conjugate(const std::array<float, 4>& q) -> std::array<float, 4> {
    return {-q[0], -q[1], -q[2], q[3]};
}

auto quat_multiply(const std::array<float, 4>& q1, const std::array<float, 4>& q2)
    -> std::array<float, 4> {
    return {
        q1[3] * q2[0] + q1[0] * q2[3] + q1[1] * q2[2] - q1[2] * q2[1],
        q1[3] * q2[1] - q1[0] * q2[2] + q1[1] * q2[3] + q1[2] * q2[0],
        q1[3] * q2[2] + q1[0] * q2[1] - q1[1] * q2[0] + q1[2] * q2[3],
        q1[3] * q2[3] - q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2],
    };
}

auto quat_normalize(const std::array<float, 4>& q) -> std::array<float, 4> {
    const float length = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if(length <= 1e-6f) {
        return {0.f, 0.f, 0.f, 1.f};
    }

    return {q[0] / length, q[1] / length, q[2] / length, q[3] / length};
}

auto quat_rotate_vec(const std::array<float, 4>& q, const std::array<float, 3>& v)
    -> std::array<float, 3> {
    const auto normalized_q = quat_normalize(q);
    const std::array<float, 4> vector_quat = {v[0], v[1], v[2], 0.f};
    const auto rotated = quat_multiply(
        quat_multiply(normalized_q, vector_quat), quat_conjugate(normalized_q));
    return {rotated[0], rotated[1], rotated[2]};
}

auto quat_from_axis_angle(const std::array<float, 3>& axis, float angle) -> std::array<float, 4> {
    const auto normalized_axis = vec_normalize(axis);
    const float half_angle = angle * 0.5f;
    const float sin_half_angle = std::sin(half_angle);

    return quat_normalize({
        normalized_axis[0] * sin_half_angle,
        normalized_axis[1] * sin_half_angle,
        normalized_axis[2] * sin_half_angle,
        std::cos(half_angle),
    });
}

} // namespace

namespace OpenGeoLab::Render {

auto NavigationController::compute_orbit(
    const std::array<float, 4>& current_orientation,
    const std::array<float, 3>& current_position,
    float dx, float dy,
    const std::array<float, 3>& scene_center) -> OrbitResult {
    const auto offset = vec_subtract(current_position, scene_center);
    const std::array<float, 3> world_up = {0.f, 1.f, 0.f};
    const auto right = quat_rotate_vec(current_orientation, {1.f, 0.f, 0.f});

    const auto yaw_q = quat_from_axis_angle(world_up, dx * std::numbers::pi_v<float>);
    const auto pitch_q = quat_from_axis_angle(right, dy * std::numbers::pi_v<float>);
    const auto rotation = quat_normalize(quat_multiply(yaw_q, pitch_q));

    return {
        .new_orientation = quat_normalize(quat_multiply(rotation, current_orientation)),
        .new_position = vec_add(scene_center, quat_rotate_vec(rotation, offset)),
    };
}

auto NavigationController::compute_pan(
    const std::array<float, 3>& current_position,
    const std::array<float, 4>& current_orientation,
    float dx, float dy,
    float focal_distance) -> PanResult {
    const auto right = quat_rotate_vec(current_orientation, {1.f, 0.f, 0.f});
    const auto up = quat_rotate_vec(current_orientation, {0.f, 1.f, 0.f});
    const float scale = focal_distance * 2.0f;

    const auto pan_delta = vec_add(vec_scale(right, -dx * scale), vec_scale(up, -dy * scale));

    return {
        .new_position = vec_add(current_position, pan_delta),
    };
}

auto NavigationController::compute_zoom(
    const std::array<float, 3>& current_position,
    const std::array<float, 4>& current_orientation,
    float focal_distance,
    float delta,
    bool is_orthographic,
    float ortho_height) -> ZoomResult {
    if(is_orthographic) {
        const float factor = std::clamp(1.0f - delta * 0.1f, 0.01f, 10.0f);
        return {
            .new_focal_distance = focal_distance * factor,
            .new_position = current_position,
            .new_height = ortho_height * factor,
        };
    }

    const auto forward = quat_rotate_vec(current_orientation, {0.f, 0.f, -1.f});
    const float move_distance = delta * focal_distance * 0.1f;

    return {
        .new_focal_distance = std::max(0.01f, focal_distance - move_distance),
        .new_position = vec_add(current_position, vec_scale(forward, move_distance)),
        .new_height = ortho_height,
    };
}

} // namespace OpenGeoLab::Render
