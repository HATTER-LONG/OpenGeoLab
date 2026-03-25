/**
 * @file geometry_module_test.cpp
 * @brief Placeholder tests for the geometry module.
 */

#include <doctest/doctest.h>

#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/scene/bounding_box.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <nlohmann/json.hpp>

#include "shape_store.hpp"
#include "tessellator.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include "occ_primitives.hpp"

namespace {

int countSubShapes(const TopoDS_Shape& shape, TopAbs_ShapeEnum type) {
    TopTools_IndexedMapOfShape shapes;
    TopExp::MapShapes(shape, type, shapes);
    return shapes.Extent();
}

} // namespace

TEST_CASE("GeometryModule create_box JSON") {
    OpenGeoLab::Scene::SceneGraph graph;
    OpenGeoLab::Geometry::GeometryModule module(graph);

    auto response = module.process(R"({
        "action": "create_box",
        "center": [0.0, 0.0, 0.0],
        "size": [2.0, 3.0, 4.0]
    })");

    auto j = nlohmann::json::parse(response);
    CHECK(j["ok"] == true);
    CHECK(j["result"]["id"].get<int>() > 0);
    CHECK(graph.root().children.size() == 1);
    CHECK(!graph.root().children[0].meshes.empty());
}

TEST_CASE("GeometryModule create_cylinder JSON") {
    OpenGeoLab::Scene::SceneGraph graph;
    OpenGeoLab::Geometry::GeometryModule module(graph);

    auto response = module.process(R"({
        "action": "create_cylinder",
        "center": [0.0, 0.0, 0.0],
        "radius": 1.0,
        "height": 5.0
    })");

    auto j = nlohmann::json::parse(response);
    CHECK(j["ok"] == true);
    CHECK(j["result"]["id"].get<int>() > 0);
    CHECK(graph.root().children.size() == 1);
}

TEST_CASE("GeometryModule list_shapes") {
    OpenGeoLab::Scene::SceneGraph graph;
    OpenGeoLab::Geometry::GeometryModule module(graph);

    const auto createBoxResponse =
        module.process(R"({"action":"create_box","center":[0,0,0],"size":[1,1,1]})");
    const auto createSphereResponse =
        module.process(R"({"action":"create_sphere","center":[5,0,0],"radius":2.0})");
    CHECK(!createBoxResponse.empty());
    CHECK(!createSphereResponse.empty());

    auto response = module.process(R"({"action":"list_shapes"})");
    auto j = nlohmann::json::parse(response);
    CHECK(j["ok"] == true);
    CHECK(j["result"]["count"].get<int>() == 2);
}

TEST_CASE("GeometryModule unknown action") {
    OpenGeoLab::Scene::SceneGraph graph;
    OpenGeoLab::Geometry::GeometryModule module(graph);

    auto response = module.process(R"({"action":"nonexistent"})");
    auto j = nlohmann::json::parse(response);
    CHECK(j["ok"] == false);
    CHECK(j.contains("error"));
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

TEST_CASE("Tessellator tessellate box") {
    TopoDS_Shape box = BRepPrimAPI_MakeBox(2.0, 3.0, 4.0).Shape();
    auto mesh = OpenGeoLab::Geometry::Tessellator::tessellate(box);

    CHECK(!mesh.positions.empty());
    CHECK(!mesh.indices.empty());
    CHECK(mesh.topology == OpenGeoLab::Scene::PrimitiveType::Triangles);
    CHECK(mesh.indices.size() % 3 == 0);
    auto vertexCount = static_cast<std::uint32_t>(mesh.positions.size() / 3);
    for(auto idx : mesh.indices) {
        CHECK(idx < vertexCount);
    }
}

TEST_CASE("Tessellator extractEdges box") {
    TopoDS_Shape box = BRepPrimAPI_MakeBox(2.0, 3.0, 4.0).Shape();
    auto mesh = OpenGeoLab::Geometry::Tessellator::extractEdges(box);

    CHECK(!mesh.positions.empty());
    CHECK(mesh.topology == OpenGeoLab::Scene::PrimitiveType::Lines);
}

TEST_CASE("Tessellator computeBounds") {
    TopoDS_Shape box = BRepPrimAPI_MakeBox(2.0, 2.0, 2.0).Shape();
    auto bounds = OpenGeoLab::Geometry::Tessellator::computeBounds(box);

    CHECK(bounds.isValid());
    CHECK(bounds.min.x == doctest::Approx(0.0).epsilon(0.01));
    CHECK(bounds.min.y == doctest::Approx(0.0).epsilon(0.01));
    CHECK(bounds.min.z == doctest::Approx(0.0).epsilon(0.01));
    CHECK(bounds.max.x == doctest::Approx(2.0).epsilon(0.01));
    CHECK(bounds.max.y == doctest::Approx(2.0).epsilon(0.01));
    CHECK(bounds.max.z == doctest::Approx(2.0).epsilon(0.01));
}

TEST_CASE("makePrimitive topology counts") {
    using namespace OpenGeoLab::Geometry;

    SUBCASE("makeBox has 6 faces, 12 edges, 8 vertices") {
        auto box = makeBox({0.0, 0.0, 0.0}, {2.0, 3.0, 4.0});
        CHECK(countSubShapes(box, TopAbs_FACE) == 6);
        CHECK(countSubShapes(box, TopAbs_EDGE) == 12);
        CHECK(countSubShapes(box, TopAbs_VERTEX) == 8);
    }

    SUBCASE("makeSphere has faces") {
        auto sphere = makeSphere({0.0, 0.0, 0.0}, 1.0);
        CHECK(countSubShapes(sphere, TopAbs_FACE) > 0);
    }

    SUBCASE("makeCylinder has faces") {
        auto cylinder = makeCylinder({0.0, 0.0, 0.0}, 1.0, 5.0);
        CHECK(countSubShapes(cylinder, TopAbs_FACE) > 0);
    }

    SUBCASE("makeTorus has faces") {
        auto torus = makeTorus({0.0, 0.0, 0.0}, 2.0, 0.5);
        CHECK(countSubShapes(torus, TopAbs_FACE) > 0);
    }
}
