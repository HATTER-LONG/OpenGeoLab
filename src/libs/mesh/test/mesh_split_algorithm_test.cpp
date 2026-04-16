/**
 * @file mesh_split_algorithm_test.cpp
 * @brief MeshSplitAlgorithm unit tests
 */

#include <opengeolab/mesh/mesh_split_algorithm.hpp>

#include <opengeolab/mesh/mesh_element.hpp>
#include <opengeolab/mesh/mesh_entry.hpp>
#include <opengeolab/mesh/mesh_node.hpp>
#include <opengeolab/mesh/mesh_topology.hpp>
#include <opengeolab/mesh/split_mode.hpp>
#include <opengeolab/mesh/split_result.hpp>

#include <doctest/doctest.h>

#include <algorithm>
#include <vector>

using OpenGeoLab::Mesh::MeshElement;
using OpenGeoLab::Mesh::MeshElementType;
using OpenGeoLab::Mesh::MeshEntry;
using OpenGeoLab::Mesh::MeshNode;
using OpenGeoLab::Mesh::MeshSplitAlgorithm;
using OpenGeoLab::Mesh::MeshTopology;
using OpenGeoLab::Mesh::SplitMode;
using OpenGeoLab::Mesh::SplitResult;

static MeshEntry makeSingleTriangle() {
    MeshEntry entry;
    entry.shapeId = 1;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}},
        MeshNode{{2.0F, 0.0F, 0.0F}},
        MeshNode{{1.0F, 2.0F, 0.0F}},
    };
    MeshElement tri{};
    tri.type = MeshElementType::Triangle;
    tri.nodeLocalIds = {1, 2, 3, 0, 0, 0, 0, 0};
    entry.elements = {tri};
    return entry;
}

static MeshEntry makeTwoTrianglesSharedEdge() {
    MeshEntry entry;
    entry.shapeId = 2;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}},
        MeshNode{{2.0F, 0.0F, 0.0F}},
        MeshNode{{1.0F, 2.0F, 0.0F}},
        MeshNode{{3.0F, 2.0F, 0.0F}},
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

static MeshEntry makeSingleQuad() {
    MeshEntry entry;
    entry.shapeId = 1;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}}, // localId 1
        MeshNode{{2.0F, 0.0F, 0.0F}}, // localId 2
        MeshNode{{2.0F, 2.0F, 0.0F}}, // localId 3
        MeshNode{{0.0F, 2.0F, 0.0F}}, // localId 4
    };
    MeshElement quad{};
    quad.type = MeshElementType::Quad;
    quad.nodeLocalIds = {1, 2, 3, 4, 0, 0, 0, 0};
    entry.elements = {quad};
    return entry;
}

static void applySplitResult(MeshEntry& entry, const SplitResult& result) {
    for(const auto& new_node : result.newNodes) {
        entry.nodes.push_back(
            MeshNode{{static_cast<float>(new_node.x), static_cast<float>(new_node.y),
                      static_cast<float>(new_node.z)}});
    }

    std::vector<std::size_t> sorted_indices(result.replacements.size());
    for(std::size_t index = 0; index < sorted_indices.size(); ++index) {
        sorted_indices[index] = index;
    }
    std::sort(sorted_indices.begin(), sorted_indices.end(), [&](std::size_t lhs, std::size_t rhs) {
        return result.replacements[lhs].originalIndex > result.replacements[rhs].originalIndex;
    });

    for(const auto replacement_index : sorted_indices) {
        const auto& replacement = result.replacements[replacement_index];
        auto position = entry.elements.begin() + replacement.originalIndex;
        position = entry.elements.erase(position);
        entry.elements.insert(position, replacement.newElements.begin(),
                              replacement.newElements.end());
    }
}

TEST_CASE("MeshSplitAlgorithm: triangle 1 edge -> 2 triangles") {
    const auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto edge_index = topo.findEdgeIndex(1, 2);
    REQUIRE(edge_index.has_value());

    const auto result =
        algo.compute(entry, topo, {edge_index.value() + 1U}, {}, SplitMode::TriaFour);

    CHECK(result.newNodes.size() == 1);
    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].originalIndex == 0);
    CHECK(result.replacements[0].newElements.size() == 2);

    CHECK(result.newNodes[0].x == doctest::Approx(1.0));
    CHECK(result.newNodes[0].y == doctest::Approx(0.0));

    for(const auto& element : result.replacements[0].newElements) {
        CHECK(element.type == MeshElementType::Triangle);
    }
}

TEST_CASE("MeshSplitAlgorithm: triangle 2 edges -> 3 triangles") {
    const auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto edge12 = topo.findEdgeIndex(1, 2);
    const auto edge23 = topo.findEdgeIndex(2, 3);
    REQUIRE(edge12.has_value());
    REQUIRE(edge23.has_value());

    const auto result = algo.compute(entry, topo, {edge12.value() + 1U, edge23.value() + 1U}, {},
                                     SplitMode::TriaFour);

    CHECK(result.newNodes.size() == 2);
    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 3);

    for(const auto& element : result.replacements[0].newElements) {
        CHECK(element.type == MeshElementType::Triangle);
    }
}

TEST_CASE("MeshSplitAlgorithm: triangle 3 edges TriaFour -> 4 triangles") {
    const auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    std::vector<uint32_t> all_edges;
    for(uint32_t edge_index = 0; edge_index < topo.edges.size(); ++edge_index) {
        all_edges.push_back(edge_index + 1U);
    }

    const auto result = algo.compute(entry, topo, all_edges, {}, SplitMode::TriaFour);

    CHECK(result.newNodes.size() == 3);
    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 4);

    for(const auto& element : result.replacements[0].newElements) {
        CHECK(element.type == MeshElementType::Triangle);
    }
}

TEST_CASE("MeshSplitAlgorithm: triangle 3 edges QuadThree -> 3 quads") {
    const auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    std::vector<uint32_t> all_edges;
    for(uint32_t edge_index = 0; edge_index < topo.edges.size(); ++edge_index) {
        all_edges.push_back(edge_index + 1U);
    }

    const auto result = algo.compute(entry, topo, all_edges, {}, SplitMode::QuadThree);

    CHECK(result.newNodes.size() == 4);
    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 3);

    for(const auto& element : result.replacements[0].newElements) {
        CHECK(element.type == MeshElementType::Quad);
    }
}

TEST_CASE("MeshSplitAlgorithm: neighbor propagation on shared edge") {
    const auto entry = makeTwoTrianglesSharedEdge();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto edge12 = topo.findEdgeIndex(1, 2);
    REQUIRE(edge12.has_value());

    const auto result = algo.compute(entry, topo, {edge12.value() + 1U}, {}, SplitMode::TriaFour);

    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].originalIndex == 0);
}

TEST_CASE("MeshSplitAlgorithm: neighbor propagation cuts adjacent element") {
    const auto entry = makeTwoTrianglesSharedEdge();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto edge23 = topo.findEdgeIndex(2, 3);
    REQUIRE(edge23.has_value());

    const auto result = algo.compute(entry, topo, {edge23.value() + 1U}, {}, SplitMode::TriaFour);

    CHECK(result.replacements.size() == 2);
    CHECK(result.newNodes.size() >= 1);
}

TEST_CASE("MeshSplitAlgorithm: empty selections -> empty result") {
    const auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto result = algo.compute(entry, topo, {}, {}, SplitMode::TriaFour);

    CHECK(result.newNodes.empty());
    CHECK(result.replacements.empty());
}

TEST_CASE("MeshSplitAlgorithm: applySplitResult preserves mesh integrity") {
    auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    std::vector<uint32_t> all_edges;
    for(uint32_t edge_index = 0; edge_index < topo.edges.size(); ++edge_index) {
        all_edges.push_back(edge_index + 1U);
    }

    const auto result = algo.compute(entry, topo, all_edges, {}, SplitMode::TriaFour);
    applySplitResult(entry, result);

    CHECK(entry.nodes.size() == 6);
    CHECK(entry.elements.size() == 4);

    for(const auto& element : entry.elements) {
        CHECK(element.type == MeshElementType::Triangle);
    }

    for(const auto& element : entry.elements) {
        for(uint8_t node_offset = 0; node_offset < OpenGeoLab::Mesh::nodeCount(element.type);
            ++node_offset) {
            CHECK(element.nodeLocalIds[node_offset] >= 1);
            CHECK(element.nodeLocalIds[node_offset] <= entry.nodes.size());
        }
    }
}

TEST_CASE("MeshSplitAlgorithm: quad 1 edge -> 1 quad + 1 triangle") {
    const auto entry = makeSingleQuad();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto edge12 = topo.findEdgeIndex(1, 2);
    REQUIRE(edge12.has_value());

    const auto result =
        algo.compute(entry, topo, {edge12.value() + 1U}, {}, SplitMode::TriaOneQuadThree);

    CHECK(result.newNodes.size() == 1);
    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 2);

    uint32_t tri_count = 0;
    uint32_t quad_count = 0;
    for(const auto& element : result.replacements[0].newElements) {
        if(element.type == MeshElementType::Triangle) {
            ++tri_count;
        }
        if(element.type == MeshElementType::Quad) {
            ++quad_count;
        }
    }
    CHECK(tri_count == 1);
    CHECK(quad_count == 1);
}

TEST_CASE("MeshSplitAlgorithm: quad 2 adjacent edges -> 3 elements") {
    const auto entry = makeSingleQuad();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto edge12 = topo.findEdgeIndex(1, 2);
    const auto edge23 = topo.findEdgeIndex(2, 3);
    REQUIRE(edge12.has_value());
    REQUIRE(edge23.has_value());

    const auto result = algo.compute(entry, topo, {edge12.value() + 1U, edge23.value() + 1U}, {},
                                     SplitMode::TriaOneQuadThree);

    CHECK(result.newNodes.size() == 2);
    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 3);
}

TEST_CASE("MeshSplitAlgorithm: quad 2 opposite edges -> 2 quads") {
    const auto entry = makeSingleQuad();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto edge12 = topo.findEdgeIndex(1, 2);
    const auto edge34 = topo.findEdgeIndex(3, 4);
    REQUIRE(edge12.has_value());
    REQUIRE(edge34.has_value());

    const auto result = algo.compute(entry, topo, {edge12.value() + 1U, edge34.value() + 1U}, {},
                                     SplitMode::TriaOneQuadThree);

    CHECK(result.newNodes.size() == 2);
    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 2);

    for(const auto& element : result.replacements[0].newElements) {
        CHECK(element.type == MeshElementType::Quad);
    }
}

TEST_CASE("MeshSplitAlgorithm: quad 3 edges TriaOneQuadThree -> 3 quads + 1 triangle") {
    const auto entry = makeSingleQuad();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto edge12 = topo.findEdgeIndex(1, 2);
    const auto edge23 = topo.findEdgeIndex(2, 3);
    const auto edge34 = topo.findEdgeIndex(3, 4);
    REQUIRE(edge12.has_value());
    REQUIRE(edge23.has_value());
    REQUIRE(edge34.has_value());

    const auto result =
        algo.compute(entry, topo, {edge12.value() + 1U, edge23.value() + 1U, edge34.value() + 1U},
                     {}, SplitMode::TriaOneQuadThree);

    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 4);

    uint32_t tri_count = 0;
    uint32_t quad_count = 0;
    for(const auto& element : result.replacements[0].newElements) {
        if(element.type == MeshElementType::Triangle) {
            ++tri_count;
        }
        if(element.type == MeshElementType::Quad) {
            ++quad_count;
        }
    }
    CHECK(tri_count == 1);
    CHECK(quad_count == 3);
}

TEST_CASE("MeshSplitAlgorithm: quad 3 edges TriaOneQuadTwo -> 2 quads + 1 triangle") {
    const auto entry = makeSingleQuad();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto edge12 = topo.findEdgeIndex(1, 2);
    const auto edge23 = topo.findEdgeIndex(2, 3);
    const auto edge34 = topo.findEdgeIndex(3, 4);
    REQUIRE(edge12.has_value());
    REQUIRE(edge23.has_value());
    REQUIRE(edge34.has_value());

    const auto result =
        algo.compute(entry, topo, {edge12.value() + 1U, edge23.value() + 1U, edge34.value() + 1U},
                     {}, SplitMode::TriaOneQuadTwo);

    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 3);

    uint32_t tri_count = 0;
    uint32_t quad_count = 0;
    for(const auto& element : result.replacements[0].newElements) {
        if(element.type == MeshElementType::Triangle) {
            ++tri_count;
        }
        if(element.type == MeshElementType::Quad) {
            ++quad_count;
        }
    }
    CHECK(tri_count == 1);
    CHECK(quad_count == 2);
}

TEST_CASE("MeshSplitAlgorithm: quad 3 edges TriaThreeQuadTwo -> 2 quads + 3 triangles") {
    const auto entry = makeSingleQuad();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto edge12 = topo.findEdgeIndex(1, 2);
    const auto edge23 = topo.findEdgeIndex(2, 3);
    const auto edge34 = topo.findEdgeIndex(3, 4);
    REQUIRE(edge12.has_value());
    REQUIRE(edge23.has_value());
    REQUIRE(edge34.has_value());

    const auto result =
        algo.compute(entry, topo, {edge12.value() + 1U, edge23.value() + 1U, edge34.value() + 1U},
                     {}, SplitMode::TriaThreeQuadTwo);

    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 5);

    uint32_t tri_count = 0;
    uint32_t quad_count = 0;
    for(const auto& element : result.replacements[0].newElements) {
        if(element.type == MeshElementType::Triangle) {
            ++tri_count;
        }
        if(element.type == MeshElementType::Quad) {
            ++quad_count;
        }
    }
    CHECK(tri_count == 3);
    CHECK(quad_count == 2);
}

TEST_CASE("MeshSplitAlgorithm: quad 4 edges -> 4 quads") {
    const auto entry = makeSingleQuad();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    std::vector<uint32_t> all_edges;
    for(uint32_t edge_index = 0; edge_index < topo.edges.size(); ++edge_index) {
        all_edges.push_back(edge_index + 1U);
    }

    const auto result = algo.compute(entry, topo, all_edges, {}, SplitMode::TriaOneQuadThree);

    CHECK(result.newNodes.size() == 5); // 4 midpoints + 1 center
    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].newElements.size() == 4);

    for(const auto& element : result.replacements[0].newElements) {
        CHECK(element.type == MeshElementType::Quad);
    }
}

TEST_CASE("MeshSplitAlgorithm: triangle 3 nodes TriaThree -> 3 triangles via centroid") {
    const auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto result = algo.compute(entry, topo, {}, {1, 2, 3}, SplitMode::TriaThree);
    const uint32_t centroid_local_id = static_cast<uint32_t>(entry.nodes.size()) + 1U;

    CHECK(result.newNodes.size() == 1); // 1 centroid
    CHECK(result.replacements.size() == 1);
    CHECK(result.replacements[0].originalIndex == 0);
    CHECK(result.replacements[0].newElements.size() == 3);

    for(const auto& element : result.replacements[0].newElements) {
        CHECK(element.type == MeshElementType::Triangle);
        CHECK(std::find(element.nodeLocalIds.begin(), element.nodeLocalIds.end(),
                        centroid_local_id) != element.nodeLocalIds.end());
    }

    CHECK(result.newNodes[0].x == doctest::Approx(1.0));
    CHECK(result.newNodes[0].y == doctest::Approx(2.0 / 3.0));
    CHECK(result.newNodes[0].z == doctest::Approx(0.0));
}

TEST_CASE("MeshSplitAlgorithm: node split ignores quads") {
    const auto entry = makeSingleQuad();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto result = algo.compute(entry, topo, {}, {1, 2, 3}, SplitMode::TriaThree);

    CHECK(result.newNodes.empty());
    CHECK(result.replacements.empty());
}

TEST_CASE("MeshSplitAlgorithm: node split with fewer than 3 nodes -> no split") {
    const auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto result = algo.compute(entry, topo, {}, {1, 2}, SplitMode::TriaThree);

    CHECK(result.newNodes.empty());
    CHECK(result.replacements.empty());
}

TEST_CASE("MeshSplitAlgorithm: TriaThree applySplitResult integrity") {
    auto entry = makeSingleTriangle();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto result = algo.compute(entry, topo, {}, {1, 2, 3}, SplitMode::TriaThree);
    applySplitResult(entry, result);

    CHECK(entry.nodes.size() == 4);    // 3 original + 1 centroid
    CHECK(entry.elements.size() == 3); // 3 sub-triangles

    for(const auto& element : entry.elements) {
        CHECK(element.type == MeshElementType::Triangle);
        for(uint8_t node_offset = 0; node_offset < OpenGeoLab::Mesh::nodeCount(element.type);
            ++node_offset) {
            CHECK(element.nodeLocalIds[node_offset] >= 1);
            CHECK(element.nodeLocalIds[node_offset] <= entry.nodes.size());
        }
    }
}

// ── Quad 3-edge: exact vertex verification ────────────────────────────

/// Helper: extract nodeLocalIds as a vector for easier comparison
static std::vector<uint32_t> nodeIds(const MeshElement& element) {
    const auto count = OpenGeoLab::Mesh::nodeCount(element.type);
    return {element.nodeLocalIds.begin(), element.nodeLocalIds.begin() + count};
}

/// Helper: create a 2x2 grid of quads (9 nodes, 4 quads)
/// Layout (node localIds, 1-based):
///   7---8---9
///   |   |   |
///   4---5---6
///   |   |   |
///   1---2---3
static MeshEntry makeQuadGrid2x2() {
    MeshEntry entry;
    entry.shapeId = 1;
    entry.nodes = {
        MeshNode{{0.0F, 0.0F, 0.0F}}, // 1
        MeshNode{{1.0F, 0.0F, 0.0F}}, // 2
        MeshNode{{2.0F, 0.0F, 0.0F}}, // 3
        MeshNode{{0.0F, 1.0F, 0.0F}}, // 4
        MeshNode{{1.0F, 1.0F, 0.0F}}, // 5
        MeshNode{{2.0F, 1.0F, 0.0F}}, // 6
        MeshNode{{0.0F, 2.0F, 0.0F}}, // 7
        MeshNode{{1.0F, 2.0F, 0.0F}}, // 8
        MeshNode{{2.0F, 2.0F, 0.0F}}, // 9
    };

    // Quads: bottom-left, bottom-right, top-left, top-right
    MeshElement q0{};
    q0.type = MeshElementType::Quad;
    q0.nodeLocalIds = {1, 2, 5, 4, 0, 0, 0, 0};

    MeshElement q1{};
    q1.type = MeshElementType::Quad;
    q1.nodeLocalIds = {2, 3, 6, 5, 0, 0, 0, 0};

    MeshElement q2{};
    q2.type = MeshElementType::Quad;
    q2.nodeLocalIds = {4, 5, 8, 7, 0, 0, 0, 0};

    MeshElement q3{};
    q3.type = MeshElementType::Quad;
    q3.nodeLocalIds = {5, 6, 9, 8, 0, 0, 0, 0};

    entry.elements = {q0, q1, q2, q3};
    return entry;
}

TEST_CASE("MeshSplitAlgorithm: quad 3-edge bitmask combined mode extraction") {
    const auto entry = makeSingleQuad();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto edge12 = topo.findEdgeIndex(1, 2);
    const auto edge23 = topo.findEdgeIndex(2, 3);
    const auto edge34 = topo.findEdgeIndex(3, 4);
    REQUIRE(edge12.has_value());
    REQUIRE(edge23.has_value());
    REQUIRE(edge34.has_value());

    const std::vector<uint32_t> edges = {edge12.value() + 1U, edge23.value() + 1U,
                                         edge34.value() + 1U};

    SUBCASE("combined mode 9 (TriaOneQuadThree=1 | TriaFour=8) -> 3Q+1T") {
        const auto result = algo.compute(entry, topo, edges, {}, static_cast<SplitMode>(9));

        CHECK(result.replacements.size() == 1);
        REQUIRE(result.replacements[0].newElements.size() == 4);

        uint32_t tri_count = 0;
        uint32_t quad_count = 0;
        for(const auto& elem : result.replacements[0].newElements) {
            if(elem.type == MeshElementType::Triangle)
                ++tri_count;
            if(elem.type == MeshElementType::Quad)
                ++quad_count;
        }
        CHECK(tri_count == 1);
        CHECK(quad_count == 3);
    }

    SUBCASE("combined mode 10 (TriaOneQuadTwo=2 | TriaFour=8) -> 2Q+1T") {
        const auto result = algo.compute(entry, topo, edges, {}, static_cast<SplitMode>(10));

        CHECK(result.replacements.size() == 1);
        REQUIRE(result.replacements[0].newElements.size() == 3);

        uint32_t tri_count = 0;
        uint32_t quad_count = 0;
        for(const auto& elem : result.replacements[0].newElements) {
            if(elem.type == MeshElementType::Triangle)
                ++tri_count;
            if(elem.type == MeshElementType::Quad)
                ++quad_count;
        }
        CHECK(tri_count == 1);
        CHECK(quad_count == 2);
    }

    SUBCASE("combined mode 12 (TriaThreeQuadTwo=4 | TriaFour=8) -> 2Q+3T") {
        const auto result = algo.compute(entry, topo, edges, {}, static_cast<SplitMode>(12));

        CHECK(result.replacements.size() == 1);
        REQUIRE(result.replacements[0].newElements.size() == 5);

        uint32_t tri_count = 0;
        uint32_t quad_count = 0;
        for(const auto& elem : result.replacements[0].newElements) {
            if(elem.type == MeshElementType::Triangle)
                ++tri_count;
            if(elem.type == MeshElementType::Quad)
                ++quad_count;
        }
        CHECK(tri_count == 3);
        CHECK(quad_count == 2);
    }
}

TEST_CASE("MeshSplitAlgorithm: quad 3-edge exact vertex positions") {
    // Quad: n1(0,0) n2(2,0) n3(2,2) n4(0,2), edges 1-2, 2-3, 3-4 selected
    // unsel=3 (side 4→1)
    // Corners: ca=n1(0,0) cb=n2(2,0) cc=n3(2,2) cd=n4(0,2)
    // Midpoints: mid_ab=(1,0) mid_bc=(2,1) mid_cd=(1,2)
    // Center: (1,1)
    const auto entry = makeSingleQuad();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto edge12 = topo.findEdgeIndex(1, 2);
    const auto edge23 = topo.findEdgeIndex(2, 3);
    const auto edge34 = topo.findEdgeIndex(3, 4);
    REQUIRE(edge12.has_value());
    REQUIRE(edge23.has_value());
    REQUIRE(edge34.has_value());

    const std::vector<uint32_t> edges = {edge12.value() + 1U, edge23.value() + 1U,
                                         edge34.value() + 1U};

    SUBCASE("TriaOneQuadThree exact vertices") {
        const auto result = algo.compute(entry, topo, edges, {}, SplitMode::TriaOneQuadThree);

        // New nodes: mid_ab(1,0)=5 mid_bc(2,1)=6 mid_cd(1,2)=7 center(1,1)=8
        REQUIRE(result.newNodes.size() == 4);
        CHECK(result.newNodes[0].x == doctest::Approx(1.0));
        CHECK(result.newNodes[0].y == doctest::Approx(0.0));
        CHECK(result.newNodes[1].x == doctest::Approx(2.0));
        CHECK(result.newNodes[1].y == doctest::Approx(1.0));
        CHECK(result.newNodes[2].x == doctest::Approx(1.0));
        CHECK(result.newNodes[2].y == doctest::Approx(2.0));
        CHECK(result.newNodes[3].x == doctest::Approx(1.0));
        CHECK(result.newNodes[3].y == doctest::Approx(1.0));

        REQUIRE(result.replacements.size() == 1);
        const auto& elems = result.replacements[0].newElements;
        REQUIRE(elems.size() == 4);

        // Q1: mid_ab(5), cb(2), mid_bc(6), center(8)
        CHECK(elems[0].type == MeshElementType::Quad);
        CHECK(nodeIds(elems[0]) == std::vector<uint32_t>{5, 2, 6, 8});

        // Q2: mid_bc(6), cc(3), mid_cd(7), center(8)
        CHECK(elems[1].type == MeshElementType::Quad);
        CHECK(nodeIds(elems[1]) == std::vector<uint32_t>{6, 3, 7, 8});

        // Q3: mid_cd(7), cd(4), ca(1), center(8)
        CHECK(elems[2].type == MeshElementType::Quad);
        CHECK(nodeIds(elems[2]) == std::vector<uint32_t>{7, 4, 1, 8});

        // T1: ca(1), mid_ab(5), center(8)
        CHECK(elems[3].type == MeshElementType::Triangle);
        CHECK(nodeIds(elems[3]) == std::vector<uint32_t>{1, 5, 8});
    }

    SUBCASE("TriaOneQuadTwo exact vertices") {
        const auto result = algo.compute(entry, topo, edges, {}, SplitMode::TriaOneQuadTwo);

        // New nodes: mid_ab(1,0)=5 mid_bc(2,1)=6 mid_cd(1,2)=7 (no center)
        REQUIRE(result.newNodes.size() == 3);

        REQUIRE(result.replacements.size() == 1);
        const auto& elems = result.replacements[0].newElements;
        REQUIRE(elems.size() == 3);

        // Q1: mid_ab(5), cb(2), mid_bc(6), mid_cd(7)
        CHECK(elems[0].type == MeshElementType::Quad);
        CHECK(nodeIds(elems[0]) == std::vector<uint32_t>{5, 2, 6, 7});

        // T1: mid_bc(6), cc(3), mid_cd(7)
        CHECK(elems[1].type == MeshElementType::Triangle);
        CHECK(nodeIds(elems[1]) == std::vector<uint32_t>{6, 3, 7});

        // Q2: mid_ab(5), mid_cd(7), cd(4), ca(1)
        CHECK(elems[2].type == MeshElementType::Quad);
        CHECK(nodeIds(elems[2]) == std::vector<uint32_t>{5, 7, 4, 1});
    }

    SUBCASE("TriaThreeQuadTwo exact vertices") {
        const auto result = algo.compute(entry, topo, edges, {}, SplitMode::TriaThreeQuadTwo);

        // New nodes: mid_ab=5 mid_bc=6 mid_cd=7 center=8
        REQUIRE(result.newNodes.size() == 4);

        REQUIRE(result.replacements.size() == 1);
        const auto& elems = result.replacements[0].newElements;
        REQUIRE(elems.size() == 5);

        // Q1: mid_ab(5), cb(2), mid_bc(6), center(8)
        CHECK(elems[0].type == MeshElementType::Quad);
        CHECK(nodeIds(elems[0]) == std::vector<uint32_t>{5, 2, 6, 8});

        // Q2: mid_bc(6), cc(3), mid_cd(7), center(8)
        CHECK(elems[1].type == MeshElementType::Quad);
        CHECK(nodeIds(elems[1]) == std::vector<uint32_t>{6, 3, 7, 8});

        // T1: mid_cd(7), cd(4), center(8)
        CHECK(elems[2].type == MeshElementType::Triangle);
        CHECK(nodeIds(elems[2]) == std::vector<uint32_t>{7, 4, 8});

        // T2: cd(4), ca(1), center(8)
        CHECK(elems[3].type == MeshElementType::Triangle);
        CHECK(nodeIds(elems[3]) == std::vector<uint32_t>{4, 1, 8});

        // T3: ca(1), mid_ab(5), center(8)
        CHECK(elems[4].type == MeshElementType::Triangle);
        CHECK(nodeIds(elems[4]) == std::vector<uint32_t>{1, 5, 8});
    }
}

TEST_CASE("MeshSplitAlgorithm: quad 3-edge in 2x2 grid with neighbor cuts") {
    // Select 3 edges of bottom-left quad (elem 0): sides 0(1-2), 1(2-5), 2(5-4)
    // This leaves side 3(4-1) unselected
    // Neighbors: elem 1 (shares edge 2-5), elem 2 (shares edge 4-5)
    // Note: edge 1-2 is boundary (no neighbor beyond bottom)
    const auto entry = makeQuadGrid2x2();
    const auto topo = MeshTopology::build(entry);
    const MeshSplitAlgorithm algo;

    const auto e12 = topo.findEdgeIndex(1, 2);
    const auto e25 = topo.findEdgeIndex(2, 5);
    const auto e45 = topo.findEdgeIndex(4, 5);
    REQUIRE(e12.has_value());
    REQUIRE(e25.has_value());
    REQUIRE(e45.has_value());

    const std::vector<uint32_t> edges = {e12.value() + 1U, e25.value() + 1U, e45.value() + 1U};

    SUBCASE("TriaOneQuadThree grid: correct element+neighbor counts") {
        const auto result = algo.compute(entry, topo, edges, {}, SplitMode::TriaOneQuadThree);

        // Should have: 1 replacement for selected quad + up to 2 neighbor cuts
        // (edge 1-2 has no other neighbor since it's boundary of elem0 only)
        // Neighbor of edge 2-5: elem 1 (bottom-right quad)
        // Neighbor of edge 4-5: elem 2 (top-left quad)
        // Edge 1-2: neighbors are [elem0] only in bottom-left quad, no other neighbor
        uint32_t selected_replacements = 0;
        uint32_t neighbor_replacements = 0;
        for(const auto& rep : result.replacements) {
            if(rep.originalIndex == 0) {
                ++selected_replacements;
            } else {
                ++neighbor_replacements;
            }
        }
        CHECK(selected_replacements == 1);
        CHECK(neighbor_replacements == 2); // elem 1 and elem 2

        // Verify all generated elements have valid node IDs
        applySplitResult(const_cast<MeshEntry&>(entry), result);
        // After split: original 9 nodes + new midpoints + centroid
        for(const auto& element : entry.elements) {
            for(uint8_t i = 0; i < OpenGeoLab::Mesh::nodeCount(element.type); ++i) {
                CHECK(element.nodeLocalIds[i] >= 1);
                CHECK(element.nodeLocalIds[i] <= entry.nodes.size());
            }
        }
    }

    SUBCASE("TriaOneQuadThree grid: applySplitResult positions valid") {
        auto mutable_entry = entry;
        const auto result = algo.compute(entry, topo, edges, {}, SplitMode::TriaOneQuadThree);
        applySplitResult(mutable_entry, result);

        // All node references should point to valid positions
        for(const auto& element : mutable_entry.elements) {
            for(uint8_t i = 0; i < OpenGeoLab::Mesh::nodeCount(element.type); ++i) {
                const auto lid = element.nodeLocalIds[i];
                REQUIRE(lid >= 1);
                REQUIRE(lid <= mutable_entry.nodes.size());
                // Verify position is finite
                const auto& pos = mutable_entry.nodes[lid - 1].position;
                CHECK(pos[0] >= 0.0F);
                CHECK(pos[0] <= 2.0F);
                CHECK(pos[1] >= 0.0F);
                CHECK(pos[1] <= 2.0F);
            }
        }
    }
}
