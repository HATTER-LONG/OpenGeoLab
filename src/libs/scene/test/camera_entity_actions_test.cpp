/**
 * @file camera_entity_actions_test.cpp
 * @brief Tests for LookAtEntityAction and BestViewForEntityAction
 */

#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/scene/best_view_for_entity_action.hpp>
#include <opengeolab/scene/look_at_entity_action.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <BRepPrimAPI_MakeBox.hxx>

#include <doctest/doctest.h>

#include <glm/geometric.hpp>

#include <cmath>

using OpenGeoLab::Geometry::ShapeStore;
using OpenGeoLab::Scene::BestViewForEntityAction;
using OpenGeoLab::Scene::LookAtEntityAction;
using OpenGeoLab::Scene::SceneGraph;

/// Create a SceneGraph with ShapeStore configured.
struct TestFixture {
    ShapeStore store;
    SceneGraph graph;
    uint32_t box_id{0};

    TestFixture() {
        box_id = store.add("Box", BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape());
        graph.setShapeStore(&store);
    }
};

TEST_SUITE("LookAtEntityAction") {

    TEST_CASE("describe returns expected schema") {
        SceneGraph graph;
        LookAtEntityAction action(graph);
        auto desc = action.describe();
        CHECK(desc["name"] == "look_at_entity");
        CHECK(desc["params"].contains("shapeId"));
        CHECK(desc["params"].contains("entityType"));
        CHECK(desc["params"].contains("localId"));
    }

    TEST_CASE("look at face updates camera") {
        TestFixture fix;
        LookAtEntityAction action(fix.graph);

        auto cam_before = fix.graph.viewportState().camera();
        float dist_before = cam_before.distance();

        auto result = action.execute(
            {{"shapeId", fix.box_id}, {"entityType", "face"}, {"localId", 1}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "look_at_entity");
        CHECK(result.contains("camera"));
        CHECK(result["camera"].contains("position"));
        CHECK(result["camera"].contains("target"));
        CHECK(result["camera"].contains("up"));

        auto cam_after = fix.graph.viewportState().camera();
        CHECK(cam_after.distance() == doctest::Approx(dist_before).epsilon(0.01));
    }

    TEST_CASE("look at edge updates camera") {
        TestFixture fix;
        LookAtEntityAction action(fix.graph);

        auto result = action.execute(
            {{"shapeId", fix.box_id}, {"entityType", "edge"}, {"localId", 1}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result.contains("camera"));
    }

    TEST_CASE("look at vertex updates camera") {
        TestFixture fix;
        LookAtEntityAction action(fix.graph);

        auto result = action.execute(
            {{"shapeId", fix.box_id}, {"entityType", "vertex"}, {"localId", 1}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result.contains("camera"));
    }

    TEST_CASE("error when ShapeStore not set") {
        SceneGraph graph;
        LookAtEntityAction action(graph);

        auto result =
            action.execute({{"shapeId", 1}, {"entityType", "face"}, {"localId", 1}}, nullptr);
        CHECK(result["ok"] == false);
        CHECK(result["error"].get<std::string>().find("ShapeStore") != std::string::npos);
    }

    TEST_CASE("error with unknown shapeId") {
        TestFixture fix;
        LookAtEntityAction action(fix.graph);

        auto result =
            action.execute({{"shapeId", 999}, {"entityType", "face"}, {"localId", 1}}, nullptr);
        CHECK(result["ok"] == false);
    }

    TEST_CASE("error with missing params") {
        TestFixture fix;
        LookAtEntityAction action(fix.graph);

        CHECK(action.execute(nlohmann::json::object(), nullptr)["ok"] == false);
        CHECK(action.execute({{"shapeId", 1}}, nullptr)["ok"] == false);
        CHECK(action.execute({{"shapeId", 1}, {"entityType", "face"}}, nullptr)["ok"] == false);
    }

    TEST_CASE("up vector is orthogonal to viewing direction") {
        TestFixture fix;
        LookAtEntityAction action(fix.graph);

        auto result = action.execute(
            {{"shapeId", fix.box_id}, {"entityType", "face"}, {"localId", 1}}, nullptr);
        REQUIRE(result["ok"] == true);

        auto cam = fix.graph.viewportState().camera();
        glm::vec3 view_dir = glm::normalize(cam.target - cam.position);
        float dot = std::abs(glm::dot(view_dir, cam.up));
        CHECK(dot < 0.1F);
    }

} // TEST_SUITE

TEST_SUITE("BestViewForEntityAction") {

    TEST_CASE("describe returns expected schema") {
        SceneGraph graph;
        BestViewForEntityAction action(graph);
        auto desc = action.describe();
        CHECK(desc["name"] == "best_view_for_entity");
        CHECK(desc["params"].contains("padding"));
    }

    TEST_CASE("best view for face auto-computes distance") {
        TestFixture fix;
        BestViewForEntityAction action(fix.graph);

        auto result = action.execute(
            {{"shapeId", fix.box_id}, {"entityType", "face"}, {"localId", 1}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "best_view_for_entity");
        CHECK(result.contains("camera"));
        CHECK(result.contains("entityBounds"));
        CHECK(result["entityBounds"].contains("min"));
        CHECK(result["entityBounds"].contains("max"));
    }

    TEST_CASE("padding affects distance") {
        TestFixture fix;
        BestViewForEntityAction action(fix.graph);

        auto result_small = action.execute(
            {{"shapeId", fix.box_id}, {"entityType", "face"}, {"localId", 1}, {"padding", 1.0}},
            nullptr);
        auto result_large = action.execute(
            {{"shapeId", fix.box_id}, {"entityType", "face"}, {"localId", 1}, {"padding", 3.0}},
            nullptr);

        REQUIRE(result_small["ok"] == true);
        REQUIRE(result_large["ok"] == true);

        auto pos_small = result_small["camera"]["position"];
        auto tgt_small = result_small["camera"]["target"];
        auto pos_large = result_large["camera"]["position"];
        auto tgt_large = result_large["camera"]["target"];

        auto dist = [](const nlohmann::json& p, const nlohmann::json& t) {
            float dx = p[0].get<float>() - t[0].get<float>();
            float dy = p[1].get<float>() - t[1].get<float>();
            float dz = p[2].get<float>() - t[2].get<float>();
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        };

        CHECK(dist(pos_large, tgt_large) > dist(pos_small, tgt_small));
    }

    TEST_CASE("best view for edge works") {
        TestFixture fix;
        BestViewForEntityAction action(fix.graph);

        auto result = action.execute(
            {{"shapeId", fix.box_id}, {"entityType", "edge"}, {"localId", 1}}, nullptr);
        CHECK(result["ok"] == true);
    }

    TEST_CASE("best view for vertex works") {
        TestFixture fix;
        BestViewForEntityAction action(fix.graph);

        auto result = action.execute(
            {{"shapeId", fix.box_id}, {"entityType", "vertex"}, {"localId", 1}}, nullptr);
        CHECK(result["ok"] == true);
    }

    TEST_CASE("error when ShapeStore not set") {
        SceneGraph graph;
        BestViewForEntityAction action(graph);

        auto result =
            action.execute({{"shapeId", 1}, {"entityType", "face"}, {"localId", 1}}, nullptr);
        CHECK(result["ok"] == false);
    }

    TEST_CASE("default padding is 1.5") {
        CHECK(BestViewForEntityAction::DEFAULT_PADDING == doctest::Approx(1.5F));
    }

} // TEST_SUITE
