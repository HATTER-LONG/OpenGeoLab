#include <doctest/doctest.h>

#include <nlohmann/json.hpp>
#include <opengeolab/command/command_dispatcher.hpp>

#include <algorithm>
#include <memory>

namespace OpenGeoLab::Command {
namespace {

/// @brief Lightweight mock for CommandDispatcher tests.
class MockCommand final : public ICommand {
public:
    [[nodiscard]] auto actionName() const noexcept -> std::string_view override {
        return "mock.action";
    }

    [[nodiscard]] auto execute(const nlohmann::json& payload) -> CommandResult override {
        return CommandResult{
            .ok = true,
            .summary = "Mock executed.",
            .result = {{"echo", payload}},
        };
    }
};

TEST_CASE("CommandDispatcher reports registered actions") {
    CommandDispatcher dispatcher;
    dispatcher.registerCommand(std::make_unique<MockCommand>());

    std::vector<std::string> actions;
    for(const std::string_view action : dispatcher.registeredActions()) {
        actions.emplace_back(action);
    }
    std::ranges::sort(actions);

    REQUIRE(actions.size() == 1);
    CHECK(actions.front() == "mock.action");
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

TEST_CASE("CommandDispatcher executes registered mock command") {
    CommandDispatcher dispatcher;
    dispatcher.registerCommand(std::make_unique<MockCommand>());

    const auto response = nlohmann::json::parse(dispatcher.dispatch(
        R"({"requestId":"req-2","action":"mock.action","payload":{"key":"value"}})"));

    REQUIRE(response.at("ok") == true);
    CHECK(response.at("protocolVersion") == "1.0");
    CHECK(response.at("requestId") == "req-2");
    CHECK(response.at("action") == "mock.action");
    CHECK(response.at("summary") == "Mock executed.");
    REQUIRE(response.at("result").is_object());
    CHECK(response.at("result").at("echo") == nlohmann::json{{"key", "value"}});
}

} // namespace
} // namespace OpenGeoLab::Command
