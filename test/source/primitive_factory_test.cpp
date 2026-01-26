/**
 * @file primitive_factory_test.cpp
 * @brief Unit tests for PrimitiveFactory
 */

#include "geometry/geometry_document.hpp"
#include "geometry/geometry_types.hpp"
#include "geometry/primitive_factory.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

using namespace OpenGeoLab::Geometry;

TEST_CASE("PrimitiveFactory - create box") {
    // Reset ID generators for clean tests
    resetEntityIdGenerator();
    for(int i = 0; i <= static_cast<int>(EntityType::Part); ++i) {
        resetEntityUIDGenerator(static_cast<EntityType>(i));
    }

    auto document = GeometryDocument::create();
    auto part = PrimitiveFactory::createBox(10.0, 20.0, 30.0, document);

    REQUIRE(part != nullptr);
    REQUIRE(part->entityType() == EntityType::Part);
    REQUIRE(part->name() == "Box");
    REQUIRE(part->hasChildren());

    // Check bounding box size approximately matches
    auto bbox = part->boundingBox();
    REQUIRE(bbox.isValid());

    double dx = bbox.m_max.m_x - bbox.m_min.m_x;
    double dy = bbox.m_max.m_y - bbox.m_min.m_y;
    double dz = bbox.m_max.m_z - bbox.m_min.m_z;

    REQUIRE(std::abs(dx - 10.0) < 0.001);
    REQUIRE(std::abs(dy - 20.0) < 0.001);
    REQUIRE(std::abs(dz - 30.0) < 0.001);
}

TEST_CASE("PrimitiveFactory - create box from points") {
    auto document = GeometryDocument::create();
    Point3D p1(0.0, 0.0, 0.0);
    Point3D p2(5.0, 10.0, 15.0);

    auto part = PrimitiveFactory::createBox(p1, p2, document);

    REQUIRE(part != nullptr);
    REQUIRE(part->name() == "Box");
}

TEST_CASE("PrimitiveFactory - create box invalid dimensions") {
    auto document = GeometryDocument::create();

    // Zero dimensions
    auto part1 = PrimitiveFactory::createBox(0.0, 10.0, 10.0, document);
    REQUIRE(part1 == nullptr);

    // Negative dimensions
    auto part2 = PrimitiveFactory::createBox(-5.0, 10.0, 10.0, document);
    REQUIRE(part2 == nullptr);
}

TEST_CASE("PrimitiveFactory - create sphere") {
    auto document = GeometryDocument::create();
    auto part = PrimitiveFactory::createSphere(5.0, document);

    REQUIRE(part != nullptr);
    REQUIRE(part->name() == "Sphere");
    REQUIRE(part->hasChildren());

    // Bounding box should be roughly 10x10x10 centered at origin
    auto bbox = part->boundingBox();
    REQUIRE(bbox.isValid());

    double dx = bbox.m_max.m_x - bbox.m_min.m_x;
    REQUIRE(std::abs(dx - 10.0) < 0.01);
}

TEST_CASE("PrimitiveFactory - create sphere at center") {
    auto document = GeometryDocument::create();
    Point3D center(10.0, 20.0, 30.0);

    auto part = PrimitiveFactory::createSphere(center, 3.0, document);

    REQUIRE(part != nullptr);

    auto bbox = part->boundingBox();
    REQUIRE(bbox.isValid());

    // Center should be at (10, 20, 30)
    double cx = (bbox.m_max.m_x + bbox.m_min.m_x) / 2.0;
    double cy = (bbox.m_max.m_y + bbox.m_min.m_y) / 2.0;
    double cz = (bbox.m_max.m_z + bbox.m_min.m_z) / 2.0;

    REQUIRE(std::abs(cx - 10.0) < 0.01);
    REQUIRE(std::abs(cy - 20.0) < 0.01);
    REQUIRE(std::abs(cz - 30.0) < 0.01);
}

TEST_CASE("PrimitiveFactory - create sphere invalid radius") {
    auto document = GeometryDocument::create();

    auto part = PrimitiveFactory::createSphere(0.0, document);
    REQUIRE(part == nullptr);

    auto part2 = PrimitiveFactory::createSphere(-1.0, document);
    REQUIRE(part2 == nullptr);
}

TEST_CASE("PrimitiveFactory - create cylinder") {
    auto document = GeometryDocument::create();
    auto part = PrimitiveFactory::createCylinder(5.0, 20.0, document);

    REQUIRE(part != nullptr);
    REQUIRE(part->name() == "Cylinder");
    REQUIRE(part->hasChildren());

    auto bbox = part->boundingBox();
    REQUIRE(bbox.isValid());

    // Height should be 20
    double dz = bbox.m_max.m_z - bbox.m_min.m_z;
    REQUIRE(std::abs(dz - 20.0) < 0.01);
}

TEST_CASE("PrimitiveFactory - create cylinder invalid params") {
    auto document = GeometryDocument::create();

    auto part1 = PrimitiveFactory::createCylinder(0.0, 10.0, document);
    REQUIRE(part1 == nullptr);

    auto part2 = PrimitiveFactory::createCylinder(5.0, 0.0, document);
    REQUIRE(part2 == nullptr);
}

TEST_CASE("PrimitiveFactory - create cone") {
    auto document = GeometryDocument::create();
    auto part = PrimitiveFactory::createCone(10.0, 5.0, 15.0, document);

    REQUIRE(part != nullptr);
    REQUIRE(part->name() == "Cone");
    REQUIRE(part->hasChildren());
}

TEST_CASE("PrimitiveFactory - create cone becomes cylinder") {
    auto document = GeometryDocument::create();

    // Equal radii should create a cylinder
    auto part = PrimitiveFactory::createCone(5.0, 5.0, 10.0, document);
    REQUIRE(part != nullptr);
    REQUIRE(part->name() == "Cylinder"); // Falls back to cylinder
}

TEST_CASE("PrimitiveFactory - create torus") {
    auto document = GeometryDocument::create();
    auto part = PrimitiveFactory::createTorus(10.0, 2.0, document);

    REQUIRE(part != nullptr);
    REQUIRE(part->name() == "Torus");
    REQUIRE(part->hasChildren());
}

TEST_CASE("PrimitiveFactory - create torus invalid params") {
    auto document = GeometryDocument::create();

    // Minor radius must be positive
    auto part1 = PrimitiveFactory::createTorus(10.0, 0.0, document);
    REQUIRE(part1 == nullptr);

    // Major must be greater than minor
    auto part2 = PrimitiveFactory::createTorus(5.0, 5.0, document);
    REQUIRE(part2 == nullptr);

    auto part3 = PrimitiveFactory::createTorus(3.0, 5.0, document);
    REQUIRE(part3 == nullptr);
}

TEST_CASE("PrimitiveFactory - create wedge") {
    auto document = GeometryDocument::create();
    auto part = PrimitiveFactory::createWedge(10.0, 20.0, 30.0, 5.0, document);

    REQUIRE(part != nullptr);
    REQUIRE(part->name() == "Wedge");
    REQUIRE(part->hasChildren());
}

TEST_CASE("PrimitiveFactory - auto document creation") {
    // Use default document manager
    auto& manager = GeometryDocumentManager::instance();

    // Create without specifying document
    auto part = PrimitiveFactory::createBox(5.0, 5.0, 5.0, nullptr);

    REQUIRE(part != nullptr);

    // Should have created a document
    auto current_doc = manager.currentDocument();
    REQUIRE(current_doc != nullptr);
    REQUIRE(current_doc->entityCount() > 0);
}

} // namespace
