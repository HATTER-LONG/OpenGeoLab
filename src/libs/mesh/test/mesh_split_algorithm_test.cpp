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
