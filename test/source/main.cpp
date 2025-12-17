/**
 * @file main.cpp
 * @brief Main entry point for OpenGeoLab unit tests
 */

#include <catch2/catch_test_macros.hpp>

namespace {

TEST_CASE("Sanity check - basic assertions") {
    CHECK(1 + 1 == 2);
    CHECK(true);
    CHECK_FALSE(false);
}

TEST_CASE("Sanity check - string operations") {
    std::string hello = "Hello";
    std::string world = "World";

    CHECK(hello + " " + world == "Hello World");
    CHECK(hello.size() == 5);
}

} // namespace