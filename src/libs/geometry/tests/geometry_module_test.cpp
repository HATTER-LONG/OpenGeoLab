/**
 * @file geometry_module_test.cpp
 * @brief Placeholder tests for the geometry module.
 */

#include <doctest/doctest.h>

#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/scene/bounding_box.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>

#include "shape_store.hpp"

#include <BRepPrimAPI_MakeBox.hxx>

TEST_CASE("GeometryModule stub returns not-implemented") {
    OpenGeoLab::Geometry::GeometryModule mod;
    auto result = mod.process(R"({"action":"unknown"})");
    CHECK(result.find("not implemented") != std::string::npos);
}

TEST_CASE("ShapeStore addShape and retrieve") {
    OpenGeoLab::Geometry::ShapeStore store;

    TopoDS_Shape box1 = BRepPrimAPI_MakeBox(1.0, 2.0, 3.0).Shape();
    TopoDS_Shape box2 = BRepPrimAPI_MakeBox(4.0, 5.0, 6.0).Shape();

    OpenGeoLab::Scene::RenderMeshData emptyMesh;
    OpenGeoLab::Scene::BoundingBox emptyBounds;

    int id1 = store.addShape(box1, "box1", emptyMesh, emptyMesh, emptyBounds);
    int id2 = store.addShape(box2, "box2", emptyMesh, emptyMesh, emptyBounds);

    CHECK(id1 > 0);
    CHECK(id2 > 0);
    CHECK(id1 != id2);

    auto info1 = store.getInfo(id1);
    CHECK(info1.id == id1);
    CHECK(info1.label == "box1");

    auto infos = store.allInfos();
    CHECK(infos.size() == 2);
    CHECK(store.shapeCount() == 2);
}

TEST_CASE("ShapeStore removeShape") {
    OpenGeoLab::Geometry::ShapeStore store;
    TopoDS_Shape box = BRepPrimAPI_MakeBox(1.0, 1.0, 1.0).Shape();
    OpenGeoLab::Scene::RenderMeshData emptyMesh;
    OpenGeoLab::Scene::BoundingBox emptyBounds;

    int id = store.addShape(box, "box", emptyMesh, emptyMesh, emptyBounds);
    CHECK(store.shapeCount() == 1);

    CHECK(store.removeShape(id) == true);
    CHECK(store.shapeCount() == 0);
    CHECK(store.removeShape(999) == false);
}

TEST_CASE("ShapeStore clear") {
    OpenGeoLab::Geometry::ShapeStore store;
    TopoDS_Shape box = BRepPrimAPI_MakeBox(1.0, 1.0, 1.0).Shape();
    OpenGeoLab::Scene::RenderMeshData emptyMesh;
    OpenGeoLab::Scene::BoundingBox emptyBounds;

    store.addShape(box, "a", emptyMesh, emptyMesh, emptyBounds);
    store.addShape(box, "b", emptyMesh, emptyMesh, emptyBounds);
    store.addShape(box, "c", emptyMesh, emptyMesh, emptyBounds);
    CHECK(store.shapeCount() == 3);

    store.clear();
    CHECK(store.shapeCount() == 0);
}
