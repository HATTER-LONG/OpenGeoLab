/**
 * @file topology_utils_test.cpp
 * @brief Unit tests for topology extraction utilities
 */

#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/geometry/topology_utils.hpp>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <TopoDS.hxx>

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>

using OpenGeoLab::Geometry::EdgeInfo;
using OpenGeoLab::Geometry::FaceInfo;
using OpenGeoLab::Geometry::ShapeStore;
using OpenGeoLab::Geometry::VertexInfo;

static TopoDS_Shape makeBox(double w = 10.0, double h = 10.0, double d = 10.0) {
    return BRepPrimAPI_MakeBox(w, h, d).Shape();
}

static TopoDS_Shape makeCylinder(double r = 5.0, double h = 10.0) {
    return BRepPrimAPI_MakeCylinder(r, h).Shape();
}

TEST_SUITE("topology_utils") {

    TEST_CASE("extractFaceInfo returns plane for box face") {
        ShapeStore store;
        auto id = store.add("Box", makeBox());
        const auto* entry = store.find(id);
        REQUIRE(entry != nullptr);
        REQUIRE(entry->faceMap.Extent() == 6);

        const auto& face = TopoDS::Face(entry->faceMap(1));
        auto info = OpenGeoLab::Geometry::extractFaceInfo(1, face);
        CHECK(info.localId == 1);
        CHECK(info.surfaceType == "plane");
        CHECK(info.area == doctest::Approx(100.0).epsilon(0.01));
        CHECK_FALSE(info.axis.has_value());
        CHECK_FALSE(info.radius.has_value());
    }

    TEST_CASE("extractFaceInfo returns cylinder for cylinder lateral face") {
        ShapeStore store;
        auto id = store.add("Cyl", makeCylinder(5.0, 10.0));
        const auto* entry = store.find(id);
        REQUIRE(entry != nullptr);

        bool found_cylinder = false;
        for(int i = 1; i <= entry->faceMap.Extent(); ++i) {
            const auto& face = TopoDS::Face(entry->faceMap(i));
            auto info = OpenGeoLab::Geometry::extractFaceInfo(static_cast<uint32_t>(i), face);
            if(info.surfaceType == "cylinder") {
                found_cylinder = true;
                CHECK(info.radius.has_value());
                CHECK(info.radius.value() == doctest::Approx(5.0).epsilon(0.01));
                CHECK(info.axis.has_value());
                CHECK(info.area == doctest::Approx(2.0 * M_PI * 5.0 * 10.0).epsilon(0.5));
                break;
            }
        }
        CHECK(found_cylinder);
    }

    TEST_CASE("extractEdgeInfo returns line for box edge") {
        ShapeStore store;
        auto id = store.add("Box", makeBox());
        const auto* entry = store.find(id);
        REQUIRE(entry != nullptr);
        REQUIRE(entry->edgeMap.Extent() == 12);

        const auto& edge = TopoDS::Edge(entry->edgeMap(1));
        auto info = OpenGeoLab::Geometry::extractEdgeInfo(1, edge);
        CHECK(info.localId == 1);
        CHECK(info.curveType == "line");
        CHECK(info.length == doctest::Approx(10.0).epsilon(0.01));
        CHECK_FALSE(info.center.has_value());
        CHECK_FALSE(info.radius.has_value());
    }

    TEST_CASE("extractEdgeInfo returns circle for cylinder edge") {
        ShapeStore store;
        auto id = store.add("Cyl", makeCylinder(5.0, 10.0));
        const auto* entry = store.find(id);
        REQUIRE(entry != nullptr);

        bool found_circle = false;
        for(int i = 1; i <= entry->edgeMap.Extent(); ++i) {
            const auto& edge = TopoDS::Edge(entry->edgeMap(i));
            auto info = OpenGeoLab::Geometry::extractEdgeInfo(static_cast<uint32_t>(i), edge);
            if(info.curveType == "circle") {
                found_circle = true;
                CHECK(info.radius.has_value());
                CHECK(info.radius.value() == doctest::Approx(5.0).epsilon(0.01));
                CHECK(info.center.has_value());
                CHECK(info.length == doctest::Approx(2.0 * M_PI * 5.0).epsilon(0.1));
                break;
            }
        }
        CHECK(found_circle);
    }

    TEST_CASE("extractVertexInfo returns vertex position") {
        ShapeStore store;
        auto id = store.add("Box", makeBox(10.0, 20.0, 30.0));
        const auto* entry = store.find(id);
        REQUIRE(entry != nullptr);
        REQUIRE(entry->vertexMap.Extent() == 8);

        for(int i = 1; i <= entry->vertexMap.Extent(); ++i) {
            const auto& vtx = TopoDS::Vertex(entry->vertexMap(i));
            auto info = OpenGeoLab::Geometry::extractVertexInfo(static_cast<uint32_t>(i), vtx);
            CHECK(info.localId == static_cast<uint32_t>(i));
            CHECK((info.position[0] == doctest::Approx(0.0) ||
                   info.position[0] == doctest::Approx(10.0)));
            CHECK((info.position[1] == doctest::Approx(0.0) ||
                   info.position[1] == doctest::Approx(20.0)));
            CHECK((info.position[2] == doctest::Approx(0.0) ||
                   info.position[2] == doctest::Approx(30.0)));
        }
    }

    TEST_CASE("toJson(FaceInfo) produces expected keys") {
        OpenGeoLab::Geometry::FaceInfo info;
        info.localId = 3;
        info.surfaceType = "plane";
        info.center = {1.0, 2.0, 3.0};
        info.normal = {0.0, 0.0, 1.0};
        info.area = 42.0;
        auto j = OpenGeoLab::Geometry::toJson(info);
        CHECK(j["localId"] == 3);
        CHECK(j["surfaceType"] == "plane");
        CHECK(j["area"] == doctest::Approx(42.0));
        CHECK(j["center"].is_array());
        CHECK(j["normal"].is_array());
        CHECK_FALSE(j.contains("axis"));
        CHECK_FALSE(j.contains("radius"));
    }

    TEST_CASE("toJson(FaceInfo) includes axis and radius when present") {
        OpenGeoLab::Geometry::FaceInfo info;
        info.localId = 1;
        info.surfaceType = "cylinder";
        info.center = {0, 0, 0};
        info.normal = {1, 0, 0};
        info.axis = std::array<double, 3>{0.0, 0.0, 1.0};
        info.radius = 5.0;
        info.area = 100.0;
        auto j = OpenGeoLab::Geometry::toJson(info);
        CHECK(j.contains("axis"));
        CHECK(j.contains("radius"));
        CHECK(j["radius"] == doctest::Approx(5.0));
    }

    TEST_CASE("toJson(EdgeInfo) produces expected keys") {
        OpenGeoLab::Geometry::EdgeInfo info;
        info.localId = 2;
        info.curveType = "line";
        info.start = {0, 0, 0};
        info.end = {10, 0, 0};
        info.length = 10.0;
        auto j = OpenGeoLab::Geometry::toJson(info);
        CHECK(j["localId"] == 2);
        CHECK(j["curveType"] == "line");
        CHECK(j["length"] == doctest::Approx(10.0));
        CHECK_FALSE(j.contains("center"));
        CHECK_FALSE(j.contains("radius"));
    }

    TEST_CASE("toJson(VertexInfo) produces expected keys") {
        OpenGeoLab::Geometry::VertexInfo info;
        info.localId = 5;
        info.position = {1.0, 2.0, 3.0};
        auto j = OpenGeoLab::Geometry::toJson(info);
        CHECK(j["localId"] == 5);
        CHECK(j["position"][0] == doctest::Approx(1.0));
        CHECK(j["position"][1] == doctest::Approx(2.0));
        CHECK(j["position"][2] == doctest::Approx(3.0));
    }

    TEST_CASE("buildEdgeToFaceAdjacency for a box") {
        ShapeStore store;
        auto id = store.add("Box", makeBox());
        const auto* entry = store.find(id);
        REQUIRE(entry != nullptr);

        auto adj = OpenGeoLab::Geometry::buildEdgeToFaceAdjacency(*entry);
        CHECK(adj.size() == 12);
        for(const auto& [edgeId, faceIds] : adj) {
            CHECK(faceIds.size() == 2);
        }
    }

    TEST_CASE("buildVertexToEdgeAdjacency for a box") {
        ShapeStore store;
        auto id = store.add("Box", makeBox());
        const auto* entry = store.find(id);
        REQUIRE(entry != nullptr);

        auto adj = OpenGeoLab::Geometry::buildVertexToEdgeAdjacency(*entry);
        CHECK(adj.size() == 8);
        for(const auto& [vtxId, edgeIds] : adj) {
            CHECK(edgeIds.size() == 3);
        }
    }

    TEST_CASE("buildFaceToEdgeAdjacency for a box") {
        ShapeStore store;
        auto id = store.add("Box", makeBox());
        const auto* entry = store.find(id);
        REQUIRE(entry != nullptr);

        auto adj = OpenGeoLab::Geometry::buildFaceToEdgeAdjacency(*entry);
        CHECK(adj.size() == 6);
        for(const auto& [faceId, edgeIds] : adj) {
            CHECK(edgeIds.size() == 4);
        }
    }

    TEST_CASE("computeSubShapeBounds for a box face") {
        ShapeStore store;
        auto id = store.add("Box", makeBox(10.0, 20.0, 30.0));
        const auto* entry = store.find(id);
        REQUIRE(entry != nullptr);

        const auto& face = entry->faceMap(1);
        auto bounds = OpenGeoLab::Geometry::computeSubShapeBounds(face);
        REQUIRE(bounds.has_value());
        const auto& [mn, mx] = *bounds;
        for(int i = 0; i < 3; ++i) {
            CHECK(mn[i] >= doctest::Approx(-0.01));
        }
        CHECK(mx[0] <= doctest::Approx(10.01));
        CHECK(mx[1] <= doctest::Approx(20.01));
        CHECK(mx[2] <= doctest::Approx(30.01));
    }

} // TEST_SUITE
