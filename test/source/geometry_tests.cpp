/**
 * @file geometry_tests.cpp
 * @brief Unit tests for geometry types, entities, and document management
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "geometry/geometry_document.hpp"
#include "geometry/geometry_entity.hpp"
#include "geometry/geometry_types.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <TopoDS_Shape.hxx>

using namespace OpenGeoLab::Geometry;

// =============================================================================
// Point3D Tests
// =============================================================================

TEST_CASE("Point3D - Basic construction and access", "[geometry][point]") {
    SECTION("Default constructor creates origin") {
        Point3D p;
        CHECK(p.m_x == 0.0);
        CHECK(p.m_y == 0.0);
        CHECK(p.m_z == 0.0);
    }

    SECTION("Parameterized constructor sets coordinates") {
        Point3D p(1.0, 2.0, 3.0);
        CHECK(p.m_x == 1.0);
        CHECK(p.m_y == 2.0);
        CHECK(p.m_z == 3.0);
    }

    SECTION("Origin static method") {
        auto origin = Point3D::origin();
        CHECK(origin == Point3D(0.0, 0.0, 0.0));
    }
}

TEST_CASE("Point3D - Arithmetic operations", "[geometry][point]") {
    Point3D a(1.0, 2.0, 3.0);
    Point3D b(4.0, 5.0, 6.0);

    SECTION("Addition") {
        auto result = a + b;
        CHECK(result == Point3D(5.0, 7.0, 9.0));
    }

    SECTION("Subtraction") {
        auto result = b - a;
        CHECK(result == Point3D(3.0, 3.0, 3.0));
    }

    SECTION("Scalar multiplication") {
        auto result = a * 2.0;
        CHECK(result == Point3D(2.0, 4.0, 6.0));
    }

    SECTION("Scalar division") {
        auto result = b / 2.0;
        CHECK(result == Point3D(2.0, 2.5, 3.0));
    }
}

TEST_CASE("Point3D - Distance calculations", "[geometry][point]") {
    Point3D a(0.0, 0.0, 0.0);
    Point3D b(3.0, 4.0, 0.0);

    SECTION("distanceTo calculates Euclidean distance") {
        CHECK_THAT(a.distanceTo(b), Catch::Matchers::WithinAbs(5.0, 1e-9));
    }

    SECTION("squaredDistanceTo avoids sqrt") {
        CHECK_THAT(a.squaredDistanceTo(b), Catch::Matchers::WithinAbs(25.0, 1e-9));
    }
}

TEST_CASE("Point3D - Approximate equality", "[geometry][point]") {
    Point3D a(1.0, 2.0, 3.0);
    Point3D b(1.0 + 1e-10, 2.0, 3.0);
    Point3D c(1.1, 2.0, 3.0);

    SECTION("isApprox with default tolerance") {
        CHECK(a.isApprox(b));
        CHECK_FALSE(a.isApprox(c));
    }

    SECTION("isApprox with custom tolerance") { CHECK(a.isApprox(c, 0.2)); }
}

TEST_CASE("Point3D - Linear interpolation", "[geometry][point]") {
    Point3D a(0.0, 0.0, 0.0);
    Point3D b(10.0, 10.0, 10.0);

    CHECK(a.lerp(b, 0.0) == a);
    CHECK(a.lerp(b, 1.0) == b);
    CHECK(a.lerp(b, 0.5) == Point3D(5.0, 5.0, 5.0));
}

// =============================================================================
// Vector3D Tests
// =============================================================================

TEST_CASE("Vector3D - Basic operations", "[geometry][vector]") {
    Vector3D v(3.0, 4.0, 0.0);

    SECTION("Length calculation") { CHECK_THAT(v.length(), Catch::Matchers::WithinAbs(5.0, 1e-9)); }

    SECTION("Squared length") {
        CHECK_THAT(v.squaredLength(), Catch::Matchers::WithinAbs(25.0, 1e-9));
    }

    SECTION("Normalization") {
        auto n = v.normalized();
        CHECK_THAT(n.length(), Catch::Matchers::WithinAbs(1.0, 1e-9));
        CHECK_THAT(n.m_x, Catch::Matchers::WithinAbs(0.6, 1e-9));
        CHECK_THAT(n.m_y, Catch::Matchers::WithinAbs(0.8, 1e-9));
    }
}

TEST_CASE("Vector3D - Dot and cross products", "[geometry][vector]") {
    Vector3D x = Vector3D::unitX();
    Vector3D y = Vector3D::unitY();
    Vector3D z = Vector3D::unitZ();

    SECTION("Dot product of orthogonal vectors is zero") {
        CHECK_THAT(x.dot(y), Catch::Matchers::WithinAbs(0.0, 1e-9));
        CHECK_THAT(y.dot(z), Catch::Matchers::WithinAbs(0.0, 1e-9));
    }

    SECTION("Dot product of parallel vectors") {
        CHECK_THAT(x.dot(x), Catch::Matchers::WithinAbs(1.0, 1e-9));
    }

    SECTION("Cross product of basis vectors") {
        auto cross_xy = x.cross(y);
        CHECK(cross_xy.isApprox(z));

        auto cross_yz = y.cross(z);
        CHECK(cross_yz.isApprox(x));
    }
}

TEST_CASE("Vector3D - Angle and parallelism", "[geometry][vector]") {
    Vector3D x = Vector3D::unitX();
    Vector3D y = Vector3D::unitY();

    SECTION("Angle between perpendicular vectors") {
        double angle = x.angleTo(y);
        CHECK_THAT(angle, Catch::Matchers::WithinAbs(M_PI / 2.0, 1e-9));
    }

    SECTION("Parallel detection") {
        Vector3D scaled_x(2.0, 0.0, 0.0);
        CHECK(x.isParallelTo(scaled_x));
        CHECK_FALSE(x.isParallelTo(y));
    }

    SECTION("Perpendicular detection") { CHECK(x.isPerpendicularTo(y)); }
}

// =============================================================================
// BoundingBox3D Tests
// =============================================================================

TEST_CASE("BoundingBox3D - Construction and validity", "[geometry][bbox]") {
    SECTION("Default constructor creates invalid box") {
        BoundingBox3D box;
        CHECK_FALSE(box.isValid());
    }

    SECTION("Valid box with proper min/max") {
        BoundingBox3D box(Point3D(0, 0, 0), Point3D(1, 1, 1));
        CHECK(box.isValid());
    }
}

TEST_CASE("BoundingBox3D - Expansion", "[geometry][bbox]") {
    BoundingBox3D box;

    box.expand(Point3D(1.0, 2.0, 3.0));
    box.expand(Point3D(-1.0, -2.0, -3.0));

    CHECK(box.isValid());
    CHECK(box.m_min == Point3D(-1.0, -2.0, -3.0));
    CHECK(box.m_max == Point3D(1.0, 2.0, 3.0));
}

TEST_CASE("BoundingBox3D - Queries", "[geometry][bbox]") {
    BoundingBox3D box(Point3D(0, 0, 0), Point3D(10, 10, 10));

    SECTION("Center calculation") {
        auto center = box.center();
        CHECK(center == Point3D(5.0, 5.0, 5.0));
    }

    SECTION("Contains point") {
        CHECK(box.contains(Point3D(5, 5, 5)));
        CHECK(box.contains(Point3D(0, 0, 0)));
        CHECK_FALSE(box.contains(Point3D(-1, 5, 5)));
    }

    SECTION("Intersection test") {
        BoundingBox3D other(Point3D(5, 5, 5), Point3D(15, 15, 15));
        CHECK(box.intersects(other));

        BoundingBox3D non_intersecting(Point3D(20, 20, 20), Point3D(30, 30, 30));
        CHECK_FALSE(box.intersects(non_intersecting));
    }
}

// =============================================================================
// Entity ID Generation Tests
// =============================================================================

TEST_CASE("Entity ID generation", "[geometry][entity]") {
    // Reset generators for predictable test results
    resetEntityIdGenerator();
    resetEntityUIDGenerator(EntityType::Vertex);
    resetEntityUIDGenerator(EntityType::Face);

    SECTION("Global IDs are sequential") {
        auto id1 = generateEntityId();
        auto id2 = generateEntityId();
        auto id3 = generateEntityId();

        CHECK(id1 == 1);
        CHECK(id2 == 2);
        CHECK(id3 == 3);
    }

    SECTION("Type-scoped UIDs are independent") {
        auto vertex_uid1 = generateEntityUID(EntityType::Vertex);
        auto vertex_uid2 = generateEntityUID(EntityType::Vertex);
        auto face_uid1 = generateEntityUID(EntityType::Face);

        CHECK(vertex_uid1 == 1);
        CHECK(vertex_uid2 == 2);
        CHECK(face_uid1 == 1); // Independent counter for faces
    }
}

// =============================================================================
// GeometryEntity Tests
// =============================================================================

TEST_CASE("GeometryEntity - Construction via factory", "[geometry][entity]") {
    resetEntityIdGenerator();
    resetEntityUIDGenerator(EntityType::Solid);
    resetEntityUIDGenerator(EntityType::Face);

    SECTION("Construction from OCC box shape") {
        BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
        TopoDS_Shape box_shape = make_box.Shape();

        auto entity = createEntityFromShape(box_shape);
        REQUIRE(entity != nullptr);
        CHECK(entity->hasShape());
        CHECK(entity->entityType() == EntityType::Solid);
        CHECK(std::string(entity->typeName()) == "Solid");
    }

    SECTION("Construction from null shape returns nullptr") {
        TopoDS_Shape null_shape;
        auto entity = createEntityFromShape(null_shape);
        CHECK(entity == nullptr);
    }

    SECTION("Type-specific entity creation") {
        BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
        TopoDS_Solid solid = TopoDS::Solid(make_box.Shape());

        auto solid_entity = std::make_shared<SolidEntity>(solid);
        CHECK(solid_entity->entityType() == EntityType::Solid);
        CHECK(solid_entity->volume() > 0);
    }
}

TEST_CASE("GeometryEntity - Hierarchy", "[geometry][entity]") {
    // Use factory to create entities
    BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
    auto parent = createEntityFromShape(make_box.Shape());
    parent->setName("Parent");

    BRepPrimAPI_MakeBox make_box2(5.0, 5.0, 5.0);
    auto child1 = createEntityFromShape(make_box2.Shape());
    child1->setName("Child1");

    BRepPrimAPI_MakeBox make_box3(3.0, 3.0, 3.0);
    auto child2 = createEntityFromShape(make_box3.Shape());
    child2->setName("Child2");

    SECTION("Adding children") {
        parent->addChild(child1);
        parent->addChild(child2);

        CHECK(parent->childCount() == 2);
        CHECK(parent->hasChildren());
        CHECK(child1->parent().lock() == parent);
    }

    SECTION("Removing children") {
        parent->addChild(child1);
        parent->addChild(child2);

        bool removed = parent->removeChild(child1);
        CHECK(removed);
        CHECK(parent->childCount() == 1);
        CHECK(child1->parent().expired());
    }

    SECTION("Root detection") {
        parent->addChild(child1);

        CHECK(parent->isRoot());
        CHECK_FALSE(child1->isRoot());
    }
}

// =============================================================================
// EntityIndex Tests
// =============================================================================

TEST_CASE("EntityIndex - Indexing and lookup", "[geometry][index]") {
    resetEntityIdGenerator();
    resetEntityUIDGenerator(EntityType::Solid);

    EntityIndex index;

    BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
    auto entity = createEntityFromShape(make_box.Shape());
    entity->setName("TestBox");

    index.addEntity(entity);

    SECTION("Find by ID") {
        auto found = index.findById(entity->entityId());
        CHECK(found == entity);

        auto not_found = index.findById(999);
        CHECK(not_found == nullptr);
    }

    SECTION("Find by type and UID") {
        auto found = index.findByTypeAndUID(EntityType::Solid, entity->entityUID());
        CHECK(found == entity);
    }

    SECTION("Find by shape") {
        auto found = index.findByShape(entity->shape());
        CHECK(found == entity);
    }

    SECTION("Get entities by type") {
        auto solids = index.getEntitiesByType(EntityType::Solid);
        REQUIRE(solids.size() == 1);
        CHECK(solids[0] == entity);

        auto faces = index.getEntitiesByType(EntityType::Face);
        CHECK(faces.empty());
    }

    SECTION("Entity count") {
        CHECK(index.entityCount() == 1);
        CHECK(index.entityCountByType(EntityType::Solid) == 1);
    }

    SECTION("Remove entity") {
        bool removed = index.removeEntity(entity);
        CHECK(removed);
        CHECK(index.entityCount() == 0);
        CHECK(index.findById(entity->entityId()) == nullptr);
    }
}

// =============================================================================
// GeometryDocument Tests
// =============================================================================

TEST_CASE("GeometryDocument - Basic operations", "[geometry][document]") {
    auto doc = std::make_shared<GeometryDocument>();

    SECTION("Initial state") {
        CHECK(doc->name() == "Untitled");
        CHECK_FALSE(doc->isModified());
        CHECK(doc->rootEntities().empty());
    }

    SECTION("Register and unregister entities") {
        BRepPrimAPI_MakeBox make_box(5.0, 5.0, 5.0);
        auto entity = createEntityFromShape(make_box.Shape());
        doc->registerEntity(entity);

        CHECK(doc->isModified());
        CHECK(doc->index().entityCount() == 1);
        CHECK(doc->findEntityById(entity->entityId()) == entity);

        doc->unregisterEntity(entity);
        CHECK(doc->findEntityById(entity->entityId()) == nullptr);
    }
}

TEST_CASE("GeometryDocument - Topology hierarchy building", "[geometry][document]") {
    resetEntityIdGenerator();

    auto doc = std::make_shared<GeometryDocument>();

    // Create a simple box shape
    BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
    TopoDS_Shape box_shape = make_box.Shape();

    SECTION("Build hierarchy from box") {
        auto root = doc->buildTopologyHierarchy(box_shape, "TestBox");

        REQUIRE(root != nullptr);
        CHECK(root->name() == "TestBox");
        CHECK(root->entityType() == EntityType::Solid);
        CHECK(root->hasChildren());

        // Box should have: 1 solid, shells, 6 faces, edges, vertices
        CHECK(doc->index().entityCount() > 1);

        // Check faces are created
        auto faces = doc->index().getEntitiesByType(EntityType::Face);
        CHECK(faces.size() == 6); // Box has 6 faces
    }

    SECTION("Clear document") {
        [[maybe_unused]] auto root = doc->buildTopologyHierarchy(box_shape, "TestBox");
        CHECK(doc->index().entityCount() > 0);

        doc->clear();
        CHECK(doc->index().entityCount() == 0);
        CHECK(doc->rootEntities().empty());
    }
}

TEST_CASE("GeometryDocument - Shape lookup", "[geometry][document]") {
    auto doc = std::make_shared<GeometryDocument>();

    BRepPrimAPI_MakeSphere make_sphere(5.0);
    TopoDS_Shape sphere = make_sphere.Shape();

    auto root = doc->buildTopologyHierarchy(sphere, "Sphere");

    SECTION("Find entity by shape") {
        auto found = doc->findEntityByShape(sphere);
        CHECK(found == root);
    }
}

// =============================================================================
// GeometryManager Tests
// =============================================================================

TEST_CASE("GeometryManager - Singleton and entity lookup", "[geometry][manager]") {
    // Clear manager state before test
    GeometryManager::instance().clear();

    resetEntityIdGenerator();
    resetEntityUIDGenerator(EntityType::Solid);
    resetEntityUIDGenerator(EntityType::Face);

    SECTION("Singleton access") {
        auto& mgr1 = GeometryManager::instance();
        auto& mgr2 = GeometryManager::instance();
        CHECK(&mgr1 == &mgr2);
    }

    SECTION("Build and lookup via manager") {
        auto& mgr = GeometryManager::instance();

        BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
        auto root = mgr.buildTopologyHierarchy(make_box.Shape(), "TestBox");

        REQUIRE(root != nullptr);

        // Lookup by ID
        auto foundById = mgr.findById(root->entityId());
        CHECK(foundById == root);

        // Lookup by type and UID
        auto foundByTypeUID = mgr.findByTypeAndUID(EntityType::Solid, root->entityUID());
        CHECK(foundByTypeUID == root);

        // Lookup by shape
        auto foundByShape = mgr.findByShape(make_box.Shape());
        CHECK(foundByShape == root);

        // Entity counts
        CHECK(mgr.entityCount() > 0);
        CHECK(mgr.entityCountByType(EntityType::Face) == 6);
    }

    SECTION("Document management") {
        auto& mgr = GeometryManager::instance();

        auto doc = mgr.createDocument("TestDoc");
        REQUIRE(doc != nullptr);
        CHECK(doc->name() == "TestDoc");
        CHECK(mgr.activeDocument() == doc);
        CHECK(mgr.documents().size() == 1);
    }
}

// =============================================================================
// Derived Entity Methods Tests
// =============================================================================

TEST_CASE("EdgeEntity - Geometry queries", "[geometry][edge]") {
    BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
    TopoDS_Shape box_shape = make_box.Shape();

    auto doc = std::make_shared<GeometryDocument>();
    auto root = doc->buildTopologyHierarchy(box_shape, "Box");

    // Get edges from the document
    auto edges = doc->index().getEntitiesByType(EntityType::Edge);
    REQUIRE(!edges.empty());

    auto edgeEntity = entityAs<EdgeEntity>(edges[0]);
    REQUIRE(edgeEntity != nullptr);

    SECTION("Edge length") {
        double len = edgeEntity->length();
        CHECK(len > 0);
    }

    SECTION("Start and end points") {
        Point3D start = edgeEntity->startPoint();
        Point3D end = edgeEntity->endPoint();
        Point3D mid = edgeEntity->midPoint();

        // Mid should be between start and end
        CHECK(mid.m_x >= std::min(start.m_x, end.m_x) - 1e-6);
        CHECK(mid.m_x <= std::max(start.m_x, end.m_x) + 1e-6);
    }

    SECTION("Tangent at parameter") {
        double first, last;
        edgeEntity->parameterRange(first, last);

        Vector3D tangent = edgeEntity->tangentAt((first + last) / 2.0);
        // Tangent should be a unit vector (approximately)
        double len = tangent.length();
        if(len > 0) {
            CHECK_THAT(len, Catch::Matchers::WithinAbs(1.0, 0.01));
        }
    }

    SECTION("Distance to point") {
        Point3D pt(100.0, 100.0, 100.0);
        double dist = edgeEntity->distanceTo(pt);
        CHECK(dist > 0);
    }

    SECTION("Closest point to query") {
        Point3D pt(5.0, 5.0, 100.0);
        Point3D closest = edgeEntity->closestPointTo(pt);
        // Closest point should be on the box surface
        CHECK(closest.m_z <= 10.0 + 1e-6);
    }
}

TEST_CASE("SolidEntity - Geometry queries", "[geometry][solid]") {
    BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);

    auto solidEntity = std::make_shared<SolidEntity>(TopoDS::Solid(make_box.Shape()));

    SECTION("Volume calculation") {
        double vol = solidEntity->volume();
        CHECK_THAT(vol, Catch::Matchers::WithinAbs(1000.0, 1e-6));
    }

    SECTION("Surface area calculation") {
        double area = solidEntity->surfaceArea();
        CHECK_THAT(area, Catch::Matchers::WithinAbs(600.0, 1e-6));
    }

    SECTION("Center of mass") {
        Point3D center = solidEntity->centerOfMass();
        CHECK_THAT(center.m_x, Catch::Matchers::WithinAbs(5.0, 1e-6));
        CHECK_THAT(center.m_y, Catch::Matchers::WithinAbs(5.0, 1e-6));
        CHECK_THAT(center.m_z, Catch::Matchers::WithinAbs(5.0, 1e-6));
    }

    SECTION("Topology counts") {
        CHECK(solidEntity->faceCount() == 6);
        CHECK(solidEntity->edgeCount() == 12);
        CHECK(solidEntity->vertexCount() == 8);
    }
}

TEST_CASE("FaceEntity - Geometry queries", "[geometry][face]") {
    BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
    TopoDS_Shape box_shape = make_box.Shape();

    auto doc = std::make_shared<GeometryDocument>();
    auto root = doc->buildTopologyHierarchy(box_shape, "Box");

    auto faces = doc->index().getEntitiesByType(EntityType::Face);
    REQUIRE(!faces.empty());

    auto faceEntity = entityAs<FaceEntity>(faces[0]);
    REQUIRE(faceEntity != nullptr);

    SECTION("Face area") {
        double faceArea = faceEntity->area();
        CHECK(faceArea > 0);
        // Each face of 10x10x10 box should be 100 sq units
        CHECK_THAT(faceArea, Catch::Matchers::WithinAbs(100.0, 1e-6));
    }

    SECTION("Normal at center") {
        double uMin, uMax, vMin, vMax;
        faceEntity->parameterBounds(uMin, uMax, vMin, vMax);

        double uMid = (uMin + uMax) / 2.0;
        double vMid = (vMin + vMax) / 2.0;

        Vector3D normal = faceEntity->normalAt(uMid, vMid);
        double normLen = normal.length();
        if(normLen > 0) {
            CHECK_THAT(normLen, Catch::Matchers::WithinAbs(1.0, 0.01));
        }
    }

    SECTION("Hole count") {
        // Box faces have no holes
        CHECK(faceEntity->holeCount() == 0);
    }
}

TEST_CASE("VertexEntity - Geometry queries", "[geometry][vertex]") {
    BRepPrimAPI_MakeBox make_box(10.0, 10.0, 10.0);
    TopoDS_Shape box_shape = make_box.Shape();

    auto doc = std::make_shared<GeometryDocument>();
    auto root = doc->buildTopologyHierarchy(box_shape, "Box");

    auto vertices = doc->index().getEntitiesByType(EntityType::Vertex);
    REQUIRE(!vertices.empty());

    auto vertexEntity = entityAs<VertexEntity>(vertices[0]);
    REQUIRE(vertexEntity != nullptr);

    SECTION("Point location") {
        Point3D pt = vertexEntity->point();
        // Point should be on box corner
        bool onCorner = (pt.m_x == 0.0 || pt.m_x == 10.0) && (pt.m_y == 0.0 || pt.m_y == 10.0) &&
                        (pt.m_z == 0.0 || pt.m_z == 10.0);
        CHECK(onCorner);
    }

    SECTION("Distance to another point") {
        Point3D farPoint(100.0, 100.0, 100.0);
        double dist = vertexEntity->distanceTo(farPoint);
        CHECK(dist > 0);
    }
}
