/**
 * @file describe_topology_action_test.cpp
 * @brief Unit tests for DescribeTopologyAction
 */

#include <opengeolab/geometry/describe_topology_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>

#include <doctest/doctest.h>

using OpenGeoLab::Geometry::DescribeTopologyAction;
using OpenGeoLab::Geometry::ShapeStore;

static TopoDS_Shape makeBox(double w = 10.0, double h = 10.0, double d = 10.0) {
    return BRepPrimAPI_MakeBox(w, h, d).Shape();
}

TEST_SUITE("DescribeTopologyAction") {

    TEST_CASE("describe returns expected schema") {
        ShapeStore store;
        DescribeTopologyAction action(store);
        auto desc = action.describe();
        CHECK(desc["name"] == "describe_topology");
        CHECK(desc.contains("params"));
        CHECK(desc["params"].contains("shapeId"));
    }

    TEST_CASE("execute with valid shapeId returns topology overview") {
        ShapeStore store;
        auto id = store.add("Box", makeBox());
        DescribeTopologyAction action(store);

        auto result = action.execute({{"shapeId", id}}, nullptr);
        CHECK(result["ok"] == true);
        CHECK(result["action"] == "describe_topology");
        CHECK(result["shapeId"] == id);
        CHECK(result["shapeName"] == "Box");

        // Counts
        CHECK(result["counts"]["faces"] == 6);
        CHECK(result["counts"]["edges"] == 12);
        CHECK(result["counts"]["vertices"] == 8);

        // Bounding box
        CHECK(result.contains("boundingBox"));
        auto bb = result["boundingBox"];
        CHECK(bb["min"][0] == doctest::Approx(0.0).epsilon(0.01));
        CHECK(bb["max"][0] == doctest::Approx(10.0).epsilon(0.01));

        // Faces array
        CHECK(result["faces"].is_array());
        CHECK(result["faces"].size() == 6);
        for(const auto& f : result["faces"]) {
            CHECK(f.contains("localId"));
            CHECK(f.contains("surfaceType"));
            CHECK(f["surfaceType"] == "plane");
            CHECK(f.contains("center"));
            CHECK(f.contains("normal"));
            CHECK(f.contains("area"));
        }

        // Edges array
        CHECK(result["edges"].is_array());
        CHECK(result["edges"].size() == 12);
        for(const auto& e : result["edges"]) {
            CHECK(e.contains("localId"));
            CHECK(e.contains("curveType"));
            CHECK(e["curveType"] == "line");
            CHECK(e.contains("length"));
        }
    }

    TEST_CASE("execute with cylinder includes cylinder face info") {
        ShapeStore store;
        auto id = store.add("Cyl", BRepPrimAPI_MakeCylinder(5.0, 10.0).Shape());
        DescribeTopologyAction action(store);

        auto result = action.execute({{"shapeId", id}}, nullptr);
        CHECK(result["ok"] == true);

        bool found_cylinder = false;
        for(const auto& f : result["faces"]) {
            if(f["surfaceType"] == "cylinder") {
                found_cylinder = true;
                CHECK(f.contains("axis"));
                CHECK(f.contains("radius"));
                CHECK(f["radius"].get<double>() == doctest::Approx(5.0).epsilon(0.01));
            }
        }
        CHECK(found_cylinder);
    }

    TEST_CASE("execute with unknown shapeId returns error") {
        ShapeStore store;
        DescribeTopologyAction action(store);
        auto result = action.execute({{"shapeId", 999}}, nullptr);
        CHECK(result["ok"] == false);
        CHECK(result.contains("error"));
    }

    TEST_CASE("execute without shapeId returns error") {
        ShapeStore store;
        DescribeTopologyAction action(store);
        auto result = action.execute(nlohmann::json::object(), nullptr);
        CHECK(result["ok"] == false);
    }

    TEST_CASE("execute reports progress") {
        ShapeStore store;
        auto id = store.add("Box", makeBox());
        DescribeTopologyAction action(store);

        std::vector<double> progress_values;
        auto progress_cb = [&](double p, const std::string&) {
            progress_values.push_back(p);
            return true;
        };
        auto progress_result = action.execute({{"shapeId", id}}, progress_cb);
        CHECK(progress_result["ok"] == true);
        CHECK_FALSE(progress_values.empty());
        CHECK(progress_values.back() == doctest::Approx(1.0));
    }

} // TEST_SUITE
