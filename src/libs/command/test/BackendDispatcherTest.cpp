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

TEST_CASE("BackendDispatcher describes box selection with replay metadata")
{
    const auto response = processRequest({
        {"protocolVersion", "1.0"},
        {"requestId", "test-box-selection"},
        {"source", "doctest"},
        {"action", "selection.box.describe"},
        {"payload",
         {{"viewport",
           {{"viewportId", "mainViewport"},
            {"width", 1280},
            {"height", 720},
            {"camera", {{"distance", 8.0}, {"azimuthDeg", 30.0}, {"elevationDeg", 20.0}}}}},
          {"rectangle", {{"left", 50}, {"top", 70}, {"right", 500}, {"bottom", 350}}},
          {"entityKinds", {"edge", "face"}}}},
        {"context", nlohmann::json::object()}
    });

    CHECK(response.at("ok").get<bool>());
    CHECK(response.at("result").at("selectionMode").get<std::string>() == "box");
    CHECK(response.at("result").at("replayBoundary").at("headlessReady").get<bool>());
}

TEST_CASE("BackendDispatcher exports replayable Python script")
{
    const nlohmann::json operations = nlohmann::json::array(
        {nlohmann::json {
             {"kind", "camera.orbit"},
             {"view",
              {{"viewportId", "mainViewport"},
               {"width", 1280},
               {"height", 720},
               {"camera", {{"distance", 9.0}, {"azimuthDeg", 25.0}, {"elevationDeg", 18.0}}}}}
         },
         nlohmann::json {
             {"kind", "selection.box"},
             {"selection",
              {{"viewport",
                {{"viewportId", "mainViewport"},
                 {"width", 1280},
                 {"height", 720},
                 {"camera", {{"distance", 9.0}, {"azimuthDeg", 25.0}, {"elevationDeg", 18.0}}}}},
               {"rectangle", {{"left", 40}, {"top", 60}, {"right", 420}, {"bottom", 280}}},
               {"entityKinds", {"edge", "face"}}}}
         }}
    );

    const auto response = processRequest({
        {"protocolVersion", "1.0"},
        {"requestId", "test-python-export"},
        {"source", "doctest"},
        {"action", "interaction.export.python"},
        {"payload", {{"operations", operations}}},
        {"context", nlohmann::json::object()}
    });

    CHECK(response.at("ok").get<bool>());
    const auto script = response.at("result").at("script").get<std::string>();
    CHECK(script.find("restore_viewport") != std::string::npos);
    CHECK(script.find("box_select") != std::string::npos);
}
