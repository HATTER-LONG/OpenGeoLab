/**
 * @file geometry_creator_test.cpp
 * @brief Unit tests for GeometryCreator component
 */

#include "geometry/geometry_creator.hpp"
#include "geometry/geometry_document.hpp"
#include "geometry/part_entity.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

namespace {

using namespace OpenGeoLab::Geometry;
using Catch::Matchers::WithinRel;

TEST_CASE("GeometryCreator - create point") {
    auto doc = GeometryDocument::create();
    REQUIRE(doc != nullptr);

    SECTION("create point with coordinates") {
        auto point = GeometryCreator::createPoint(doc, "TestPoint", 1.0, 2.0, 3.0);

        REQUIRE(point != nullptr);
        REQUIRE(point->name() == "TestPoint");
        REQUIRE(point->entityType() == EntityType::Part);
        REQUIRE(doc->entityCount() == 1);
        REQUIRE(doc->findById(point->entityId()) == point);
    }

    SECTION("create point with null document fails") {
        auto point = GeometryCreator::createPoint(nullptr, "TestPoint", 0.0, 0.0, 0.0);
        REQUIRE(point == nullptr);
    }

    SECTION("create multiple points") {
        auto p1 = GeometryCreator::createPoint(doc, "Point1", 0.0, 0.0, 0.0);
        auto p2 = GeometryCreator::createPoint(doc, "Point2", 1.0, 1.0, 1.0);
        auto p3 = GeometryCreator::createPoint(doc, "Point3", 2.0, 2.0, 2.0);

        REQUIRE(p1 != nullptr);
        REQUIRE(p2 != nullptr);
        REQUIRE(p3 != nullptr);
        REQUIRE(doc->entityCount() == 3);
    }
}

TEST_CASE("GeometryCreator - create line") {
    auto doc = GeometryDocument::create();
    REQUIRE(doc != nullptr);

    SECTION("create line with valid endpoints") {
        auto line = GeometryCreator::createLine(doc, "TestLine", 0.0, 0.0, 0.0, 10.0, 0.0, 0.0);

        REQUIRE(line != nullptr);
        REQUIRE(line->name() == "TestLine");
        REQUIRE(line->entityType() == EntityType::Part);
        REQUIRE(doc->entityCount() == 1);
    }

    SECTION("create degenerate line fails") {
        auto line =
            GeometryCreator::createLine(doc, "DegenerateLine", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        REQUIRE(line == nullptr);
        REQUIRE(doc->entityCount() == 0);
    }

    SECTION("create line with null document fails") {
        auto line = GeometryCreator::createLine(nullptr, "TestLine", 0.0, 0.0, 0.0, 1.0, 0.0, 0.0);
        REQUIRE(line == nullptr);
    }
}

TEST_CASE("GeometryCreator - create box") {
    auto doc = GeometryDocument::create();
    REQUIRE(doc != nullptr);

    SECTION("create box with valid dimensions") {
        auto box = GeometryCreator::createBox(doc, "TestBox", 0.0, 0.0, 0.0, 10.0, 20.0, 30.0);

        REQUIRE(box != nullptr);
        REQUIRE(box->name() == "TestBox");
        REQUIRE(box->entityType() == EntityType::Part);
        REQUIRE(doc->entityCount() == 1);
    }

    SECTION("create box with zero dimensions fails") {
        auto box = GeometryCreator::createBox(doc, "ZeroBox", 0.0, 0.0, 0.0, 0.0, 10.0, 10.0);
        REQUIRE(box == nullptr);
    }

    SECTION("create box with negative dimensions fails") {
        auto box = GeometryCreator::createBox(doc, "NegativeBox", 0.0, 0.0, 0.0, -10.0, 10.0, 10.0);
        REQUIRE(box == nullptr);
    }

    SECTION("create box with null document fails") {
        auto box = GeometryCreator::createBox(nullptr, "TestBox", 0.0, 0.0, 0.0, 10.0, 10.0, 10.0);
        REQUIRE(box == nullptr);
    }
}

TEST_CASE("GeometryCreator - create from JSON") {
    auto doc = GeometryDocument::create();
    REQUIRE(doc != nullptr);

    SECTION("create point from JSON") {
        nlohmann::json params = {{"name", "JsonPoint"},
                                 {"coordinates", {{"x", 5.0}, {"y", 10.0}, {"z", 15.0}}}};

        auto point = GeometryCreator::createFromJson(doc, "createPoint", params);
        REQUIRE(point != nullptr);
        REQUIRE(point->name() == "JsonPoint");
    }

    SECTION("create line from JSON") {
        nlohmann::json params = {{"name", "JsonLine"},
                                 {"start", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}},
                                 {"end", {{"x", 10.0}, {"y", 10.0}, {"z", 10.0}}}};

        auto line = GeometryCreator::createFromJson(doc, "createLine", params);
        REQUIRE(line != nullptr);
        REQUIRE(line->name() == "JsonLine");
    }

    SECTION("create box from JSON") {
        nlohmann::json params = {{"name", "JsonBox"},
                                 {"origin", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}},
                                 {"dimensions", {{"x", 5.0}, {"y", 5.0}, {"z", 5.0}}}};

        auto box = GeometryCreator::createFromJson(doc, "createBox", params);
        REQUIRE(box != nullptr);
        REQUIRE(box->name() == "JsonBox");
    }

    SECTION("unknown action returns nullptr") {
        nlohmann::json params = {{"name", "Unknown"}};
        auto entity = GeometryCreator::createFromJson(doc, "unknownAction", params);
        REQUIRE(entity == nullptr);
    }

    SECTION("missing required fields handled gracefully") {
        // createPoint without coordinates - should use defaults
        nlohmann::json params = {{"name", "NoCoords"}};

        // This should fail because "coordinates" is expected via .at()
        auto entity = GeometryCreator::createFromJson(doc, "createPoint", params);
        REQUIRE(entity == nullptr);
    }
}

} // namespace
