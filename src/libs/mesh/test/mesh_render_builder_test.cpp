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

    CHECK(data.vertices.size() == 12);
    CHECK(data.pickIds.size() == data.vertices.size());
    CHECK(data.triangleRanges.size() == 1);
    CHECK(data.triangleRanges[0].entityType == Core::EntityType::MeshElement);
    CHECK(data.triangleRanges[0].localId == 1);
    CHECK(data.triangleRanges[0].vertexCount == 3);
    CHECK(data.triangleRanges[0].indexCount == 3);
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
        CHECK(range.vertexCount == 1);
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
    CHECK(data.triangleRanges[0].vertexCount == 6);
    CHECK(data.triangleRanges[0].indexCount == 6);
    CHECK(data.lineRanges.size() == 4);
    CHECK(data.pointRanges.size() == 4);
}

TEST_CASE("MeshRenderBuilder: pick ids are encoded per topology section") {
    const auto data = Mesh::MeshRenderBuilder::build(7, makeTriEntry());

    REQUIRE(data.pointRanges.size() == 3);
    for(const auto& range : data.pointRanges) {
        const auto pick_id = data.pickIds[range.vertexOffset].pickId;
        CHECK(pick_id == Scene::PickId::encode(7, Core::EntityType::MeshNode, range.localId));
    }

    REQUIRE(data.triangleRanges.size() == 1);
    const auto triangle_pick = data.pickIds[data.triangleRanges[0].vertexOffset].pickId;
    CHECK(triangle_pick == Scene::PickId::encode(7, Core::EntityType::MeshElement, 1));

    REQUIRE(data.lineRanges.size() == 3);
    for(const auto& range : data.lineRanges) {
        const auto pick_id = data.pickIds[range.vertexOffset].pickId;
        CHECK(pick_id == Scene::PickId::encode(7, Core::EntityType::MeshEdge, range.localId));
    }
}

TEST_CASE("MeshRenderBuilder: shared edges are deduplicated across elements") {
    Mesh::MeshEntry entry;
    entry.shapeId = 1;
    entry.nodes = {
        Mesh::MeshNode{{0.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{1.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{1.0F, 1.0F, 0.0F}},
        Mesh::MeshNode{{0.0F, 1.0F, 0.0F}},
    };

    Mesh::MeshElement first;
    first.type = Mesh::MeshElementType::Triangle;
    first.nodeLocalIds = {1, 2, 3, 0, 0, 0, 0, 0};
    Mesh::MeshElement second;
    second.type = Mesh::MeshElementType::Triangle;
    second.nodeLocalIds = {1, 3, 4, 0, 0, 0, 0, 0};
    entry.elements = {first, second};

    const auto data = Mesh::MeshRenderBuilder::build(1, entry);
    CHECK(data.triangleRanges.size() == 2);
    CHECK(data.lineRanges.size() == 5);
}

TEST_CASE("MeshRenderBuilder: tetra emits non-zero triangle normals") {
    Mesh::MeshEntry entry;
    entry.shapeId = 3;
    entry.nodes = {
        Mesh::MeshNode{{0.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{1.0F, 0.0F, 0.0F}},
        Mesh::MeshNode{{0.0F, 1.0F, 0.0F}},
        Mesh::MeshNode{{0.0F, 0.0F, 1.0F}},
    };
    Mesh::MeshElement tetra;
    tetra.type = Mesh::MeshElementType::Tetra;
    tetra.nodeLocalIds = {1, 2, 3, 4, 0, 0, 0, 0};
    entry.elements = {tetra};

    const auto data = Mesh::MeshRenderBuilder::build(3, entry);
    REQUIRE(data.triangleRanges.size() == 1);
    CHECK(data.triangleRanges[0].vertexCount == 12);
    CHECK(data.triangleRanges[0].indexCount == 12);
    const auto& first_triangle_vertex = data.vertices[data.triangleRanges[0].vertexOffset];
    const bool has_non_zero_normal = first_triangle_vertex.normal[0] != doctest::Approx(0.0F) ||
                                     first_triangle_vertex.normal[1] != doctest::Approx(0.0F) ||
                                     first_triangle_vertex.normal[2] != doctest::Approx(0.0F);
    CHECK(has_non_zero_normal);
}
