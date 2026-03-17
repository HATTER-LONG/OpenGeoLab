#include <catch2/catch_test_macros.hpp>

#include <ogl/geometry/GeometryModel.hpp>

#include <string>

TEST_CASE("geometry model normalizes missing descriptor defaults", "[geometry][unit]") {
    const OGL::Geometry::GeometryModel model({.modelName = "", .bodyCount = 0, .source = ""});

    CHECK(model.modelName() == "GeometryModel");
    CHECK(model.bodyCount() == 1);
    CHECK(model.source() == "component");
    CHECK(model.summary().find("GeometryModel") != std::string::npos);

    const auto model_json = model.toJson();
    CHECK(model_json.at("modelName") == "GeometryModel");
    CHECK(model_json.at("bodyCount") == 1);
    CHECK(model_json.at("source") == "component");
}
