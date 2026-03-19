#include <opengeolab/command/BackendDispatcher.hpp>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

namespace
{

[[nodiscard]] nlohmann::json processRequest(const nlohmann::json& request)
{
    return nlohmann::json::parse(OpenGeoLab::Command::BackendDispatcher::process(request.dump()));
}

}  // namespace

TEST_CASE("BackendDispatcher responds to ping requests")
{
    const auto response = processRequest({
        {"protocolVersion", "1.0"},
        {"requestId", "test-ping"},
        {"source", "doctest"},
        {"action", "system.ping"},
        {"payload", nlohmann::json::object()},
        {"context", nlohmann::json::object()}
    });

    CHECK(response.at("ok").get<bool>());
    CHECK(response.at("action").get<std::string>() == "system.ping");
    CHECK(response.at("result").at("capabilities").at("render").at("snapshot").get<bool>());
}

TEST_CASE("BackendDispatcher computes box metrics")
{
    const auto response = processRequest({
        {"protocolVersion", "1.0"},
        {"requestId", "test-box"},
        {"source", "doctest"},
        {"action", "geometry.box.describe"},
        {"payload", {{"width", 2.0}, {"height", 3.0}, {"depth", 4.0}}},
        {"context", nlohmann::json::object()}
    });

    CHECK(response.at("ok").get<bool>());
    CHECK(response.at("result").at("metrics").at("volume").get<double>() == doctest::Approx(24.0));
    CHECK(
        response.at("result").at("metrics").at("surfaceArea").get<double>() == doctest::Approx(52.0)
    );
}
