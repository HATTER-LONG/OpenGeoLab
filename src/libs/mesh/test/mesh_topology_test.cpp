/**
 * @file mesh_topology_test.cpp
 * @brief MeshTopology unit tests
 */

#include <opengeolab/mesh/mesh_topology.hpp>

#include <opengeolab/mesh/mesh_element.hpp>
#include <opengeolab/mesh/mesh_entry.hpp>
#include <opengeolab/mesh/mesh_node.hpp>

#include <doctest/doctest.h>

#include <algorithm>

using OpenGeoLab::Mesh::MeshElement;
using OpenGeoLab::Mesh::MeshElementType;
using OpenGeoLab::Mesh::MeshEntry;
using OpenGeoLab::Mesh::MeshNode;
using OpenGeoLab::Mesh::MeshTopology;

namespace {

MeshEntry makeSingleTriangle() {
    MeshEntry entry;
    entry.shapeId = 1;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}},
        MeshNode{{1.0F, 0.0F, 0.0F}},
        MeshNode{{0.5F, 1.0F, 0.0F}},
    };
    MeshElement tri{};
    tri.type = MeshElementType::Triangle;
    tri.nodeLocalIds = {1, 2, 3, 0, 0, 0, 0, 0};
    entry.elements = {tri};
    return entry;
}

MeshEntry makeTwoTrianglesSharedEdge() {
    MeshEntry entry;
    entry.shapeId = 2;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}},
        MeshNode{{1.0F, 0.0F, 0.0F}},
        MeshNode{{0.5F, 1.0F, 0.0F}},
        MeshNode{{1.5F, 1.0F, 0.0F}},
    };
    MeshElement tri0{};
    tri0.type = MeshElementType::Triangle;
    tri0.nodeLocalIds = {1, 2, 3, 0, 0, 0, 0, 0};
    MeshElement tri1{};
    tri1.type = MeshElementType::Triangle;
    tri1.nodeLocalIds = {2, 4, 3, 0, 0, 0, 0, 0};
    entry.elements = {tri0, tri1};
    return entry;
}

MeshEntry makeSingleQuad() {
    MeshEntry entry;
    entry.shapeId = 3;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}},
        MeshNode{{1.0F, 0.0F, 0.0F}},
        MeshNode{{1.0F, 1.0F, 0.0F}},
        MeshNode{{0.0F, 1.0F, 0.0F}},
    };
    MeshElement quad{};
    quad.type = MeshElementType::Quad;
    quad.nodeLocalIds = {1, 2, 3, 4, 0, 0, 0, 0};
    entry.elements = {quad};
    return entry;
}

} // namespace

TEST_CASE("MeshTopology: single triangle has 3 edges") {
    const auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);

    CHECK(topo.edges.size() == 3);
    for(const auto& [n1, n2] : topo.edges) {
        CHECK(n1 < n2);
        CHECK(n2 <= 2);
    }
}

TEST_CASE("MeshTopology: edgeToElements maps edges to element indices") {
    const auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);

    REQUIRE(topo.edgeToElements.size() == topo.edges.size());
    for(const auto& elems : topo.edgeToElements) {
        REQUIRE(elems.size() == 1);
        CHECK(elems[0] == 0);
    }
}

TEST_CASE("MeshTopology: two triangles shared edge") {
    const auto entry = makeTwoTrianglesSharedEdge();
    const auto topo = MeshTopology::build(entry);

    CHECK(topo.edges.size() == 5);

    const auto shared_idx = topo.findEdgeIndex(2, 3);
    REQUIRE(shared_idx.has_value());

    const auto& shared_elems = topo.edgeToElements[shared_idx.value()];
    CHECK(shared_elems.size() == 2);
    auto sorted_elems = shared_elems;
    std::sort(sorted_elems.begin(), sorted_elems.end());
    CHECK(sorted_elems[0] == 0);
    CHECK(sorted_elems[1] == 1);
}

TEST_CASE("MeshTopology: single quad has 4 edges") {
    const auto entry = makeSingleQuad();
    const auto topo = MeshTopology::build(entry);

    CHECK(topo.edges.size() == 4);
}

TEST_CASE("MeshTopology: nodeToElements adjacency") {
    const auto entry = makeTwoTrianglesSharedEdge();
    const auto topo = MeshTopology::build(entry);

    REQUIRE(topo.nodeToElements.size() > 1);
    CHECK(topo.nodeToElements[1].size() == 1);
    CHECK(topo.nodeToElements[1][0] == 0);

    REQUIRE(topo.nodeToElements.size() > 2);
    CHECK(topo.nodeToElements[2].size() == 2);

    REQUIRE(topo.nodeToElements.size() > 3);
    CHECK(topo.nodeToElements[3].size() == 2);

    REQUIRE(topo.nodeToElements.size() > 4);
    CHECK(topo.nodeToElements[4].size() == 1);
    CHECK(topo.nodeToElements[4][0] == 1);
}

TEST_CASE("MeshTopology: nodeToEdges adjacency") {
    const auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);

    for(uint32_t local_id = 1; local_id <= 3; ++local_id) {
        REQUIRE(topo.nodeToEdges.size() > local_id);
        CHECK(topo.nodeToEdges[local_id].size() == 2);
    }
}

TEST_CASE("MeshTopology: resolveEdge returns correct node pair") {
    const auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);

    for(uint32_t i = 0; i < topo.edges.size(); ++i) {
        const uint32_t local_id = i + 1;
        const auto [n1, n2] = topo.resolveEdge(local_id);
        CHECK(n1 == topo.edges[i].first);
        CHECK(n2 == topo.edges[i].second);
    }
}

TEST_CASE("MeshTopology: findEdgeIndex returns nullopt for non-existent edge") {
    const auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);

    CHECK_FALSE(topo.findEdgeIndex(1, 99).has_value());
}

TEST_CASE("MeshTopology: findEdgeIndex is order-independent") {
    const auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);

    const auto idx_forward = topo.findEdgeIndex(1, 2);
    const auto idx_reverse = topo.findEdgeIndex(2, 1);
    REQUIRE(idx_forward.has_value());
    REQUIRE(idx_reverse.has_value());
    CHECK(idx_forward.value() == idx_reverse.value());
}

TEST_CASE("MeshTopology: Tri6 has 3 edges (corner-only)") {
    MeshEntry entry;
    entry.shapeId = 10;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}}, MeshNode{{2.0F, 0.0F, 0.0F}}, MeshNode{{1.0F, 2.0F, 0.0F}},
        MeshNode{{1.0F, 0.0F, 0.0F}}, MeshNode{{1.5F, 1.0F, 0.0F}}, MeshNode{{0.5F, 1.0F, 0.0F}},
    };
    MeshElement tri6{};
    tri6.type = MeshElementType::Tri6;
    tri6.nodeLocalIds = {1, 2, 3, 4, 5, 6, 0, 0, 0};
    entry.elements = {tri6};

    const auto topo = MeshTopology::build(entry);
    CHECK(topo.edges.size() == 3);
    CHECK(topo.edgeToElements.size() == 3);
}

TEST_CASE("MeshTopology: Quad9 has 4 edges (corner-only)") {
    MeshEntry entry;
    entry.shapeId = 11;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}}, MeshNode{{2.0F, 0.0F, 0.0F}}, MeshNode{{2.0F, 2.0F, 0.0F}},
        MeshNode{{0.0F, 2.0F, 0.0F}}, MeshNode{{1.0F, 0.0F, 0.0F}}, MeshNode{{2.0F, 1.0F, 0.0F}},
        MeshNode{{1.0F, 2.0F, 0.0F}}, MeshNode{{0.0F, 1.0F, 0.0F}}, MeshNode{{1.0F, 1.0F, 0.0F}},
    };
    MeshElement quad9{};
    quad9.type = MeshElementType::Quad9;
    quad9.nodeLocalIds = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    entry.elements = {quad9};

    const auto topo = MeshTopology::build(entry);
    CHECK(topo.edges.size() == 4);
    CHECK(topo.edgeToElements.size() == 4);
}

TEST_CASE("MeshTopology: empty entry produces empty topology") {
    MeshEntry entry;
    entry.shapeId = 99;
    const auto topo = MeshTopology::build(entry);

    CHECK(topo.edges.empty());
    CHECK(topo.edgeToElements.empty());
    CHECK(topo.nodeToElements.empty());
    CHECK(topo.nodeToEdges.empty());
}
