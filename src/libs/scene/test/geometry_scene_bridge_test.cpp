/**
 * @file geometry_scene_bridge_test.cpp
 * @brief Unit tests for GeometrySceneBridge
 */

#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/scene/geometry_scene_bridge.hpp>

#include <BRepPrimAPI_MakeBox.hxx>

#include <doctest/doctest.h>

#include <cstdint>
#include <string>

namespace OpenGeoLab::Scene::Tests {

namespace {

SceneNode* firstChild(SceneGraph& scene) {
    if(scene.root()->children().empty()) {
        return nullptr;
    }

    return scene.root()->children().front().get();
}

struct BridgeFixture {
    Geometry::ShapeStore store;
    SceneGraph scene;
    TopologyIndex topoIndex;
    GeometrySceneBridge bridge{scene, store, topoIndex};
};

} // namespace

TEST_CASE("GeometrySceneBridge creates renderable node after tessellation") {
    BridgeFixture fixture;
    BRepPrimAPI_MakeBox boxMaker(10.0, 20.0, 30.0);

    const uint32_t shapeId = fixture.store.add("TestBox", boxMaker.Shape());
    fixture.store.tessellate(shapeId);

    SceneNode* node = firstChild(fixture.scene);
    REQUIRE(node != nullptr);
    CHECK(std::string{node->name()} == "TestBox");
    REQUIRE(node->renderComponent() != nullptr);
    REQUIRE(node->pickComponent() != nullptr);

    const RenderMeshData& meshData = node->renderComponent()->meshData();
    CHECK_FALSE(meshData.vertices.empty());
    CHECK(meshData.pickIds.size() == meshData.vertices.size());
    CHECK_FALSE(meshData.triangleRanges.empty());
    CHECK_FALSE(meshData.lineRanges.empty());
    CHECK(meshData.bounds.isValid());
    CHECK(fixture.topoIndex.edgeToWire(shapeId, 1).has_value());
    CHECK(node->sourceType() == "geometry");
    CHECK(node->sourceId() == shapeId);
}

TEST_CASE("GeometrySceneBridge removes scene node and topology data") {
    BridgeFixture fixture;
    BRepPrimAPI_MakeBox boxMaker(10.0, 20.0, 30.0);

    const uint32_t shapeId = fixture.store.add("TestBox", boxMaker.Shape());
    fixture.store.tessellate(shapeId);
    REQUIRE(firstChild(fixture.scene) != nullptr);
    REQUIRE(fixture.topoIndex.edgeToWire(shapeId, 1).has_value());

    fixture.store.remove(shapeId);

    CHECK(firstChild(fixture.scene) == nullptr);
    CHECK_FALSE(fixture.topoIndex.edgeToWire(shapeId, 1).has_value());
}

TEST_CASE("GeometrySceneBridge refreshes mesh data on retessellation") {
    BridgeFixture fixture;
    BRepPrimAPI_MakeBox boxMaker(10.0, 20.0, 30.0);

    const uint32_t shapeId = fixture.store.add("TestBox", boxMaker.Shape());
    fixture.store.tessellate(shapeId);

    SceneNode* node = firstChild(fixture.scene);
    REQUIRE(node != nullptr);
    REQUIRE(node->renderComponent() != nullptr);

    const uint64_t originalVersion = node->renderComponent()->dataVersion();
    const std::size_t originalVertexCount = node->renderComponent()->meshData().vertices.size();

    fixture.store.tessellate(shapeId, Geometry::TessellationParams{0.05, 0.25});

    node = firstChild(fixture.scene);
    REQUIRE(node != nullptr);
    REQUIRE(node->renderComponent() != nullptr);
    CHECK(node->renderComponent()->dataVersion() > originalVersion);
    CHECK(node->renderComponent()->meshData().vertices.size() >= originalVertexCount);
    CHECK(node->renderComponent()->meshData().bounds.isValid());
}

TEST_CASE("GeometrySceneBridge buildRenderData keeps pickIds aligned with vertices") {
    Geometry::ShapeStore store;
    SceneGraph scene;
    TopologyIndex topoIndex;
    GeometrySceneBridge bridge(scene, store, topoIndex);
    BRepPrimAPI_MakeBox boxMaker(10.0, 20.0, 30.0);

    const uint32_t shapeId = store.add("TestBox", boxMaker.Shape());
    store.tessellate(shapeId);
    const Geometry::ShapeEntry* entry = store.find(shapeId);
    REQUIRE(entry != nullptr);

    const RenderMeshData data = GeometrySceneBridge::buildRenderData(shapeId, *entry);

    CHECK(data.pickIds.size() == data.vertices.size());
    CHECK(data.bounds.isValid());
}

TEST_CASE("GeometrySceneBridge destructor disconnects from ShapeStore") {
    Geometry::ShapeStore store;
    SceneGraph scene;
    TopologyIndex topoIndex;

    {
        GeometrySceneBridge bridge(scene, store, topoIndex);
    }

    BRepPrimAPI_MakeBox boxMaker(10.0, 20.0, 30.0);
    const uint32_t shapeId = store.add("DetachedBox", boxMaker.Shape());
    store.tessellate(shapeId);

    CHECK(firstChild(scene) == nullptr);
    CHECK_FALSE(topoIndex.edgeToWire(shapeId, 1).has_value());
}

} // namespace OpenGeoLab::Scene::Tests
