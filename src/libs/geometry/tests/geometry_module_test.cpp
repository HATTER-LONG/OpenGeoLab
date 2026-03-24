/// @file geometry_module_test.cpp
/// @brief Tests for the geometry module JSON dispatcher.
#include <opengeolab/geometry/create_box_action.hpp>
#include <opengeolab/geometry/geometry_module.hpp>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <string>
#include <vector>

TEST_CASE("createBox generates correct BoxData with progress") {
    std::vector<double> reported_progress;

    OpenGeoLab::Geometry::ProgressCallback callback =
        [&reported_progress](double progress, std::string_view /*message*/) {
            reported_progress.push_back(progress);
        };

    const auto box =
        OpenGeoLab::Geometry::createBox({1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}, 10, callback);

    CHECK(box.center[0] == doctest::Approx(1.0));
    CHECK(box.center[1] == doctest::Approx(2.0));
    CHECK(box.center[2] == doctest::Approx(3.0));
    CHECK(box.size[0] == doctest::Approx(4.0));
    CHECK(box.size[1] == doctest::Approx(5.0));
    CHECK(box.size[2] == doctest::Approx(6.0));
    CHECK(box.vertexCount == 10);
    CHECK_FALSE(box.label.empty());

    // Should have received progress reports
    CHECK_FALSE(reported_progress.empty());
    // Last progress should be 1.0
    CHECK(reported_progress.back() == doctest::Approx(1.0));
}

TEST_CASE("processGeometry handles create_box action") {
    nlohmann::json request;
    request["module"] = "geometry";
    request["action"] = "create_box";
    request["requestId"] = "test-123";
    request["param"] = {{"vertexCount", 5}, {"center", {0.0, 0.0, 0.0}}, {"size", {2.0, 2.0, 2.0}}};

    const auto response_str = OpenGeoLab::Geometry::processGeometry(request.dump());
    const auto response = nlohmann::json::parse(response_str);

    CHECK(response["ok"] == true);
    CHECK(response["module"] == "geometry");
    CHECK(response["action"] == "create_box");
    CHECK(response["request_id"] == "test-123");
    CHECK(response["result"]["vertexCount"] == 5);
}

TEST_CASE("processGeometry rejects unknown action") {
    nlohmann::json request;
    request["module"] = "geometry";
    request["action"] = "unknown_action";

    const auto response_str = OpenGeoLab::Geometry::processGeometry(request.dump());
    const auto response = nlohmann::json::parse(response_str);

    CHECK(response["ok"] == false);
}
