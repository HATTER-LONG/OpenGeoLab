/**
 * @file geometry_document_test.cpp
 * @brief Unit tests for GeometryDocument and entity management
 */

#include "geometry/geometry_document.hpp"
#include "geometry/geometry_entity.hpp"
#include "geometry/geometry_types.hpp"
#include "geometry/part_entity.hpp"
#include "geometry/shape_builder.hpp"
#include "geometry/solid_entity.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Solid.hxx>

#include <catch2/catch_test_macros.hpp>

using namespace OpenGeoLab::Geometry; // NOLINT

namespace {

/**
 * @brief Create a simple box shape for testing
 */
TopoDS_Shape createTestBox(double dx = 10.0, double dy = 10.0, double dz = 10.0) {
    BRepPrimAPI_MakeBox maker(dx, dy, dz);
    maker.Build();
    return maker.Shape();
}

/**
 * @brief Create a simple sphere shape for testing
 */
TopoDS_Shape createTestSphere(double radius = 5.0) {
    BRepPrimAPI_MakeSphere maker(radius);
    maker.Build();
    return maker.Shape();
}

// =============================================================================
// GeometryDocument Basic Tests
// =============================================================================

TEST_CASE("GeometryDocument - Creation and basic operations", "[geometry][document]") { // NOLINT
    auto doc = GeometryDocument::create();

    REQUIRE(doc != nullptr);
    CHECK(doc->entityCount() == 0);

    SECTION("Add and find entity by ID") {
        auto shape = createTestBox();
        auto part = std::make_shared<PartEntity>(shape);

        bool added = doc->addEntity(part);
        REQUIRE(added);
        CHECK(doc->entityCount() == 1);

        auto found = doc->findById(part->entityId());
        REQUIRE(found != nullptr);
        CHECK(found->entityId() == part->entityId());
    }

    SECTION("Add and find entity by UID and type") {
        auto shape = createTestBox();
        auto part = std::make_shared<PartEntity>(shape);

        REQUIRE(doc->addEntity(part));

        auto found = doc->findByUIDAndType(part->entityUID(), EntityType::Part);
        REQUIRE(found != nullptr);
        CHECK(found->entityType() == EntityType::Part);
    }

    SECTION("Add and find entity by shape") {
        auto shape = createTestBox();
        auto part = std::make_shared<PartEntity>(shape);

        REQUIRE(doc->addEntity(part));

        auto found = doc->findByShape(shape);
        REQUIRE(found != nullptr);
        CHECK(found->entityId() == part->entityId());
    }

    SECTION("Remove entity") {
        auto shape = createTestBox();
        auto part = std::make_shared<PartEntity>(shape);
        EntityId part_id = part->entityId();

        REQUIRE(doc->addEntity(part));
        CHECK(doc->entityCount() == 1);

        bool removed = doc->removeEntity(part_id);
        REQUIRE(removed);
        CHECK(doc->entityCount() == 0);
        CHECK(doc->findById(part_id) == nullptr);
    }

    SECTION("Clear document") {
        auto shape1 = createTestBox();
        auto shape2 = createTestSphere();
        auto part1 = std::make_shared<PartEntity>(shape1);
        auto part2 = std::make_shared<PartEntity>(shape2);

        REQUIRE(doc->addEntity(part1));
        REQUIRE(doc->addEntity(part2));
        CHECK(doc->entityCount() == 2);

        doc->clear();
        CHECK(doc->entityCount() == 0);
    }
}

// =============================================================================
// Entity Relationship Tests
// =============================================================================

TEST_CASE("GeometryDocument - Entity relationships", // NOLINT
          "[geometry][document][relationships]") {
    auto doc = GeometryDocument::create();
    auto box_shape = createTestBox();

    SECTION("Parent-child edge management") {
        auto part = std::make_shared<PartEntity>(box_shape);
        REQUIRE(doc->addEntity(part));

        // Create a solid entity from the box shape (which is a solid)
        TopoDS_Solid solid = TopoDS::Solid(box_shape);
        auto solid_entity = std::make_shared<SolidEntity>(solid);
        REQUIRE(doc->addEntity(solid_entity));

        // Part can have Solid as child
        bool edge_added = doc->addChildEdge(part->entityId(), solid_entity->entityId());
        REQUIRE(edge_added);

        // Verify relationship
        auto children = part->children();
        REQUIRE(children.size() == 1);
        CHECK(children[0]->entityId() == solid_entity->entityId());

        auto parents = solid_entity->parents();
        REQUIRE(parents.size() == 1);
        CHECK(parents[0]->entityId() == part->entityId());
    }

    SECTION("Remove parent-child edge") {
        auto part = std::make_shared<PartEntity>(box_shape);
        REQUIRE(doc->addEntity(part));

        TopoDS_Solid solid = TopoDS::Solid(box_shape);
        auto solid_entity = std::make_shared<SolidEntity>(solid);
        REQUIRE(doc->addEntity(solid_entity));

        REQUIRE(doc->addChildEdge(part->entityId(), solid_entity->entityId()));

        bool removed = doc->removeChildEdge(part->entityId(), solid_entity->entityId());
        REQUIRE(removed);

        CHECK(part->children().empty());
        CHECK(solid_entity->parents().empty());
    }
}

// =============================================================================
// ShapeBuilder Tests
// =============================================================================

TEST_CASE("ShapeBuilder - Build entity hierarchy from shape", "[geometry][builder]") {
    auto doc = GeometryDocument::create();
    auto shape = createTestBox();

    ShapeBuilder builder(doc);
    auto result = builder.buildFromShape(shape, "TestBox");

    REQUIRE(result.m_success);
    REQUIRE(result.m_rootPart != nullptr);

    CHECK(result.m_rootPart->name() == "TestBox");
    CHECK(result.m_rootPart->entityType() == EntityType::Part);

    // Box should have faces, edges, vertices
    CHECK(result.m_faceCount == 6); // Box has 6 faces
    CHECK(result.m_edgeCount > 0);
    CHECK(result.m_vertexCount > 0);

    // Document should contain all entities
    CHECK(doc->entityCount() > 1);
    CHECK(doc->entityCountByType(EntityType::Part) == 1);
    CHECK(doc->entityCountByType(EntityType::Face) == 6);
}

// =============================================================================
// Entity Query Tests
// =============================================================================

TEST_CASE("GeometryDocument - Entity queries", "[geometry][document][query]") { // NOLINT
    auto doc = GeometryDocument::create();
    auto shape = createTestBox();

    ShapeBuilder builder(doc);
    auto result = builder.buildFromShape(shape, "TestBox");
    REQUIRE(result.m_success);

    SECTION("Find entities by type") {
        auto faces = doc->entitiesByType(EntityType::Face);
        CHECK(faces.size() == 6);

        auto parts = doc->entitiesByType(EntityType::Part);
        CHECK(parts.size() == 1);
    }

    SECTION("Get all entities") {
        auto all = doc->allEntities();
        CHECK(all.size() == doc->entityCount());
    }

    SECTION("Find owning part") {
        auto faces = doc->entitiesByType(EntityType::Face);
        REQUIRE(!faces.empty());

        auto owning_part = doc->findOwningPart(faces[0]->entityId());
        REQUIRE(owning_part != nullptr);
        CHECK(owning_part->entityType() == EntityType::Part);
    }

    SECTION("Find descendants by type") {
        auto faces = doc->findDescendants(result.m_rootPart->entityId(), EntityType::Face);
        CHECK(faces.size() == 6);

        auto edges = doc->findDescendants(result.m_rootPart->entityId(), EntityType::Edge);
        CHECK(edges.size() > 0);
    }

    SECTION("Find ancestors by type") {
        auto faces = doc->entitiesByType(EntityType::Face);
        REQUIRE(!faces.empty());

        auto parts = doc->findAncestors(faces[0]->entityId(), EntityType::Part);
        CHECK(parts.size() == 1);
    }
}

// =============================================================================
// Recursive Deletion Tests
// =============================================================================

TEST_CASE("GeometryDocument - Recursive entity deletion", "[geometry][document][deletion]") {
    auto doc = GeometryDocument::create();
    auto shape = createTestBox();

    ShapeBuilder builder(doc);
    auto result = builder.buildFromShape(shape, "TestBox");
    REQUIRE(result.m_success);

    size_t initial_count = doc->entityCount();
    REQUIRE(initial_count > 0);

    SECTION("Remove entity with children") {
        EntityId part_id = result.m_rootPart->entityId();
        size_t removed = doc->removeEntityWithChildren(part_id);

        CHECK(removed == initial_count);
        CHECK(doc->entityCount() == 0);
    }
}

// =============================================================================
// ID Generation Tests
// =============================================================================

TEST_CASE("Geometry ID generation", "[geometry][ids]") {
    SECTION("EntityId uniqueness") {
        EntityId id1 = generateEntityId();
        EntityId id2 = generateEntityId();
        EntityId id3 = generateEntityId();

        CHECK(id1 != id2);
        CHECK(id2 != id3);
        CHECK(id1 != id3);
        CHECK(id1 != INVALID_ENTITY_ID);
    }

    SECTION("EntityUID per type") {
        EntityUID uid1 = generateEntityUID(EntityType::Face);
        EntityUID uid2 = generateEntityUID(EntityType::Face);
        EntityUID uid3 = generateEntityUID(EntityType::Edge);

        CHECK(uid1 != uid2); // Same type, different UIDs
        CHECK(uid1 != INVALID_ENTITY_UID);
        CHECK(uid3 != INVALID_ENTITY_UID);
    }
}

// =============================================================================
// BoundingBox Tests
// =============================================================================

TEST_CASE("Entity bounding box", "[geometry][bbox]") {
    auto shape = createTestBox(10.0, 20.0, 30.0);
    auto part = std::make_shared<PartEntity>(shape);

    auto bbox = part->boundingBox();

    CHECK(bbox.isValid());

    // Check approximate dimensions (box at origin)
    auto size = bbox.size();
    CHECK(size.m_x > 9.0);
    CHECK(size.m_x < 11.0);
    CHECK(size.m_y > 19.0);
    CHECK(size.m_y < 21.0);
    CHECK(size.m_z > 29.0);
    CHECK(size.m_z < 31.0);
}

} // anonymous namespace