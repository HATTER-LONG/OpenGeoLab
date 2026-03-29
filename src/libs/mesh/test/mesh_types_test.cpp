/// @file mesh_types_test.cpp
/// @brief Unit tests for mesh_types.hpp helpers.

#include <doctest/doctest.h>

#include <opengeolab/mesh/mesh_types.hpp>

using OpenGeoLab::Mesh::elementDimension;
using OpenGeoLab::Mesh::ElementType;
using OpenGeoLab::Mesh::MeshNodeArray;
using OpenGeoLab::Mesh::nodesPerElement;

// ---------------------------------------------------------------------------
// nodesPerElement
// ---------------------------------------------------------------------------

TEST_CASE("nodesPerElement returns correct value for linear types") {
    CHECK(nodesPerElement(ElementType::Line2) == 2);
    CHECK(nodesPerElement(ElementType::Triangle3) == 3);
    CHECK(nodesPerElement(ElementType::Quad4) == 4);
    CHECK(nodesPerElement(ElementType::Tetra4) == 4);
    CHECK(nodesPerElement(ElementType::Hexa8) == 8);
    CHECK(nodesPerElement(ElementType::Prism6) == 6);
    CHECK(nodesPerElement(ElementType::Pyramid5) == 5);
}

TEST_CASE("nodesPerElement returns correct value for quadratic types") {
    CHECK(nodesPerElement(ElementType::Line3) == 3);
    CHECK(nodesPerElement(ElementType::Triangle6) == 6);
    CHECK(nodesPerElement(ElementType::Quad9) == 9);
    CHECK(nodesPerElement(ElementType::Tetra10) == 10);
    CHECK(nodesPerElement(ElementType::Hexa27) == 27);
    CHECK(nodesPerElement(ElementType::Prism18) == 18);
    CHECK(nodesPerElement(ElementType::Pyramid14) == 14);
}

// ---------------------------------------------------------------------------
// elementDimension
// ---------------------------------------------------------------------------

TEST_CASE("elementDimension returns correct topological dimension") {
    CHECK(elementDimension(ElementType::Line2) == 1);
    CHECK(elementDimension(ElementType::Line3) == 1);

    CHECK(elementDimension(ElementType::Triangle3) == 2);
    CHECK(elementDimension(ElementType::Quad4) == 2);
    CHECK(elementDimension(ElementType::Triangle6) == 2);
    CHECK(elementDimension(ElementType::Quad9) == 2);

    CHECK(elementDimension(ElementType::Tetra4) == 3);
    CHECK(elementDimension(ElementType::Hexa8) == 3);
    CHECK(elementDimension(ElementType::Prism6) == 3);
    CHECK(elementDimension(ElementType::Pyramid5) == 3);
    CHECK(elementDimension(ElementType::Tetra10) == 3);
    CHECK(elementDimension(ElementType::Hexa27) == 3);
    CHECK(elementDimension(ElementType::Prism18) == 3);
    CHECK(elementDimension(ElementType::Pyramid14) == 3);
}

// ---------------------------------------------------------------------------
// MeshNodeArray
// ---------------------------------------------------------------------------

TEST_CASE("MeshNodeArray count and position") {
    MeshNodeArray nodes;
    nodes.coords = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};

    CHECK(nodes.count() == 3);

    auto p1 = nodes.position(1);
    CHECK(p1[0] == doctest::Approx(1.0));
    CHECK(p1[1] == doctest::Approx(2.0));
    CHECK(p1[2] == doctest::Approx(3.0));

    auto p2 = nodes.position(2);
    CHECK(p2[0] == doctest::Approx(4.0));
    CHECK(p2[1] == doctest::Approx(5.0));
    CHECK(p2[2] == doctest::Approx(6.0));

    auto p3 = nodes.position(3);
    CHECK(p3[0] == doctest::Approx(7.0));
    CHECK(p3[1] == doctest::Approx(8.0));
    CHECK(p3[2] == doctest::Approx(9.0));
}

TEST_CASE("MeshNodeArray empty") {
    MeshNodeArray const nodes;
    CHECK(nodes.count() == 0);
}

// ---------------------------------------------------------------------------
// ElementBlock
// ---------------------------------------------------------------------------

TEST_CASE("ElementBlock elementCount") {
    OpenGeoLab::Mesh::ElementBlock block;
    block.type = ElementType::Triangle3;
    block.connectivity = {1, 2, 3, 4, 5, 6};
    CHECK(block.elementCount() == 2);
    CHECK(block.nodesPerElem() == 3);
}
