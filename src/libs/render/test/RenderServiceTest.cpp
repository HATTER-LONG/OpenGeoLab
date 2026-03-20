#include <opengeolab/render/RenderService.hpp>

#include <doctest/doctest.h>

TEST_CASE("RenderService normalizes viewport state for replay")
{
    const auto result = OpenGeoLab::Render::RenderService::describeViewport({
        {"viewportId", "testViewport"},
        {"width", 1440},
        {"height", 900},
        {"camera",
         {{"target", {{"x", 1.0}, {"y", 2.0}, {"z", 3.0}}},
          {"distance", 12.5},
          {"azimuthDeg", 40.0},
          {"elevationDeg", 15.0}}}
    });

    CHECK(result.at("viewportId").get<std::string>() == "testViewport");
    CHECK(result.at("camera").at("distance").get<double>() == doctest::Approx(12.5));
    CHECK(result.at("replayBoundary").at("headlessReady").get<bool>());
}
