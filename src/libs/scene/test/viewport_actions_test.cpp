/**
 * @file viewport_actions_test.cpp
 * @brief Tests for viewport camera and pick area actions.
 */

#include <opengeolab/scene/fit_to_scene_action.hpp>
#include <opengeolab/scene/pick_area_action.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/set_camera_action.hpp>
#include <opengeolab/scene/set_view_preset_action.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Scene::FitToSceneAction;
using OpenGeoLab::Scene::PickAreaAction;
using OpenGeoLab::Scene::SceneGraph;
using OpenGeoLab::Scene::SetCameraAction;
using OpenGeoLab::Scene::SetViewPresetAction;

TEST_SUITE("ViewportActions") {

    TEST_CASE("FitToSceneAction resets camera when scene is empty") {
        SceneGraph graph;
        FitToSceneAction action(graph);

        const auto result = action.execute({}, nullptr);
        CHECK(result["ok"] == true);

        const auto cam = graph.viewportState().camera();
        CHECK(cam.position.z == doctest::Approx(50.0F));
    }

    TEST_CASE("SetViewPresetAction sets Front view") {
        SceneGraph graph;
        auto& vps = graph.viewportState();
        SetViewPresetAction action(vps);

        const auto result = action.execute({{"preset", "Front"}}, nullptr);
        CHECK(result["ok"] == true);

        const auto cam = vps.camera();
        CHECK(cam.position.z > cam.target.z);
        CHECK(cam.position.x == doctest::Approx(cam.target.x));
    }

    TEST_CASE("SetViewPresetAction sets Top view") {
        SceneGraph graph;
        auto& vps = graph.viewportState();
        SetViewPresetAction action(vps);

        const auto result = action.execute({{"preset", "Top"}}, nullptr);
        CHECK(result["ok"] == true);

        const auto cam = vps.camera();
        CHECK(cam.position.y > cam.target.y);
        CHECK(cam.up.z == doctest::Approx(-1.0F));
    }

    TEST_CASE("SetViewPresetAction rejects invalid preset") {
        SceneGraph graph;
        SetViewPresetAction action(graph.viewportState());

        const auto result = action.execute({{"preset", "InvalidPreset"}}, nullptr);
        CHECK(result["ok"] == false);
    }

    TEST_CASE("SetCameraAction sets position, target, up") {
        SceneGraph graph;
        auto& vps = graph.viewportState();
        SetCameraAction action(vps);

        const auto result = action.execute({{"position", {10.0, 20.0, 30.0}},
                                            {"target", {1.0, 2.0, 3.0}},
                                            {"up", {0.0, 1.0, 0.0}}},
                                           nullptr);

        CHECK(result["ok"] == true);
        const auto cam = vps.camera();
        CHECK(cam.position.x == doctest::Approx(10.0F));
        CHECK(cam.position.y == doctest::Approx(20.0F));
        CHECK(cam.position.z == doctest::Approx(30.0F));
        CHECK(cam.target.x == doctest::Approx(1.0F));
        CHECK(cam.target.y == doctest::Approx(2.0F));
        CHECK(cam.target.z == doctest::Approx(3.0F));
    }

    TEST_CASE("SetCameraAction rejects missing position") {
        SceneGraph graph;
        SetCameraAction action(graph.viewportState());

        const auto result =
            action.execute({{"target", {0.0, 0.0, 0.0}}, {"up", {0.0, 1.0, 0.0}}}, nullptr);
        CHECK(result["ok"] == false);
    }

    TEST_CASE("PickAreaAction stores pending pick with normalized coords") {
        SceneGraph graph;
        auto& vps = graph.viewportState();
        PickAreaAction action(vps);

        const auto result = action.execute({{"x0", 0.2},
                                            {"y0", 0.3},
                                            {"x1", 0.8},
                                            {"y1", 0.9},
                                            {"coordType", "normalized"},
                                            {"pickAction", "Add"}},
                                           nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["async"] == true);

        const auto pending = vps.consumePickArea();
        REQUIRE(pending.has_value());
        CHECK(pending->x0 == doctest::Approx(0.2F));
        CHECK(pending->y1 == doctest::Approx(0.9F));
        CHECK(pending->coordType == OpenGeoLab::Scene::PickAreaCoordType::Normalized);
    }

    TEST_CASE("PickAreaAction stores pending pick with pixel coords") {
        SceneGraph graph;
        auto& vps = graph.viewportState();
        PickAreaAction action(vps);

        const auto result = action.execute({{"x0", 100},
                                            {"y0", 200},
                                            {"x1", 400},
                                            {"y1", 500},
                                            {"coordType", "pixel"},
                                            {"pickAction", "Remove"}},
                                           nullptr);

        CHECK(result["ok"] == true);
        const auto pending = vps.consumePickArea();
        REQUIRE(pending.has_value());
        CHECK(pending->coordType == OpenGeoLab::Scene::PickAreaCoordType::Pixel);
        CHECK(pending->action == OpenGeoLab::Core::PickAction::Remove);
    }

    TEST_CASE("PickAreaAction defaults to normalized and Add") {
        SceneGraph graph;
        auto& vps = graph.viewportState();
        PickAreaAction action(vps);

        const auto result =
            action.execute({{"x0", 0.0}, {"y0", 0.0}, {"x1", 1.0}, {"y1", 1.0}}, nullptr);

        CHECK(result["ok"] == true);
        const auto pending = vps.consumePickArea();
        REQUIRE(pending.has_value());
        CHECK(pending->coordType == OpenGeoLab::Scene::PickAreaCoordType::Normalized);
        CHECK(pending->action == OpenGeoLab::Core::PickAction::Add);
    }

} // TEST_SUITE
