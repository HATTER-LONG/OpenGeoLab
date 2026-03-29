/// @file gmsh_bridge_test.cpp
/// @brief Integration tests for GmshBridge — real Gmsh mesh generation on OCC shapes.

#include <doctest/doctest.h>

#include <opengeolab/mesh/gmsh_bridge.hpp>
#include <opengeolab/mesh/mesh_params.hpp>
#include <opengeolab/mesh/mesh_visual_builder.hpp>

#include <opengeolab/core/progress_callback.hpp>

#include <BRepPrimAPI_MakeBox.hxx>

using OpenGeoLab::Core::NO_PROGRESS_CALLBACK;
using OpenGeoLab::Mesh::nodesPerElement;
using OpenGeoLab::Mesh::SurfaceMeshParams;
using OpenGeoLab::Mesh::VolumeMeshParams;
using OpenGeoLab::Mesh::GmshBridge::generateSurfaceMesh;
using OpenGeoLab::Mesh::GmshBridge::generateVolumeMesh;

/// Helper: create a unit box shape.
static TopoDS_Shape makeBox(double w = 10.0, double h = 10.0, double d = 10.0) {
    return BRepPrimAPI_MakeBox(w, h, d).Shape();
}

// ---------------------------------------------------------------------------
// Surface mesh generation
// ---------------------------------------------------------------------------

TEST_CASE("GmshBridge generateSurfaceMesh produces valid mesh") {
    auto shape = makeBox();
    SurfaceMeshParams params;
    params.minSize = 1.0;
    params.maxSize = 5.0;

    auto entry = generateSurfaceMesh(shape, params, NO_PROGRESS_CALLBACK);

    // Must have nodes
    CHECK(entry.nodeCount() > 0);
    // Must have surface element blocks
    CHECK_FALSE(entry.surfaceBlocks.empty());
    // Must have elements
    CHECK(entry.elementCount() > 0);

    // Verify node coords dimension
    CHECK(entry.nodes.coords.size() == entry.nodeCount() * 3);

    // Verify each surface block has valid connectivity
    for(const auto& block : entry.surfaceBlocks) {
        auto npe = nodesPerElement(block.type);
        CHECK(block.connectivity.size() % npe == 0);
        CHECK(block.elementCount() > 0);
    }
}

// ---------------------------------------------------------------------------
// Volume mesh generation
// ---------------------------------------------------------------------------

TEST_CASE("GmshBridge generateVolumeMesh produces valid mesh") {
    auto shape = makeBox();
    VolumeMeshParams params;
    params.minSize = 1.0;
    params.maxSize = 5.0;

    auto entry = generateVolumeMesh(shape, params, NO_PROGRESS_CALLBACK);

    // Must have nodes
    CHECK(entry.nodeCount() > 0);
    // Must have volume element blocks
    CHECK_FALSE(entry.volumeBlocks.empty());
    // Must have elements
    CHECK(entry.elementCount() > 0);

    // Verify each volume block
    for(const auto& block : entry.volumeBlocks) {
        auto npe = nodesPerElement(block.type);
        CHECK(block.connectivity.size() % npe == 0);
        CHECK(block.elementCount() > 0);
    }

    // Volume mesh should also produce surface elements (boundary)
    CHECK_FALSE(entry.surfaceBlocks.empty());
}

// ---------------------------------------------------------------------------
// ElementLocator integration
// ---------------------------------------------------------------------------

TEST_CASE("GmshBridge result has working ElementLocator") {
    auto shape = makeBox();
    SurfaceMeshParams params;
    params.minSize = 2.0;
    params.maxSize = 5.0;

    auto entry = generateSurfaceMesh(shape, params, NO_PROGRESS_CALLBACK);
    REQUIRE(entry.elementCount() > 0);

    // Verify locate works for first and last element
    auto loc1 = entry.elementLocator.locate(1);
    CHECK(loc1.localIndex == 0);

    auto loc_last = entry.elementLocator.locate(entry.elementCount());
    // Just verify it doesn't crash and returns valid indices
    CHECK(loc_last.localIndex < 1000000); // sanity bound
}

// ---------------------------------------------------------------------------
// MeshVisualBuilder integration
// ---------------------------------------------------------------------------

TEST_CASE("MeshVisualBuilder produces valid VisualData from surface mesh") {
    auto shape = makeBox();
    SurfaceMeshParams params;
    params.minSize = 2.0;
    params.maxSize = 5.0;

    auto entry = generateSurfaceMesh(shape, params, NO_PROGRESS_CALLBACK);
    REQUIRE(entry.elementCount() > 0);

    auto visual = OpenGeoLab::Mesh::MeshVisualBuilder::buildVisualData(entry);

    // Must have at least one SurfaceMesh from 2D surface blocks
    REQUIRE_FALSE(visual.surfaces.empty());
    CHECK(visual.surfaces[0].positions.size() > 0);
    CHECK(visual.surfaces[0].normals.size() == visual.surfaces[0].positions.size());
    CHECK(visual.surfaces[0].indices.size() > 0);
    // Indices must be multiple of 3 (triangles)
    CHECK(visual.surfaces[0].indices.size() % 3 == 0);

    // Must have wireframe edges
    REQUIRE_FALSE(visual.edges.empty());
    CHECK(visual.edges[0].positions.size() > 0);
    CHECK(visual.edges[0].indices.size() > 0);
    CHECK(visual.edges[0].indices.size() % 2 == 0);

    // Must have point cloud
    REQUIRE_FALSE(visual.points.empty());
    CHECK(visual.points[0].positions.size() == entry.nodeCount() * 3);
}

TEST_CASE("MeshVisualBuilder produces valid VisualData from volume mesh") {
    auto shape = makeBox();
    VolumeMeshParams params;
    params.minSize = 2.0;
    params.maxSize = 5.0;

    auto entry = generateVolumeMesh(shape, params, NO_PROGRESS_CALLBACK);
    REQUIRE(entry.elementCount() > 0);
    REQUIRE_FALSE(entry.volumeBlocks.empty());

    auto visual = OpenGeoLab::Mesh::MeshVisualBuilder::buildVisualData(entry);

    // Surface mesh from both 2D surface blocks and 3D boundary faces
    REQUIRE(visual.surfaces.size() >= 1);

    // At least the volume boundary surface should have triangles
    bool has_triangles = false;
    for(const auto& surf : visual.surfaces) {
        if(surf.indices.size() > 0) {
            CHECK(surf.indices.size() % 3 == 0);
            has_triangles = true;
        }
    }
    CHECK(has_triangles);

    // Must have wireframe edges
    REQUIRE_FALSE(visual.edges.empty());
    CHECK(visual.edges[0].indices.size() % 2 == 0);

    // Must have point cloud
    REQUIRE_FALSE(visual.points.empty());
}

TEST_CASE("MeshVisualBuilder buildEntityTags has correct counts") {
    auto shape = makeBox();
    SurfaceMeshParams params;
    params.minSize = 2.0;
    params.maxSize = 5.0;

    auto entry = generateSurfaceMesh(shape, params, NO_PROGRESS_CALLBACK);
    REQUIRE(entry.elementCount() > 0);

    auto tags = OpenGeoLab::Mesh::MeshVisualBuilder::buildEntityTags(entry);

    // One node tag per mesh node
    CHECK(tags.nodeTags.size() == entry.nodeCount());
    // Verify first node tag
    CHECK(tags.nodeTags[0].type == OpenGeoLab::Core::EntityType::MeshNode);
    CHECK(tags.nodeTags[0].localId == 1);

    // Total element tags (edge + element) should match total element count
    CHECK(tags.edgeTags.size() + tags.elementTags.size() == entry.elementCount());
}
