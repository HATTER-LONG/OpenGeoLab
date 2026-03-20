#include <opengeolab/selection/SelectionService.hpp>

#include <doctest/doctest.h>

TEST_CASE("SelectionService describes box selection with headless replay metadata")
{
    const auto result = OpenGeoLab::Selection::SelectionService::describeBoxSelection({
        {"viewport",
         {{"viewportId", "mainViewport"},
          {"width", 1280},
          {"height", 720},
          {"camera", {{"distance", 10.0}, {"azimuthDeg", 30.0}, {"elevationDeg", 25.0}}}}},
        {"rectangle", {{"left", 10}, {"top", 20}, {"right", 300}, {"bottom", 200}}},
        {"entityKinds", {"edge", "face"}}
    });

    CHECK(result.at("selectionMode").get<std::string>() == "box");
    CHECK(result.at("filters").at("entityKinds").size() == 2);
    CHECK(result.at("replayBoundary").at("headlessReady").get<bool>());
}
