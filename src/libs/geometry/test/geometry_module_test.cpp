/**
 * @file geometry_module_test.cpp
 * @brief Unit tests for GeometryModule and CreateBoxAction
 */

#include <opengeolab/geometry/create_box_action.hpp>
#include <opengeolab/geometry/geometry_module.hpp>

#include <doctest/doctest.h>

TEST_CASE("GeometryModule describe returns module info with actions") {
    OpenGeoLab::Geometry::GeometryModule mod;
    auto desc = mod.describe();
    CHECK(desc["name"] == "geometry");
    CHECK(desc.contains("description"));
    CHECK(desc["actions"].is_array());
    CHECK(desc["actions"].size() == 1);
    CHECK(desc["actions"][0]["name"] == "create_box");
}

TEST_CASE("GeometryModule dispatches create_box action") {
    OpenGeoLab::Geometry::GeometryModule mod;
    nlohmann::json request = {{"module", "geometry"},
                              {"action", "create_box"},
                              {"param", {{"width", 2.0}, {"height", 3.0}, {"depth", 4.0}}}};

    // Track progress calls
    std::vector<double> progress_values;
    auto progress_cb = [&](double p, const std::string&) {
        progress_values.push_back(p);
        return true;
    };

    auto result = mod.process(request, progress_cb);
    CHECK(result["ok"] == true);
    CHECK(result["action"] == "create_box");
    CHECK(result["data"]["width"] == 2.0);
    CHECK(result["data"]["height"] == 3.0);
    CHECK(result["data"]["depth"] == 4.0);

    // Should have 11 progress calls: 0.0 + 10 steps
    CHECK(progress_values.size() == 11);
    CHECK(progress_values.front() == doctest::Approx(0.0));
    CHECK(progress_values.back() == doctest::Approx(1.0));
}

TEST_CASE("GeometryModule throws on missing action field") {
    OpenGeoLab::Geometry::GeometryModule mod;
    nlohmann::json request = {{"module", "geometry"}, {"param", {{"width", 1.0}}}};
    CHECK_THROWS_AS((void)mod.process(request, OpenGeoLab::Core::NO_PROGRESS_CALLBACK),
                    std::invalid_argument);
}

TEST_CASE("GeometryModule throws on unknown action") {
    OpenGeoLab::Geometry::GeometryModule mod;
    nlohmann::json request = {{"module", "geometry"}, {"action", "unknown_action"}};
    CHECK_THROWS_AS((void)mod.process(request, OpenGeoLab::Core::NO_PROGRESS_CALLBACK),
                    std::invalid_argument);
}

TEST_CASE("CreateBoxAction describe returns action metadata") {
    OpenGeoLab::Geometry::CreateBoxAction action;
    auto desc = action.describe();
    CHECK(desc["name"] == "create_box");
    CHECK(desc.contains("description"));
    CHECK(desc.contains("params"));
    CHECK(desc["params"].contains("width"));
    CHECK(desc["params"].contains("height"));
    CHECK(desc["params"].contains("depth"));
}

TEST_CASE("CreateBoxAction uses default dimensions when params missing") {
    OpenGeoLab::Geometry::CreateBoxAction action;
    nlohmann::json param = nlohmann::json::object();
    auto result = action.execute(param, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);
    CHECK(result["data"]["width"] == 1.0);
    CHECK(result["data"]["height"] == 1.0);
    CHECK(result["data"]["depth"] == 1.0);
}

TEST_CASE("CreateBoxAction supports cancellation via progress callback") {
    OpenGeoLab::Geometry::CreateBoxAction action;
    nlohmann::json param = {{"width", 1.0}};

    int call_count = 0;
    auto cancel_cb = [&](double, const std::string&) {
        ++call_count;
        return call_count < 4; // Cancel after 3 progress calls
    };

    auto result = action.execute(param, cancel_cb);
    CHECK(result["ok"] == false);
    CHECK(result.contains("summary"));
}
