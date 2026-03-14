#include <catch2/catch_test_macros.hpp>

#include <ogl/core/ComponentRequestDispatcher.hpp>
#include <ogl/geometry/GeometryComponentRegistration.hpp>

#include <nlohmann/json.hpp>

#include <string>

TEST_CASE("geometry placeholder model request returns deterministic payload", "[geometry][smoke]") {
    OGL::Geometry::registerGeometryComponents();

    const auto response = OGL::Core::ComponentRequestDispatcher::dispatch(
        "geometry", nlohmann::json{{"operation", "placeholderModel"},
                                   {"modelName", "SmokeTestModel"},
                                   {"bodyCount", 2},
                                   {"source", "test"}});

    REQUIRE(response.success);

    const auto model = response.payload.value("model", nlohmann::json::object());
    CHECK(model.value("modelName", std::string{}) == "SmokeTestModel");
    CHECK(model.value("bodyCount", 0) == 2);
}