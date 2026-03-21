#include <doctest/doctest.h>
#include <opengeolab/render/camera_state.hpp>

TEST_CASE("CameraState JSON round-trip - perspective") {
    OpenGeoLab::Render::CameraState state;
    state.projection = OpenGeoLab::Render::CameraState::ProjectionType::kPerspective;
    state.position = {1.f, 2.f, 3.f};
    state.orientation = {0.1f, 0.2f, 0.3f, 0.9f};
    state.near_distance = 0.5f;
    state.far_distance = 500.f;
    state.focal_distance = 10.f;
    state.height_angle = 0.6f;

    auto json = state.to_json();
    auto restored = OpenGeoLab::Render::CameraState::from_json(json);

    CHECK(restored.projection == state.projection);
    CHECK(restored.position == state.position);
    CHECK(restored.orientation == state.orientation);
    CHECK(restored.near_distance == doctest::Approx(state.near_distance));
    CHECK(restored.far_distance == doctest::Approx(state.far_distance));
    CHECK(restored.focal_distance == doctest::Approx(state.focal_distance));
    CHECK(restored.height_angle == doctest::Approx(state.height_angle));
}

TEST_CASE("CameraState JSON round-trip - orthographic") {
    OpenGeoLab::Render::CameraState state;
    state.projection = OpenGeoLab::Render::CameraState::ProjectionType::kOrthographic;
    state.height = 20.f;

    auto json = state.to_json();
    auto restored = OpenGeoLab::Render::CameraState::from_json(json);

    CHECK(restored.projection == state.projection);
    CHECK(restored.height == doctest::Approx(state.height));
}
