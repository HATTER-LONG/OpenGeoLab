/**
 * @file geometry_module_test.cpp
 * @brief Placeholder tests for the geometry module.
 */

#include <doctest/doctest.h>

#include <opengeolab/geometry/geometry_module.hpp>

TEST_CASE("GeometryModule stub returns not-implemented") {
    OpenGeoLab::Geometry::GeometryModule mod;
    auto result = mod.process(R"({"action":"unknown"})");
    CHECK(result.find("not implemented") != std::string::npos);
}
