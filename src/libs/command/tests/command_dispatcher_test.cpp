#include <doctest/doctest.h>

#include <opengeolab/command/bounding_box_command.hpp>
#include <opengeolab/command/command_dispatcher.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace OpenGeoLab::Command {
namespace {

TEST_CASE("CommandDispatcher reports registered actions") {
    CommandDispatcher dispatcher;
    dispatcher.registerCommand(std::make_unique<BoundingBoxCommand>());

    std::vector<std::string> actions;
    for(const std::string_view action : dispatcher.registeredActions()) {
        actions.emplace_back(action);
    }
    std::ranges::sort(actions);

    REQUIRE(actions.size() == 1);
    CHECK(actions.front() == "geometry.bounding_box");
}

TEST_CASE("CommandDispatcher returns error payload for unknown action") {
    CommandDispatcher dispatcher;

    const auto response = nlohmann::json::parse(
        dispatcher.dispatch(R"({"requestId":"req-1","action":"unknown.action"})"));

    CHECK(response.at("protocolVersion") == "1.0");
    CHECK(response.at("requestId") == "req-1");
    CHECK(response.at("ok") == false);
    CHECK(response.at("action") == "unknown.action");
    CHECK(response.at("summary") == "Unknown action: unknown.action");
    CHECK(response.at("errors").is_array());
}

TEST_CASE("CommandDispatcher executes bounding box command") {
    CommandDispatcher dispatcher;
    dispatcher.registerCommand(std::make_unique<BoundingBoxCommand>());

    const auto response = nlohmann::json::parse(dispatcher.dispatch(
        R"({"requestId":"req-2","action":"geometry.bounding_box","payload":{"pointCount":32,"seed":7}})"));

    REQUIRE(response.at("ok") == true);
    CHECK(response.at("protocolVersion") == "1.0");
    CHECK(response.at("requestId") == "req-2");
    CHECK(response.at("action") == "geometry.bounding_box");
    CHECK(response.at("summary") == "Computed bounding box.");
    REQUIRE(response.at("result").is_object());
    CHECK(response.at("result").at("pointCount") == 32);
    CHECK(response.at("result").at("elapsedMs").is_number());
    CHECK(response.at("result").at("min").is_object());
    CHECK(response.at("result").at("max").is_object());
}

} // namespace
} // namespace OpenGeoLab::Command
