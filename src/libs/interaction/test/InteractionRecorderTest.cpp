#include <opengeolab/interaction/InteractionRecorder.hpp>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

TEST_CASE("InteractionRecorder exports semantic replay script")
{
    const nlohmann::json operations = nlohmann::json::array(
        {nlohmann::json {
             {"kind", "camera.orbit"},
             {"view",
              {{"viewportId", "mainViewport"},
               {"width", 1280},
               {"height", 720},
               {"camera", {{"distance", 9.0}, {"azimuthDeg", 20.0}, {"elevationDeg", 30.0}}}}}
         },
         nlohmann::json {
             {"kind", "selection.box"},
             {"selection",
              {{"viewport",
                {{"viewportId", "mainViewport"},
                 {"width", 1280},
                 {"height", 720},
                 {"camera", {{"distance", 9.0}, {"azimuthDeg", 20.0}, {"elevationDeg", 30.0}}}}},
               {"rectangle", {{"left", 20}, {"top", 40}, {"right", 300}, {"bottom", 260}}},
               {"entityKinds", {"edge", "face"}}}}
         }}
    );

    const auto result = OpenGeoLab::Interaction::InteractionRecorder::exportPythonScript({
        {"operations", operations}
    });

    const auto script = result.at("script").get<std::string>();
    CHECK(script.find("restore_viewport") != std::string::npos);
    CHECK(script.find("box_select") != std::string::npos);
    CHECK(result.at("headlessReady").get<bool>());
}
