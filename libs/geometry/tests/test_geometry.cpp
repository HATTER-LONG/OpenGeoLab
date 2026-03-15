#include <catch2/catch_test_macros.hpp>

#include <ogl/core/ComponentRequestDispatcher.hpp>
#include <ogl/geometry/GeometryComponentRegistration.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <string>
#include <vector>

TEST_CASE("geometry model request returns deterministic payload", "[geometry][smoke]") {
    OGL::Geometry::registerGeometryComponents();

    const auto response = OGL::Core::ComponentRequestDispatcher::dispatch(
        "geometry", "inspectModel",
        nlohmann::json{{"modelName", "SmokeTestModel"}, {"bodyCount", 2}, {"source", "test"}});

    REQUIRE(response.success);

    const auto model = response.payload.value("model", nlohmann::json::object());
    CHECK(model.value("modelName", std::string{}) == "SmokeTestModel");
    CHECK(model.value("bodyCount", 0) == 2);
}

TEST_CASE("geometry create operations return structured payloads", "[geometry][smoke][create]") {
    OGL::Geometry::registerGeometryComponents();

    const auto assertCreateResponse = [](const std::string& operation_name,
                                         const std::string& expected_shape,
                                         const std::string& expected_model_name,
                                         const nlohmann::json& request) {
        const auto response =
            OGL::Core::ComponentRequestDispatcher::dispatch("geometry", operation_name, request);

        REQUIRE(response.success);
        CHECK(response.action == operation_name);
        CHECK(response.payload.value("shapeType", std::string{}) == expected_shape);

        const auto model = response.payload.value("model", nlohmann::json::object());
        CHECK(model.value("modelName", std::string{}) == expected_model_name);
        CHECK(model.value("bodyCount", 0) == 1);
    };

    SECTION("createBox returns the normalized model data") {
        assertCreateResponse("createBox", "box", "BoxSmoke",
                             {{"modelName", "BoxSmoke"},
                              {"origin", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}},
                              {"dimensions", {{"x", 120.0}, {"y", 80.0}, {"z", 60.0}}},
                              {"source", "test"}});
    }

    SECTION("createCylinder returns the normalized model data") {
        assertCreateResponse("createCylinder", "cylinder", "CylinderSmoke",
                             {{"modelName", "CylinderSmoke"},
                              {"baseCenter", {{"x", 0.0}, {"y", 4.0}, {"z", -2.0}}},
                              {"radius", 32.0},
                              {"height", 120.0},
                              {"axis", "Y"},
                              {"source", "test"}});
    }

    SECTION("createSphere returns the normalized model data") {
        assertCreateResponse("createSphere", "sphere", "SphereSmoke",
                             {{"modelName", "SphereSmoke"},
                              {"center", {{"x", 6.0}, {"y", 0.0}, {"z", 3.0}}},
                              {"radius", 48.0},
                              {"source", "test"}});
    }

    SECTION("createTorus returns the normalized model data") {
        assertCreateResponse("createTorus", "torus", "TorusSmoke",
                             {{"modelName", "TorusSmoke"},
                              {"center", {{"x", 1.0}, {"y", 2.0}, {"z", 3.0}}},
                              {"majorRadius", 90.0},
                              {"minorRadius", 24.0},
                              {"axis", "Z"},
                              {"source", "test"}});
    }
}

TEST_CASE("geometry createBox reports intermediate progress", "[geometry][progress][createBox]") {
    OGL::Geometry::registerGeometryComponents();

    std::vector<double> progress_values;
    std::vector<std::string> progress_messages;
    const auto response = OGL::Core::ComponentRequestDispatcher::dispatch(
        {.module = "geometry",
         .action = "createBox",
         .param = {{"modelName", "ProgressBox"},
                   {"origin", {{"x", 1.0}, {"y", 2.0}, {"z", 3.0}}},
                   {"dimensions", {{"x", 50.0}, {"y", 40.0}, {"z", 30.0}}},
                   {"source", "test"}}},
        [&](double progress, const std::string& message) {
            progress_values.push_back(progress);
            progress_messages.push_back(message);
            return true;
        });

    REQUIRE(response.success);
    CHECK(progress_values.size() >= 4);
    CHECK(std::any_of(progress_values.begin(), progress_values.end(),
                      [](double progress) { return progress > 0.0 && progress < 1.0; }));
    CHECK(std::any_of(
        progress_messages.begin(), progress_messages.end(),
        [](const std::string& message) { return message.find("box") != std::string::npos; }));
}
