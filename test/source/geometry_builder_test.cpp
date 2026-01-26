/**
 * @file geometry_builder_test.cpp
 * @brief Unit tests for GeometryBuilder
 */

#include "geometry/geometry_builder.hpp"
#include "geometry/geometry_document.hpp"
#include "geometry/geometry_types.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <catch2/catch_test_macros.hpp>

namespace {

using namespace OpenGeoLab::Geometry;

TEST_CASE("GeometryBuilder - build box hierarchy") {
    // Reset ID generators for predictable tests
    resetEntityIdGenerator();
    for(int i = 0; i <= static_cast<int>(EntityType::Part); ++i) {
        resetEntityUIDGenerator(static_cast<EntityType>(i));
    }

    auto document = GeometryDocument::create();
    GeometryBuilder builder(document);

    // Create a simple box
    BRepPrimAPI_MakeBox box_maker(10.0, 20.0, 30.0);
    box_maker.Build();
    REQUIRE(box_maker.IsDone());

    TopoDS_Shape box_shape = box_maker.Shape();
    REQUIRE_FALSE(box_shape.IsNull());

    // Build entity hierarchy
    auto result = builder.buildFromShape(box_shape, "TestBox", nullptr);

    REQUIRE(result.m_success);
    REQUIRE(result.m_partEntity != nullptr);
    REQUIRE(result.m_partEntity->name() == "TestBox");
    REQUIRE(result.m_partEntity->entityType() == EntityType::Part);

    // Check document has entities
    REQUIRE(document->entityCount() > 1); // At least Part + some children

    // Part should have children
    REQUIRE(result.m_partEntity->hasChildren());

    // The box shape is a Solid
    auto children = result.m_partEntity->children();
    REQUIRE_FALSE(children.empty());

    // Find the solid child
    bool found_solid = false;
    for(const auto& child : children) {
        if(child->entityType() == EntityType::Solid) {
            found_solid = true;
            REQUIRE(child->hasChildren()); // Solid should have Shell children
        }
    }
    REQUIRE(found_solid);
}

TEST_CASE("GeometryBuilder - null shape returns failure") {
    auto document = GeometryDocument::create();
    GeometryBuilder builder(document);

    TopoDS_Shape null_shape;
    auto result = builder.buildFromShape(null_shape, "NullPart", nullptr);

    REQUIRE_FALSE(result.m_success);
    REQUIRE(result.m_partEntity == nullptr);
    REQUIRE_FALSE(result.m_errorMessage.empty());
}

TEST_CASE("GeometryBuilder - null document returns failure") {
    GeometryBuilder builder(nullptr);

    BRepPrimAPI_MakeBox box_maker(10.0, 10.0, 10.0);
    box_maker.Build();

    auto result = builder.buildFromShape(box_maker.Shape(), "TestBox", nullptr);

    REQUIRE_FALSE(result.m_success);
    REQUIRE_FALSE(result.m_errorMessage.empty());
}

TEST_CASE("GeometryBuilder - progress callback") {
    auto document = GeometryDocument::create();
    GeometryBuilder builder(document);

    BRepPrimAPI_MakeSphere sphere_maker(5.0);
    sphere_maker.Build();

    int callback_count = 0;
    double last_progress = 0.0;

    auto result = builder.buildFromShape(
        sphere_maker.Shape(), "TestSphere",
        [&callback_count, &last_progress](double progress, const std::string& /*message*/) {
            ++callback_count;
            REQUIRE(progress >= last_progress); // Progress should be non-decreasing
            last_progress = progress;
            return true; // Continue
        });

    REQUIRE(result.m_success);
    REQUIRE(callback_count > 0);
}

TEST_CASE("GeometryBuilder - cancellation via callback") {
    auto document = GeometryDocument::create();
    GeometryBuilder builder(document);

    BRepPrimAPI_MakeBox box_maker(10.0, 10.0, 10.0);
    box_maker.Build();

    int callback_count = 0;

    auto result = builder.buildFromShape(
        box_maker.Shape(), "CancelledBox",
        [&callback_count](double /*progress*/, const std::string& /*message*/) {
            ++callback_count;
            return callback_count < 2; // Cancel after first callback
        });

    // Should fail due to cancellation
    REQUIRE_FALSE(result.m_success);
}

TEST_CASE("GeometryBuilder - shape deduplication") {
    // Reset for clean test
    resetEntityIdGenerator();
    for(int i = 0; i <= static_cast<int>(EntityType::Part); ++i) {
        resetEntityUIDGenerator(static_cast<EntityType>(i));
    }

    auto document = GeometryDocument::create();
    GeometryBuilder builder(document);

    BRepPrimAPI_MakeBox box_maker(10.0, 10.0, 10.0);
    box_maker.Build();

    auto result = builder.buildFromShape(box_maker.Shape(), "DedupeTest", nullptr);
    REQUIRE(result.m_success);

    // Count entities of each type
    size_t solid_count = document->entityCountByType(EntityType::Solid);
    size_t face_count = document->entityCountByType(EntityType::Face);

    // A box has 1 solid and 6 faces
    REQUIRE(solid_count == 1);
    REQUIRE(face_count == 6);
}

} // namespace
