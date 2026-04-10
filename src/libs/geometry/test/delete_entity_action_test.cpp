/**
 * @file delete_entity_action_test.cpp
 * @brief Unit tests for DeleteEntityAction — face and solid deletion
 */

#include <opengeolab/geometry/delete_entity_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Pnt.hxx>

#include <doctest/doctest.h>

using OpenGeoLab::Core::NO_PROGRESS_CALLBACK;
using OpenGeoLab::Geometry::DeleteEntityAction;
using OpenGeoLab::Geometry::ShapeStore;

static TopoDS_Shape makeBox(double w = 1.0, double h = 1.0, double d = 1.0) {
    return BRepPrimAPI_MakeBox(w, h, d).Shape();
}

static TopoDS_Shape makeTwoBoxCompound() {
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    builder.Add(compound, BRepPrimAPI_MakeBox(1, 1, 1).Shape());
    builder.Add(compound, BRepPrimAPI_MakeBox(gp_Pnt(5, 0, 0), 2, 2, 2).Shape());
    return compound;
}

TEST_CASE("DeleteEntityAction describe returns valid schema") {
    ShapeStore store;
    DeleteEntityAction action(store);
    auto desc = action.describe();
    CHECK(desc["name"] == "delete_entity");
    CHECK(desc.contains("description"));
    CHECK(desc.contains("params"));
    CHECK(desc["params"].contains("entities"));
}

TEST_CASE("DeleteEntityAction returns error for empty entities array") {
    ShapeStore store;
    DeleteEntityAction action(store);
    const nlohmann::json param = {{"entities", nlohmann::json::array()}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == false);
}

TEST_CASE("DeleteEntityAction returns error for unknown shapeId") {
    ShapeStore store;
    DeleteEntityAction action(store);
    const nlohmann::json param = {
        {"entities", {{{"shapeId", 999}, {"type", "GeoFace"}, {"localId", 1}}}}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == false);
}

TEST_CASE("DeleteEntityAction returns error for unsupported entity type") {
    ShapeStore store;
    const auto id = store.add("Box", makeBox());
    DeleteEntityAction action(store);
    const nlohmann::json param = {
        {"entities", {{{"shapeId", id}, {"type", "GeoEdge"}, {"localId", 1}}}}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == false);
    auto results = result["results"];
    REQUIRE(results.is_array());
    CHECK(results[0]["status"] == "unsupported");
}

TEST_CASE("DeleteEntityAction does not mix unsupported and supported entities for one shape") {
    ShapeStore store;
    const auto id = store.add("Box", makeBox());
    const auto initial_face_count = store.find(id)->faceMap.Extent();

    DeleteEntityAction action(store);
    const nlohmann::json param = {{"entities",
                                   {{{"shapeId", id}, {"type", "GeoEdge"}, {"localId", 1}},
                                    {{"shapeId", id}, {"type", "GeoFace"}, {"localId", 1}}}}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);

    CHECK(result["ok"] == false);
    auto results = result["results"];
    REQUIRE(results.is_array());
    REQUIRE(results.size() == 1);
    CHECK(results[0]["status"] == "unsupported");
    REQUIRE(store.find(id) != nullptr);
    CHECK(store.find(id)->faceMap.Extent() == initial_face_count);
}

TEST_CASE("DeleteEntityAction removes a face from a box (defeaturing)") {
    ShapeStore store;
    auto id = store.add("Box", makeBox(10, 10, 10));
    store.tessellate(id);
    REQUIRE(store.find(id)->faceMap.Extent() == 6);

    DeleteEntityAction action(store);
    const nlohmann::json param = {
        {"entities", {{{"shapeId", id}, {"type", "GeoFace"}, {"localId", 1}}}}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);

    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);
    CHECK(entry->faceMap.Extent() == 5);
    CHECK(entry->visualData != nullptr);
}

TEST_CASE("DeleteEntityAction removes a solid from a compound") {
    ShapeStore store;
    auto id = store.add("TwoBoxes", makeTwoBoxCompound());
    store.tessellate(id);
    REQUIRE(store.find(id)->solidMap.Extent() == 2);

    DeleteEntityAction action(store);
    const nlohmann::json param = {
        {"entities", {{{"shapeId", id}, {"type", "GeoSolid"}, {"localId", 1}}}}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);

    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);
    CHECK(entry->solidMap.Extent() == 1);
    CHECK(entry->visualData != nullptr);
}

TEST_CASE("DeleteEntityAction removes all solids deletes entire shape") {
    ShapeStore store;
    auto id = store.add("SingleBox", makeBox());
    REQUIRE(store.find(id)->solidMap.Extent() == 1);

    DeleteEntityAction action(store);
    const nlohmann::json param = {
        {"entities", {{{"shapeId", id}, {"type", "GeoSolid"}, {"localId", 1}}}}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);

    CHECK(store.find(id) == nullptr);
}

TEST_CASE("DeleteEntityAction returns error for invalid localId") {
    ShapeStore store;
    const auto id = store.add("Box", makeBox());
    DeleteEntityAction action(store);
    const nlohmann::json param = {
        {"entities", {{{"shapeId", id}, {"type", "GeoFace"}, {"localId", 999}}}}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == false);
}
