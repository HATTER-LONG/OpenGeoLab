#include "mesh_render_builder.hpp"

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/mesh/mesh_entry.hpp>
#include <opengeolab/scene/pick_id.hpp>

#include <doctest/doctest.h>

using namespace OpenGeoLab;

namespace {

Mesh::MeshEntry makeTriEntry() {
    Mesh::MeshEntry entry;
    entry.shapeId = 1;
    entry.nodes = {
        Mesh::MeshNode{{0.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{1.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{0.5F, 1.0F, 0.0F}},
    };
    Mesh::MeshElement tri;
    tri.type = Mesh::MeshElementType::Triangle;
    tri.nodeLocalIds = {1, 2, 3, 0, 0, 0, 0, 0};
    entry.elements = {tri};
    return entry;
}

} // namespace

TEST_CASE("MeshRenderBuilder: single triangle produces correct ranges") {
    const auto data = Mesh::MeshRenderBuilder::build(1, makeTriEntry());

    CHECK(data.vertices.size() == 3);
    CHECK(data.pickIds.size() == 3);
    CHECK(data.triangleRanges.size() == 1);
    CHECK(data.triangleRanges[0].entityType == Core::EntityType::MeshElement);
    CHECK(data.triangleRanges[0].localId == 1);
    CHECK(data.lineRanges.size() == 3);
    CHECK(data.pointRanges.size() == 3);
}

TEST_CASE("MeshRenderBuilder: edge ranges have MeshEdge entityType") {
    const auto data = Mesh::MeshRenderBuilder::build(1, makeTriEntry());
    for(const auto& range : data.lineRanges) {
        CHECK(range.entityType == Core::EntityType::MeshEdge);
    }
}

TEST_CASE("MeshRenderBuilder: point ranges have MeshNode entityType") {
    const auto data = Mesh::MeshRenderBuilder::build(1, makeTriEntry());
    for(const auto& range : data.pointRanges) {
        CHECK(range.entityType == Core::EntityType::MeshNode);
    }
}

TEST_CASE("MeshRenderBuilder: empty entry returns empty data") {
    Mesh::MeshEntry entry;
    entry.shapeId = 1;

    const auto data = Mesh::MeshRenderBuilder::build(1, entry);
    CHECK(data.vertices.empty());
    CHECK(data.triangleRanges.empty());
    CHECK(data.lineRanges.empty());
    CHECK(data.pointRanges.empty());
}

TEST_CASE("MeshRenderBuilder: quad produces 2 triangles") {
    Mesh::MeshEntry entry;
    entry.shapeId = 1;
    entry.nodes = {
        Mesh::MeshNode{{0.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{1.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{1.0F, 1.0F, 0.0F}},
        Mesh::MeshNode{{0.0F, 1.0F, 0.0F}},
    };
    Mesh::MeshElement quad;
    quad.type = Mesh::MeshElementType::Quad;
    quad.nodeLocalIds = {1, 2, 3, 4, 0, 0, 0, 0};
    entry.elements = {quad};

    const auto data = Mesh::MeshRenderBuilder::build(1, entry);
    REQUIRE(data.triangleRanges.size() == 1);
    CHECK(data.triangleRanges[0].indexCount == 6);
    CHECK(data.lineRanges.size() == 4);
    CHECK(data.pointRanges.size() == 4);
}

TEST_CASE("MeshRenderBuilder: point and element picks are encoded") {
    const auto data = Mesh::MeshRenderBuilder::build(7, makeTriEntry());

    REQUIRE(data.pickIds.size() == 3);
    for(const auto& pick_entry : data.pickIds) {
        CHECK(pick_entry.pickId == Scene::PickId::encode(7, Core::EntityType::MeshElement, 1));
    }

    REQUIRE(data.pointRanges.size() == 3);
    CHECK(data.pointRanges[0].localId == 1);
    CHECK(data.pointRanges[1].localId == 2);
    CHECK(data.pointRanges[2].localId == 3);
}
