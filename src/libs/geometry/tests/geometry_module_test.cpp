/// @file geometry_module_test.cpp
/// @brief Tests for SceneStore and GeometryModule JSON dispatching.
#include <opengeolab/geometry/create_box_action.hpp>
#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/geometry/scene_store.hpp>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <array>
#include <string>
#include <string_view>
#include <vector>

TEST_CASE("createBox generates correct BoxData with progress") {
    std::vector<double> reported_progress;

    const OpenGeoLab::Geometry::ProgressCallback callback =
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

TEST_CASE("SceneStore addBox and allBoxes") {
    OpenGeoLab::Geometry::SceneStore store;

    OpenGeoLab::Geometry::BoxData first{};
    first.label = "first";
    OpenGeoLab::Geometry::BoxData second{};
    second.label = "second";
    OpenGeoLab::Geometry::BoxData third{};
    third.label = "third";

    CHECK(store.addBox(first) == 1);
    CHECK(store.addBox(second) == 2);
    CHECK(store.addBox(third) == 3);
    CHECK(store.boxCount() == 3);

    const auto boxes = store.allBoxes();
    REQUIRE(boxes.size() == 3);
    CHECK(boxes[0].first == 1);
    CHECK(boxes[1].first == 2);
    CHECK(boxes[2].first == 3);
    CHECK(boxes[0].second.label == "first");
    CHECK(boxes[1].second.label == "second");
    CHECK(boxes[2].second.label == "third");

    store.clear();
    CHECK(store.boxCount() == 0);
    CHECK(store.allBoxes().empty());
}

TEST_CASE("GeometryModule handles create_box action") {
    OpenGeoLab::Geometry::SceneStore store;
    OpenGeoLab::Geometry::GeometryModule module(store);

    nlohmann::json request;
    request["module"] = "geometry";
    request["action"] = "create_box";
    request["requestId"] = "test-123";
    request["param"] = {{"vertexCount", 5}, {"center", {0.0, 0.0, 0.0}}, {"size", {2.0, 2.0, 2.0}}};

    const auto response_str = module.process(request.dump());
    const auto response = nlohmann::json::parse(response_str);

    CHECK(response["ok"] == true);
    CHECK(response["module"] == "geometry");
    CHECK(response["action"] == "create_box");
    CHECK(response["requestId"] == "test-123");
    CHECK(response["result"]["id"].is_number_integer());
    CHECK(response["result"]["id"].get<int>() > 0);
    CHECK(response["result"]["vertexCount"] == 5);
    CHECK(store.boxCount() == 1);
}

TEST_CASE("GeometryModule rejects unknown action") {
    OpenGeoLab::Geometry::SceneStore store;
    OpenGeoLab::Geometry::GeometryModule module(store);

    nlohmann::json request;
    request["module"] = "geometry";
    request["action"] = "unknown_action";

    const auto response_str = module.process(request.dump());
    const auto response = nlohmann::json::parse(response_str);

    CHECK(response["ok"] == false);
}

TEST_CASE("GeometryModule list_boxes after create_box") {
    OpenGeoLab::Geometry::SceneStore store;
    OpenGeoLab::Geometry::GeometryModule module(store);

    const auto create_request = [](std::string_view request_id, std::array<double, 3> center) {
        nlohmann::json request;
        request["module"] = "geometry";
        request["action"] = "create_box";
        request["requestId"] = request_id;
        request["param"] = {{"vertexCount", 8}, {"center", center}, {"size", {1.0, 1.0, 1.0}}};
        return request;
    };

    const auto first_response =
        nlohmann::json::parse(module.process(create_request("create-1", {0.0, 0.0, 0.0}).dump()));
    const auto second_response =
        nlohmann::json::parse(module.process(create_request("create-2", {1.0, 2.0, 3.0}).dump()));

    CHECK(first_response["ok"] == true);
    CHECK(second_response["ok"] == true);

    nlohmann::json list_request;
    list_request["module"] = "geometry";
    list_request["action"] = "list_boxes";
    list_request["requestId"] = "list-1";

    const auto list_response = nlohmann::json::parse(module.process(list_request.dump()));

    CHECK(list_response["ok"] == true);
    CHECK(list_response["module"] == "geometry");
    CHECK(list_response["action"] == "list_boxes");
    CHECK(list_response["requestId"] == "list-1");
    CHECK(list_response["result"]["count"] == 2);
    REQUIRE(list_response["result"]["boxes"].is_array());
    REQUIRE(list_response["result"]["boxes"].size() == 2);
    CHECK(list_response["result"]["boxes"][0]["id"].is_number_integer());
    CHECK(list_response["result"]["boxes"][1]["id"].is_number_integer());
    CHECK(list_response["result"]["boxes"][0]["label"].is_string());
}
