/**
 * @file geometry_document_test.cpp
 * @brief Unit tests for GeometryDocument functionality
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "geometry/geometry_document.hpp"
#include "geometry/part_entity.hpp"
#include "geometry/shape_builder.hpp"
#include "geometry/solid_entity.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>

using namespace OpenGeoLab::Geometry;

TEST_CASE("GeometryDocument basic operations", "[geometry][document]") {
    auto doc = GeometryDocument::create();

    SECTION("Document is initially empty") {
        REQUIRE(doc->entityCount() == 0);
        REQUIRE(doc->partCount() == 0);
        REQUIRE(doc->getAllParts().empty());
    }

    SECTION("Create box appends to document") {
        auto part = doc->createBox(10.0, 20.0, 30.0, "TestBox");

        REQUIRE(part != nullptr);
        REQUIRE(part->name() == "TestBox");
        REQUIRE(doc->partCount() == 1);
        REQUIRE(doc->entityCount() > 0);

        // Check bounding box
        auto bbox = part->boundingBox();
        REQUIRE(bbox.isValid());
        REQUIRE_THAT(bbox.size().m_x, Catch::Matchers::WithinAbs(10.0, 0.01));
        REQUIRE_THAT(bbox.size().m_y, Catch::Matchers::WithinAbs(20.0, 0.01));
        REQUIRE_THAT(bbox.size().m_z, Catch::Matchers::WithinAbs(30.0, 0.01));
    }

    SECTION("Create sphere appends to document") {
        auto part = doc->createSphere(5.0, "TestSphere");

        REQUIRE(part != nullptr);
        REQUIRE(part->name() == "TestSphere");
        REQUIRE(doc->partCount() == 1);
    }

    SECTION("Create cylinder appends to document") {
        auto part = doc->createCylinder(3.0, 10.0, "TestCylinder");

        REQUIRE(part != nullptr);
        REQUIRE(part->name() == "TestCylinder");
        REQUIRE(doc->partCount() == 1);
    }

    SECTION("Multiple parts can be added") {
        auto box = doc->createBox(10.0, 10.0, 10.0, "Box1");
        auto sphere = doc->createSphere(5.0, "Sphere1");

        REQUIRE(doc->partCount() == 2);

        auto parts = doc->getAllParts();
        REQUIRE(parts.size() == 2);
    }

    SECTION("Find part by name") {
        doc->createBox(10.0, 10.0, 10.0, "FindMe");
        doc->createSphere(5.0, "NotMe");

        auto found = doc->findPartByName("FindMe");
        REQUIRE(found != nullptr);
        REQUIRE(found->name() == "FindMe");

        auto not_found = doc->findPartByName("DoesNotExist");
        REQUIRE(not_found == nullptr);
    }

    SECTION("Clear document removes all entities") {
        doc->createBox(10.0, 10.0, 10.0, "Box");
        doc->createSphere(5.0, "Sphere");

        REQUIRE(doc->entityCount() > 0);

        doc->clear();

        REQUIRE(doc->entityCount() == 0);
        REQUIRE(doc->partCount() == 0);
    }
}

TEST_CASE("GeometryDocument append shape", "[geometry][document]") {
    auto doc = GeometryDocument::create();

    SECTION("Append OCC shape creates part with hierarchy") {
        BRepPrimAPI_MakeBox maker(10.0, 20.0, 30.0);
        maker.Build();
        REQUIRE(maker.IsDone());

        auto part = doc->appendShape(maker.Shape(), "AppendedBox");

        REQUIRE(part != nullptr);
        REQUIRE(doc->partCount() == 1);

        // Part should have children (the solid, faces, etc.)
        REQUIRE(part->hasChildren());
    }
}

TEST_CASE("GeometryDocumentManager operations", "[geometry][document]") {
    auto& manager = GeometryDocumentManager::instance();

    SECTION("newDocument creates fresh document") {
        manager.newDocument();
        auto doc = manager.currentDocument();

        REQUIRE(doc != nullptr);
        REQUIRE(doc->entityCount() == 0);
    }

    SECTION("currentDocument returns same document") {
        manager.newDocument();
        auto doc1 = manager.currentDocument();
        auto doc2 = manager.currentDocument();

        REQUIRE(doc1 == doc2);
    }

    SECTION("clearCurrentDocument empties document") {
        auto doc = manager.currentDocument();
        doc->createBox(10.0, 10.0, 10.0, "TestBox");

        REQUIRE(manager.partCount() > 0);

        manager.clearCurrentDocument();

        REQUIRE(manager.partCount() == 0);
    }
}

TEST_CASE("ShapeBuilder creates entity hierarchy", "[geometry][builder]") {
    auto doc = GeometryDocument::create();

    SECTION("Build from box creates proper hierarchy") {
        BRepPrimAPI_MakeBox maker(10.0, 10.0, 10.0);
        maker.Build();
        REQUIRE(maker.IsDone());

        ShapeBuilder builder(doc);
        auto result = builder.buildFromShape(maker.Shape(), "TestPart");

        REQUIRE(result.m_success);
        REQUIRE(result.m_rootPart != nullptr);
        REQUIRE(result.m_faceCount == 6); // Box has 6 faces
        // Note: Edge count is 24 because each of the 12 unique edges is referenced
        // by 2 faces (shared edges), and we count each reference in the hierarchy.
        REQUIRE(result.m_edgeCount == 24); // 12 unique edges × 2 references each
        // Each edge has 2 vertices, so 24 edges × 2 = 48 vertex references
        REQUIRE(result.m_vertexCount == 48);
    }

    SECTION("Build with render data option") {
        BRepPrimAPI_MakeSphere maker(5.0);
        maker.Build();
        REQUIRE(maker.IsDone());

        ShapeBuilder builder(doc);
        auto options = ShapeBuildOptions::forRendering();
        auto result = builder.buildFromShape(maker.Shape(), "TestSphere", options);

        REQUIRE(result.m_success);
        REQUIRE(result.m_renderData != nullptr);
        REQUIRE(result.m_renderData->totalTriangleCount() > 0);
    }

    SECTION("Build with mesh metadata option") {
        BRepPrimAPI_MakeBox maker(10.0, 10.0, 10.0);
        maker.Build();
        REQUIRE(maker.IsDone());

        ShapeBuilder builder(doc);
        auto options = ShapeBuildOptions::forMeshing();
        auto result = builder.buildFromShape(maker.Shape(), "TestBox", options);

        REQUIRE(result.m_success);
        REQUIRE(result.m_meshMetadata != nullptr);
        REQUIRE(result.m_meshMetadata->m_faces.size() == 6);
    }
}

TEST_CASE("RenderData generation", "[geometry][render]") {
    auto doc = GeometryDocument::create();

    SECTION("Generate render data for document") {
        doc->createBox(10.0, 10.0, 10.0, "Box1");
        doc->createSphere(5.0, "Sphere1");

        auto render_data = doc->generateRenderData();

        REQUIRE(render_data != nullptr);
        REQUIRE(render_data->partCount() == 2);
        REQUIRE(render_data->totalTriangleCount() > 0);
    }

    SECTION("Parts have distinct colors") {
        doc->createBox(10.0, 10.0, 10.0, "Box1");
        doc->createBox(20.0, 20.0, 20.0, "Box2");

        auto render_data = doc->generateRenderData();

        REQUIRE(render_data->m_parts.size() == 2);

        // Colors should be different (using golden ratio distribution)
        auto& color1 = render_data->m_parts[0]->m_baseColor;
        auto& color2 = render_data->m_parts[1]->m_baseColor;

        bool colors_different =
            (color1.m_r != color2.m_r) || (color1.m_g != color2.m_g) || (color1.m_b != color2.m_b);
        REQUIRE(colors_different);
    }
}

TEST_CASE("MeshMetadata generation", "[geometry][mesh]") {
    auto doc = GeometryDocument::create();

    SECTION("Generate mesh metadata for document") {
        doc->createBox(10.0, 10.0, 10.0, "Box");

        auto mesh_data = doc->generateMeshMetadata();

        REQUIRE(mesh_data != nullptr);
        REQUIRE(mesh_data->partCount() == 1);
    }
}

TEST_CASE("Scene bounding box", "[geometry][document]") {
    auto doc = GeometryDocument::create();

    SECTION("Scene bounding box combines all parts") {
        // Create box at origin (0-10, 0-20, 0-30)
        doc->createBox(10.0, 20.0, 30.0, "Box");

        auto scene_bbox = doc->sceneBoundingBox();
        REQUIRE(scene_bbox.isValid());
    }
}
