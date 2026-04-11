/**
 * @file query_entity_info_action_test.cpp
 * @brief Unit tests for QueryEntityInfoAction
 */

#include <opengeolab/geometry/query_entity_info_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>

#include <doctest/doctest.h>

using OpenGeoLab::Geometry::QueryEntityInfoAction;
using OpenGeoLab::Geometry::ShapeStore;

static TopoDS_Shape makeBox(double w = 10.0, double h = 10.0, double d = 10.0) {
    return BRepPrimAPI_MakeBox(w, h, d).Shape();
}

TEST_SUITE("QueryEntityInfoAction") {

    TEST_CASE("describe returns expected schema") {
        ShapeStore store;
        QueryEntityInfoAction action(store);
        auto desc = action.describe();
        CHECK(desc["name"] == "query_entity_info");
        CHECK(desc["params"].contains("shapeId"));
        CHECK(desc["params"].contains("entityType"));
        CHECK(desc["params"].contains("localId"));
    }

    TEST_CASE("query face returns face info with adjacency") {
        ShapeStore store;
        auto id = store.add("Box", makeBox());
        QueryEntityInfoAction action(store);

        auto result =
            action.execute({{"shapeId", id}, {"entityType", "face"}, {"localId", 1}}, nullptr);
        CHECK(result["ok"] == true);
        CHECK(result["action"] == "query_entity_info");
        CHECK(result["shapeId"] == id);
        CHECK(result["entityType"] == "face");
        CHECK(result["localId"] == 1);
        CHECK(result["surfaceType"] == "plane");
        CHECK(result.contains("center"));
        CHECK(result.contains("normal"));
        CHECK(result.contains("area"));
        CHECK(result.contains("boundingBox"));

        // Adjacency: a box face has 4 edges
        CHECK(result.contains("adjacentEdges"));
        CHECK(result["adjacentEdges"].size() == 4);
        // Adjacent faces: each box face shares edges with 4 other faces
        CHECK(result.contains("adjacentFaces"));
        CHECK(result["adjacentFaces"].size() == 4);
    }

    TEST_CASE("query edge returns edge info with adjacency") {
        ShapeStore store;
        auto id = store.add("Box", makeBox());
        QueryEntityInfoAction action(store);

        auto result =
            action.execute({{"shapeId", id}, {"entityType", "edge"}, {"localId", 1}}, nullptr);
        CHECK(result["ok"] == true);
        CHECK(result["entityType"] == "edge");
        CHECK(result["curveType"] == "line");
        CHECK(result.contains("start"));
        CHECK(result.contains("end"));
        CHECK(result.contains("length"));
        CHECK(result.contains("boundingBox"));

        // A box edge is shared by exactly 2 faces
        CHECK(result.contains("adjacentFaces"));
        CHECK(result["adjacentFaces"].size() == 2);
    }

    TEST_CASE("query vertex returns vertex info with adjacency") {
        ShapeStore store;
        auto id = store.add("Box", makeBox());
        QueryEntityInfoAction action(store);

        auto result =
            action.execute({{"shapeId", id}, {"entityType", "vertex"}, {"localId", 1}}, nullptr);
        CHECK(result["ok"] == true);
        CHECK(result["entityType"] == "vertex");
        CHECK(result.contains("position"));

        // A box vertex touches exactly 3 edges
        CHECK(result.contains("adjacentEdges"));
        CHECK(result["adjacentEdges"].size() == 3);
    }

    TEST_CASE("query with unknown shapeId returns error") {
        ShapeStore store;
        QueryEntityInfoAction action(store);
        auto result =
            action.execute({{"shapeId", 999}, {"entityType", "face"}, {"localId", 1}}, nullptr);
        CHECK(result["ok"] == false);
    }

    TEST_CASE("query with out-of-range localId returns error") {
        ShapeStore store;
        auto id = store.add("Box", makeBox());
        QueryEntityInfoAction action(store);

        auto result =
            action.execute({{"shapeId", id}, {"entityType", "face"}, {"localId", 999}}, nullptr);
        CHECK(result["ok"] == false);
    }

    TEST_CASE("query with invalid entityType returns error") {
        ShapeStore store;
        auto id = store.add("Box", makeBox());
        QueryEntityInfoAction action(store);

        auto result =
            action.execute({{"shapeId", id}, {"entityType", "unknown"}, {"localId", 1}}, nullptr);
        CHECK(result["ok"] == false);
    }

    TEST_CASE("query with missing required params returns error") {
        ShapeStore store;
        QueryEntityInfoAction action(store);

        CHECK(action.execute(nlohmann::json::object(), nullptr)["ok"] == false);
        CHECK(action.execute({{"shapeId", 0}}, nullptr)["ok"] == false);
        CHECK(action.execute({{"shapeId", 0}, {"entityType", "face"}}, nullptr)["ok"] == false);
    }

} // TEST_SUITE
