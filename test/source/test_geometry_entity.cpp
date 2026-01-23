/**
 * @file test_geometry_entity.cpp
 * @brief Unit tests for GeometryEntity and GeometryManager classes
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <geometry/geometry_entity.hpp>
#include <geometry/geometry_types.hpp>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>

using Catch::Approx;

// =============================================================================
// GeometryEntity Tests
// =============================================================================
namespace {

TEST_CASE("GeometryEntity default construction", "[geometry][entity]") {
    OpenGeoLab::Geometry::GeometryEntity entity;

    CHECK(entity.entityId() != OpenGeoLab::Geometry::INVALID_ENTITY_ID);
    CHECK(entity.entityUID() == OpenGeoLab::Geometry::INVALID_ENTITY_UID);
    CHECK(entity.entityType() == OpenGeoLab::Geometry::EntityType::None);
    CHECK_FALSE(entity.hasShape());
    CHECK(entity.name().empty());
}

TEST_CASE("GeometryEntity construction from OCC shape", "[geometry][entity]") {
    // Create a simple box shape
    BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
    TopoDS_Shape box_shape = make_box.Shape();

    OpenGeoLab::Geometry::GeometryEntity entity(box_shape);

    CHECK(entity.entityId() != OpenGeoLab::Geometry::INVALID_ENTITY_ID);
    CHECK(entity.entityUID() != OpenGeoLab::Geometry::INVALID_ENTITY_UID);
    CHECK(entity.entityType() == OpenGeoLab::Geometry::EntityType::Solid);
    CHECK(entity.hasShape());
    CHECK_FALSE(entity.shape().IsNull());
}

TEST_CASE("GeometryEntity type detection", "[geometry][entity]") {
    SECTION("Solid detection") {
        BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
        OpenGeoLab::Geometry::GeometryEntity entity(make_box.Shape());
        CHECK(entity.entityType() == OpenGeoLab::Geometry::EntityType::Solid);
    }

    SECTION("Compound detection") {
        BRepPrimAPI_MakeBox make_box1(10.0, 10.0, 10.0);
        BRepPrimAPI_MakeSphere make_sphere(5.0);

        TopoDS_Compound compound;
        BRep_Builder builder;
        builder.MakeCompound(compound);
        builder.Add(compound, make_box1.Shape());
        builder.Add(compound, make_sphere.Shape());

        OpenGeoLab::Geometry::GeometryEntity entity(compound);
        CHECK(entity.entityType() == OpenGeoLab::Geometry::EntityType::Compound);
    }
}

TEST_CASE("GeometryEntity bounding box", "[geometry][entity]") { // NOLINT
    // Create a box at origin with size 10x10x10
    BRepPrimAPI_MakeBox make_box(10.0, 20.0, 30.0);
    OpenGeoLab::Geometry::GeometryEntity entity(make_box.Shape());

    SECTION("Bounding box is computed lazily") {
        CHECK_FALSE(entity.hasBoundingBox());
        [[maybe_unused]] OpenGeoLab::Geometry::BoundingBox3D bbox = entity.boundingBox();
        CHECK(entity.hasBoundingBox());
    }

    SECTION("Bounding box values are correct") {
        OpenGeoLab::Geometry::BoundingBox3D bbox = entity.boundingBox();
        CHECK(bbox.isValid());

        // Box created from origin with dimensions 10x20x30
        CHECK(bbox.m_min.m_x == Approx(0.0).margin(0.01));
        CHECK(bbox.m_min.m_y == Approx(0.0).margin(0.01));
        CHECK(bbox.m_min.m_z == Approx(0.0).margin(0.01));
        CHECK(bbox.m_max.m_x == Approx(10.0).margin(0.01));
        CHECK(bbox.m_max.m_y == Approx(20.0).margin(0.01));
        CHECK(bbox.m_max.m_z == Approx(30.0).margin(0.01));
    }

    SECTION("Bounding box invalidation") {
        [[maybe_unused]] auto bbox = entity.boundingBox(); // Compute it
        CHECK(entity.hasBoundingBox());

        entity.invalidateBoundingBox();
        CHECK_FALSE(entity.hasBoundingBox());
    }
}

TEST_CASE("GeometryEntity naming", "[geometry][entity]") {
    OpenGeoLab::Geometry::GeometryEntity entity;

    CHECK(entity.name().empty());

    entity.setName("TestEntity");
    CHECK(entity.name() == "TestEntity");

    entity.setName("");
    CHECK(entity.name().empty());
}

TEST_CASE("GeometryEntity parent-child relationships", "[geometry][entity]") {
    auto parent = std::make_shared<OpenGeoLab::Geometry::GeometryEntity>();
    auto child1 = std::make_shared<OpenGeoLab::Geometry::GeometryEntity>();
    auto child2 = std::make_shared<OpenGeoLab::Geometry::GeometryEntity>();

    SECTION("Adding children") {
        parent->addChild(child1);
        parent->addChild(child2);

        CHECK(parent->children().size() == 2);
        CHECK(child1->parent().lock() == parent);
        CHECK(child2->parent().lock() == parent);
    }

    SECTION("Removing children") {
        parent->addChild(child1);
        parent->addChild(child2);

        bool removed = parent->removeChild(child1);
        CHECK(removed);
        CHECK(parent->children().size() == 1);
        CHECK(child1->parent().expired());
        CHECK(child2->parent().lock() == parent);
    }

    SECTION("Removing non-existent child returns false") {
        parent->addChild(child1);
        auto other = std::make_shared<OpenGeoLab::Geometry::GeometryEntity>();

        bool removed = parent->removeChild(other);
        CHECK_FALSE(removed);
        CHECK(parent->children().size() == 1);
    }
}

// =============================================================================
// GeometryManager Tests
// =============================================================================

TEST_CASE("GeometryManager entity registration", "[geometry][manager]") {
    OpenGeoLab::Geometry::GeometryManager manager;

    BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
    auto entity = std::make_shared<OpenGeoLab::Geometry::GeometryEntity>(make_box.Shape());

    SECTION("Register and find by ID") {
        manager.registerEntity(entity);

        CHECK(manager.entityCount() == 1);
        auto found = manager.findById(entity->entityId());
        CHECK(found == entity);
    }

    SECTION("Find by type and UID") {
        manager.registerEntity(entity);

        auto found = manager.findByTypeAndUID(entity->entityType(), entity->entityUID());
        CHECK(found == entity);
    }

    SECTION("Find by shape") {
        manager.registerEntity(entity);

        auto found = manager.findByShape(entity->shape());
        CHECK(found == entity);
    }

    SECTION("Unregister entity") {
        manager.registerEntity(entity);
        CHECK(manager.entityCount() == 1);

        bool removed = manager.unregisterEntity(entity);
        CHECK(removed);
        CHECK(manager.entityCount() == 0);
        CHECK(manager.findById(entity->entityId()) == nullptr);
    }
}

TEST_CASE("GeometryManager entity enumeration", "[geometry][manager]") {
    OpenGeoLab::Geometry::GeometryManager manager;

    // Create entities of different types
    BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
    BRepPrimAPI_MakeSphere make_sphere(5.0);

    auto box_entity = std::make_shared<OpenGeoLab::Geometry::GeometryEntity>(make_box.Shape());
    auto sphere_entity =
        std::make_shared<OpenGeoLab::Geometry::GeometryEntity>(make_sphere.Shape());

    manager.registerEntity(box_entity);
    manager.registerEntity(sphere_entity);

    SECTION("Get all entities") {
        auto all = manager.getAllEntities();
        CHECK(all.size() == 2);
    }

    SECTION("Get entities by type") {
        auto solids = manager.getEntitiesByType(OpenGeoLab::Geometry::EntityType::Solid);
        CHECK(solids.size() == 2); // Both box and sphere are solids
    }

    SECTION("Entity count by type") {
        CHECK(manager.entityCountByType(OpenGeoLab::Geometry::EntityType::Solid) == 2);
        CHECK(manager.entityCountByType(OpenGeoLab::Geometry::EntityType::Vertex) == 0);
    }
}

TEST_CASE("GeometryManager shape import", "[geometry][manager]") {
    OpenGeoLab::Geometry::GeometryManager manager;

    SECTION("Import simple shape") {
        BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
        auto root = manager.importShape(make_box.Shape());

        CHECK(root != nullptr);
        CHECK(root->hasShape());
        CHECK(manager.entityCount() == 1);
    }

    SECTION("Import compound shape creates hierarchy") {
        BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
        BRepPrimAPI_MakeSphere make_sphere(5.0);

        TopoDS_Compound compound;
        BRep_Builder builder;
        builder.MakeCompound(compound);
        builder.Add(compound, make_box.Shape());
        builder.Add(compound, make_sphere.Shape());

        auto root = manager.importShape(compound);

        CHECK(root != nullptr);
        CHECK(root->entityType() == OpenGeoLab::Geometry::EntityType::Compound);
        CHECK(root->children().size() == 2);
        CHECK(manager.entityCount() == 3); // compound + box + sphere
    }

    SECTION("Import null shape returns nullptr") {
        TopoDS_Shape null_shape;
        auto result = manager.importShape(null_shape);
        CHECK(result == nullptr);
    }
}

TEST_CASE("GeometryManager createEntityFromShape", "[geometry][manager]") {
    OpenGeoLab::Geometry::GeometryManager manager;

    BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
    BRepPrimAPI_MakeSphere make_sphere(5.0);

    SECTION("Create without parent") {
        auto entity = manager.createEntityFromShape(make_box.Shape());

        CHECK(entity != nullptr);
        CHECK(entity->hasShape());
        CHECK(entity->parent().expired());
        CHECK(manager.entityCount() == 1);
    }

    SECTION("Create with parent") {
        auto parent = manager.createEntityFromShape(make_box.Shape());
        auto child = manager.createEntityFromShape(make_sphere.Shape(), parent);

        CHECK(child->parent().lock() == parent);
        CHECK(parent->children().size() == 1);
        CHECK(parent->children()[0] == child);
    }
}

TEST_CASE("GeometryManager clear", "[geometry][manager]") {
    OpenGeoLab::Geometry::GeometryManager manager;

    BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
    manager.createEntityFromShape(make_box.Shape());
    manager.createEntityFromShape(make_box.Shape());
    manager.createEntityFromShape(make_box.Shape());

    CHECK(manager.entityCount() == 3);

    manager.clear();

    CHECK(manager.entityCount() == 0);
    CHECK(manager.getAllEntities().empty());
}

TEST_CASE("GeometryManager lookup edge cases", "[geometry][manager]") {
    OpenGeoLab::Geometry::GeometryManager manager;

    SECTION("Find non-existent entity by ID returns nullptr") {
        auto found = manager.findById(999);
        CHECK(found == nullptr);
    }

    SECTION("Find by invalid entity ID returns nullptr") {
        auto found = manager.findById(OpenGeoLab::Geometry::INVALID_ENTITY_ID);
        CHECK(found == nullptr);
    }

    SECTION("Find by null shape returns nullptr") {
        TopoDS_Shape null_shape;
        auto found = manager.findByShape(null_shape);
        CHECK(found == nullptr);
    }
}

} // namespace