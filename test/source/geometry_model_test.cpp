/**
 * @file geometry_model_test.cpp
 * @brief Unit tests for GeometryModel and GeometryStore.
 */

#include <catch2/catch_test_macros.hpp>

#include "geometry/geometry_model.hpp"
#include "geometry/geometry_types.hpp"

#include <atomic>
#include <thread>

namespace OpenGeoLab::Geometry {
namespace {

TEST_CASE("GeometryModel - basic operations", "[geometry][model]") {
    GeometryModel model;

    SECTION("isEmpty returns true for new model") { CHECK(model.isEmpty()); }

    SECTION("addPart and getParts work correctly") {
        Part part;
        part.m_id = 1;
        part.m_name = "TestPart";
        part.m_solidIds = {1, 2};

        model.addPart(part);

        CHECK_FALSE(model.isEmpty());
        CHECK(model.partCount() == 1);

        const auto& parts = model.getParts();
        REQUIRE(parts.size() == 1);
        CHECK(parts[0].m_id == 1);
        CHECK(parts[0].m_name == "TestPart");
    }

    SECTION("addSolid and getSolidById work correctly") {
        Solid solid;
        solid.m_id = 42;
        solid.m_faceIds = {1, 2, 3};

        model.addSolid(solid);

        CHECK(model.solidCount() == 1);

        const Solid* found = model.getSolidById(42);
        REQUIRE(found != nullptr);
        CHECK(found->m_id == 42);
        CHECK(found->m_faceIds.size() == 3);

        // Non-existent ID
        CHECK(model.getSolidById(999) == nullptr);
    }

    SECTION("addFace and getFaceById work correctly") {
        Face face;
        face.m_id = 10;
        face.m_edgeIds = {1, 2, 3, 4};

        model.addFace(face);

        CHECK(model.faceCount() == 1);

        const Face* found = model.getFaceById(10);
        REQUIRE(found != nullptr);
        CHECK(found->m_id == 10);
    }

    SECTION("addEdge and getEdgeById work correctly") {
        Edge edge;
        edge.m_id = 5;
        edge.m_startVertexId = 1;
        edge.m_endVertexId = 2;

        model.addEdge(edge);

        CHECK(model.edgeCount() == 1);

        const Edge* found = model.getEdgeById(5);
        REQUIRE(found != nullptr);
        CHECK(found->m_startVertexId == 1);
        CHECK(found->m_endVertexId == 2);
    }

    SECTION("addVertex and getVertexById work correctly") {
        Vertex vertex;
        vertex.m_id = 100;
        vertex.m_position = Point3D(1.0, 2.0, 3.0);

        model.addVertex(vertex);

        CHECK(model.vertexCount() == 1);

        const Vertex* found = model.getVertexById(100);
        REQUIRE(found != nullptr);
        CHECK(found->m_position.m_x == 1.0);
        CHECK(found->m_position.m_y == 2.0);
        CHECK(found->m_position.m_z == 3.0);
    }

    SECTION("clear removes all data") {
        Part part;
        part.m_id = 1;
        model.addPart(part);

        Solid solid;
        solid.m_id = 1;
        model.addSolid(solid);

        model.m_sourcePath = "test.brep";

        model.clear();

        CHECK(model.isEmpty());
        CHECK(model.partCount() == 0);
        CHECK(model.solidCount() == 0);
        CHECK(model.m_sourcePath.empty());
    }

    SECTION("generateNextId returns unique IDs") {
        uint32_t id1 = model.generateNextId();
        uint32_t id2 = model.generateNextId();
        uint32_t id3 = model.generateNextId();

        CHECK(id1 != id2);
        CHECK(id2 != id3);
        CHECK(id1 != id3);
        CHECK(id1 == 1);
        CHECK(id2 == 2);
        CHECK(id3 == 3);
    }

    SECTION("getSummary returns formatted string") {
        Part part;
        part.m_id = 1;
        model.addPart(part);

        Solid solid;
        solid.m_id = 1;
        model.addSolid(solid);

        Face face;
        face.m_id = 1;
        model.addFace(face);

        std::string summary = model.getSummary();

        CHECK(summary.find("Parts: 1") != std::string::npos);
        CHECK(summary.find("Solids: 1") != std::string::npos);
        CHECK(summary.find("Faces: 1") != std::string::npos);
    }
}

TEST_CASE("GeometryModel - bounding box computation", "[geometry][model]") {
    GeometryModel model;

    SECTION("empty model returns invalid bounding box") {
        BoundingBox box = model.computeBoundingBox();
        // Min should be greater than max for empty model
        CHECK(box.m_min.m_x > box.m_max.m_x);
    }

    SECTION("bounding box computed from vertices") {
        Vertex v1;
        v1.m_id = 1;
        v1.m_position = Point3D(0.0, 0.0, 0.0);
        model.addVertex(v1);

        Vertex v2;
        v2.m_id = 2;
        v2.m_position = Point3D(10.0, 20.0, 30.0);
        model.addVertex(v2);

        Vertex v3;
        v3.m_id = 3;
        v3.m_position = Point3D(-5.0, 5.0, 15.0);
        model.addVertex(v3);

        BoundingBox box = model.computeBoundingBox();

        CHECK(box.m_min.m_x == -5.0);
        CHECK(box.m_min.m_y == 0.0);
        CHECK(box.m_min.m_z == 0.0);
        CHECK(box.m_max.m_x == 10.0);
        CHECK(box.m_max.m_y == 20.0);
        CHECK(box.m_max.m_z == 30.0);
    }
}

TEST_CASE("GeometryStore - singleton and basic operations", "[geometry][store]") {
    // Clean up any previous state
    GeometryStore::instance().clear();

    SECTION("instance returns same object") {
        GeometryStore& store1 = GeometryStore::instance();
        GeometryStore& store2 = GeometryStore::instance();

        CHECK(&store1 == &store2);
    }

    SECTION("setModel and getModel work correctly") {
        auto model = std::make_shared<GeometryModel>();
        Part part;
        part.m_id = 1;
        part.m_name = "TestPart";
        model->addPart(part);

        GeometryStore::instance().setModel(model);

        auto retrieved = GeometryStore::instance().getModel();
        REQUIRE(retrieved != nullptr);
        CHECK(retrieved->partCount() == 1);
        CHECK(retrieved->getParts()[0].m_name == "TestPart");
    }

    SECTION("hasModel returns correct state") {
        GeometryStore::instance().clear();

        CHECK_FALSE(GeometryStore::instance().hasModel());

        auto model = std::make_shared<GeometryModel>();
        Part part;
        part.m_id = 1;
        model->addPart(part);

        GeometryStore::instance().setModel(model);
        CHECK(GeometryStore::instance().hasModel());

        GeometryStore::instance().clear();
        CHECK_FALSE(GeometryStore::instance().hasModel());
    }
}

TEST_CASE("GeometryStore - change notifications", "[geometry][store][callback]") {
    GeometryStore::instance().clear();

    SECTION("callback is invoked on setModel") {
        std::atomic<int> callCount{0};

        size_t callbackId =
            GeometryStore::instance().registerChangeCallback([&callCount]() { callCount++; });

        auto model = std::make_shared<GeometryModel>();
        GeometryStore::instance().setModel(model);

        CHECK(callCount == 1);

        GeometryStore::instance().unregisterChangeCallback(callbackId);
    }

    SECTION("callback is invoked on clear") {
        std::atomic<int> callCount{0};

        size_t callbackId =
            GeometryStore::instance().registerChangeCallback([&callCount]() { callCount++; });

        GeometryStore::instance().clear();

        CHECK(callCount == 1);

        GeometryStore::instance().unregisterChangeCallback(callbackId);
    }

    SECTION("unregistered callback is not invoked") {
        std::atomic<int> callCount{0};

        size_t callbackId =
            GeometryStore::instance().registerChangeCallback([&callCount]() { callCount++; });

        GeometryStore::instance().unregisterChangeCallback(callbackId);

        auto model = std::make_shared<GeometryModel>();
        GeometryStore::instance().setModel(model);

        CHECK(callCount == 0);
    }

    SECTION("multiple callbacks can be registered") {
        std::atomic<int> count1{0};
        std::atomic<int> count2{0};

        size_t id1 = GeometryStore::instance().registerChangeCallback([&count1]() { count1++; });
        size_t id2 = GeometryStore::instance().registerChangeCallback([&count2]() { count2++; });

        auto model = std::make_shared<GeometryModel>();
        GeometryStore::instance().setModel(model);

        CHECK(count1 == 1);
        CHECK(count2 == 1);

        GeometryStore::instance().unregisterChangeCallback(id1);
        GeometryStore::instance().unregisterChangeCallback(id2);
    }

    SECTION("callback IDs are unique") {
        size_t id1 = GeometryStore::instance().registerChangeCallback([]() {});
        size_t id2 = GeometryStore::instance().registerChangeCallback([]() {});
        size_t id3 = GeometryStore::instance().registerChangeCallback([]() {});

        CHECK(id1 != id2);
        CHECK(id2 != id3);
        CHECK(id1 != id3);

        GeometryStore::instance().unregisterChangeCallback(id1);
        GeometryStore::instance().unregisterChangeCallback(id2);
        GeometryStore::instance().unregisterChangeCallback(id3);
    }
}

TEST_CASE("BoundingBox - utility methods", "[geometry][types]") {
    SECTION("isValid returns correct result") {
        BoundingBox valid;
        valid.m_min = Point3D(0, 0, 0);
        valid.m_max = Point3D(10, 10, 10);
        CHECK(valid.isValid());

        BoundingBox invalid;
        invalid.m_min = Point3D(10, 10, 10);
        invalid.m_max = Point3D(0, 0, 0);
        CHECK_FALSE(invalid.isValid());

        BoundingBox point;
        point.m_min = Point3D(5, 5, 5);
        point.m_max = Point3D(5, 5, 5);
        CHECK(point.isValid());
    }

    SECTION("center returns correct point") {
        BoundingBox box;
        box.m_min = Point3D(0, 0, 0);
        box.m_max = Point3D(10, 20, 30);

        Point3D center = box.center();

        CHECK(center.m_x == 5.0);
        CHECK(center.m_y == 10.0);
        CHECK(center.m_z == 15.0);
    }
}

} // namespace
} // namespace OpenGeoLab::Geometry
