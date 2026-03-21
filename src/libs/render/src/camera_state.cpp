/**
 * @file camera_state.cpp
 * @brief CameraState JSON serialization implementation.
 */

#include <opengeolab/render/camera_state.hpp>

#include <nlohmann/json.hpp>

#include <string>

namespace OpenGeoLab::Render {

namespace {

auto projection_to_string(CameraState::ProjectionType projection) -> std::string {
    switch (projection) {
    case CameraState::ProjectionType::kPerspective:
        return "perspective";
    case CameraState::ProjectionType::kOrthographic:
        return "orthographic";
    }
    return "perspective";
}

auto string_to_projection(const std::string& value) -> CameraState::ProjectionType {
    if (value == "orthographic") {
        return CameraState::ProjectionType::kOrthographic;
    }
    return CameraState::ProjectionType::kPerspective;
}

} // namespace

auto CameraState::to_json() const -> nlohmann::json {
    return nlohmann::json{
        {"projection", projection_to_string(projection)},
        {"position", position},
        {"orientation", orientation},
        {"near_distance", near_distance},
        {"far_distance", far_distance},
        {"focal_distance", focal_distance},
        {"height_angle", height_angle},
        {"height", height},
    };
}

auto CameraState::from_json(const nlohmann::json& json) -> CameraState {
    CameraState state;
    if (json.contains("projection")) {
        state.projection = string_to_projection(json["projection"].get<std::string>());
    }
    if (json.contains("position")) {
        state.position = json["position"].get<std::array<float, 3>>();
    }
    if (json.contains("orientation")) {
        state.orientation = json["orientation"].get<std::array<float, 4>>();
    }
    if (json.contains("near_distance")) {
        state.near_distance = json["near_distance"].get<float>();
    }
    if (json.contains("far_distance")) {
        state.far_distance = json["far_distance"].get<float>();
    }
    if (json.contains("focal_distance")) {
        state.focal_distance = json["focal_distance"].get<float>();
    }
    if (json.contains("height_angle")) {
        state.height_angle = json["height_angle"].get<float>();
    }
    if (json.contains("height")) {
        state.height = json["height"].get<float>();
    }
    return state;
}

} // namespace OpenGeoLab::Render
