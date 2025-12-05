/**
 * @file main.cpp
 * @brief Main entry point for OpenGeoLab unit tests
 *
 * This file configures doctest as the testing framework and serves as
 * the entry point for all test cases in the project.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>

// =============================================================================
// Basic Sanity Tests
// =============================================================================

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
