/// @file mesh_entry_test.cpp
/// @brief Unit tests for ElementLocator and MeshEntry.

#include <doctest/doctest.h>

#include <opengeolab/mesh/mesh_entry.hpp>

using OpenGeoLab::Mesh::ElementBlock;
using OpenGeoLab::Mesh::ElementLocator;
using OpenGeoLab::Mesh::ElementType;
using OpenGeoLab::Mesh::MeshEntry;

// ---------------------------------------------------------------------------
// ElementLocator::build + locate
// ---------------------------------------------------------------------------

TEST_CASE("ElementLocator single surface block") {
    ElementBlock block;
    block.type = ElementType::Triangle3;
    block.connectivity = {1, 2, 3, 4, 5, 6, 7, 8, 9}; // 3 triangles

    ElementLocator locator;
    locator.build({}, {block}, {});

    CHECK(locator.totalCount() == 3);

    // Element IDs are 1-based
    auto loc1 = locator.locate(1);
    CHECK(loc1.group == ElementLocator::Location::Group::Surface);
    CHECK(loc1.blockIndex == 0);
    CHECK(loc1.localIndex == 0);

    auto loc2 = locator.locate(2);
    CHECK(loc2.group == ElementLocator::Location::Group::Surface);
    CHECK(loc2.blockIndex == 0);
    CHECK(loc2.localIndex == 1);

    auto loc3 = locator.locate(3);
    CHECK(loc3.group == ElementLocator::Location::Group::Surface);
    CHECK(loc3.blockIndex == 0);
    CHECK(loc3.localIndex == 2);
}

TEST_CASE("ElementLocator multiple blocks across groups") {
    // 2 line elements (1 block)
    ElementBlock line_block;
    line_block.type = ElementType::Line2;
    line_block.connectivity = {1, 2, 3, 4}; // 2 lines

    // 3 triangle elements (1 block)
    ElementBlock surf_block;
    surf_block.type = ElementType::Triangle3;
    surf_block.connectivity = {1, 2, 3, 4, 5, 6, 7, 8, 9}; // 3 triangles

    // 2 quad elements (1 block)
    ElementBlock surf_block2;
    surf_block2.type = ElementType::Quad4;
    surf_block2.connectivity = {1, 2, 3, 4, 5, 6, 7, 8}; // 2 quads

    // 1 tetra element (1 block)
    ElementBlock vol_block;
    vol_block.type = ElementType::Tetra4;
    vol_block.connectivity = {1, 2, 3, 4}; // 1 tet

    ElementLocator locator;
    locator.build({line_block}, {surf_block, surf_block2}, {vol_block});

    CHECK(locator.totalCount() == 8); // 2 + 3 + 2 + 1

    // Element 1-2: line block
    auto loc1 = locator.locate(1);
    CHECK(loc1.group == ElementLocator::Location::Group::Line);
    CHECK(loc1.blockIndex == 0);
    CHECK(loc1.localIndex == 0);

    auto loc2 = locator.locate(2);
    CHECK(loc2.group == ElementLocator::Location::Group::Line);
    CHECK(loc2.blockIndex == 0);
    CHECK(loc2.localIndex == 1);

    // Element 3-5: first surface block
    auto loc3 = locator.locate(3);
    CHECK(loc3.group == ElementLocator::Location::Group::Surface);
    CHECK(loc3.blockIndex == 0);
    CHECK(loc3.localIndex == 0);

    auto loc5 = locator.locate(5);
    CHECK(loc5.group == ElementLocator::Location::Group::Surface);
    CHECK(loc5.blockIndex == 0);
    CHECK(loc5.localIndex == 2);

    // Element 6-7: second surface block
    auto loc6 = locator.locate(6);
    CHECK(loc6.group == ElementLocator::Location::Group::Surface);
    CHECK(loc6.blockIndex == 1);
    CHECK(loc6.localIndex == 0);

    // Element 8: volume block
    auto loc8 = locator.locate(8);
    CHECK(loc8.group == ElementLocator::Location::Group::Volume);
    CHECK(loc8.blockIndex == 0);
    CHECK(loc8.localIndex == 0);
}

TEST_CASE("ElementLocator empty blocks") {
    ElementLocator locator;
    locator.build({}, {}, {});
    CHECK(locator.totalCount() == 0);
}

TEST_CASE("ElementLocator single element") {
    ElementBlock block;
    block.type = ElementType::Tetra4;
    block.connectivity = {1, 2, 3, 4};

    ElementLocator locator;
    locator.build({}, {}, {block});

    CHECK(locator.totalCount() == 1);

    auto loc = locator.locate(1);
    CHECK(loc.group == ElementLocator::Location::Group::Volume);
    CHECK(loc.blockIndex == 0);
    CHECK(loc.localIndex == 0);
}

// ---------------------------------------------------------------------------
// MeshEntry convenience methods
// ---------------------------------------------------------------------------

TEST_CASE("MeshEntry nodeCount and elementCount") {
    MeshEntry entry;
    entry.nodes.coords = {0, 0, 0, 1, 0, 0, 0, 1, 0}; // 3 nodes

    ElementBlock block;
    block.type = ElementType::Triangle3;
    block.connectivity = {1, 2, 3};
    entry.surfaceBlocks.push_back(block);
    entry.elementLocator.build(entry.lineBlocks, entry.surfaceBlocks, entry.volumeBlocks);

    CHECK(entry.nodeCount() == 3);
    CHECK(entry.elementCount() == 1);
}
