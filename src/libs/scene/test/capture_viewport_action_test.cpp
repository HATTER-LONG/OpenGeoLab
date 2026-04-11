/**
 * @file capture_viewport_action_test.cpp
 * @brief Tests for CaptureViewportAction metadata collection
 */

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/scene/bounding_box3d.hpp>
#include <opengeolab/scene/camera_state.hpp>
#include <opengeolab/scene/capture_viewport_action.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/selection_state.hpp>

#include <BRepPrimAPI_MakeBox.hxx>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

using OpenGeoLab::Scene::CaptureViewportAction;
using OpenGeoLab::Scene::SceneGraph;

namespace {

/// Helper: params with a dummy filePath (no render thread available).
/// Capture will time out, but metadata is collected synchronously.
nlohmann::json metaParams(nlohmann::json extra = {}) {
    nlohmann::json params = {{"filePath", "C:/nonexistent/test_capture.png"}};
    if(extra.is_object()) {
        params.merge_patch(extra);
    }
    return params;
}

} // namespace

TEST_SUITE("CaptureViewportAction") {

    TEST_CASE("describe returns expected schema") {
        SceneGraph graph;
        CaptureViewportAction action(graph);
        auto desc = action.describe();

        CHECK(desc["name"] == "capture_viewport");
        CHECK(desc.contains("description"));
        CHECK(desc.contains("params"));
        CHECK(desc["params"].contains("filePath"));
        CHECK(desc["params"].contains("width"));
        CHECK(desc["params"].contains("height"));
        CHECK(desc["params"].contains("includeMetadata"));
        CHECK(desc["params"].contains("includeTopology"));
        CHECK_FALSE(desc["params"].contains("captureImage"));
        CHECK_FALSE(desc["params"].contains("outputPath"));
        CHECK(desc.contains("returns"));
        CHECK(desc["returns"].contains("ok"));
        CHECK(desc["returns"].contains("savedPath"));
        CHECK_FALSE(desc["returns"].contains("image"));
    }

    TEST_CASE("execute requires filePath") {
        SceneGraph graph;
        CaptureViewportAction action(graph);

        auto result = action.execute(nlohmann::json::object(), nullptr);
        CHECK(result["ok"] == false);
        CHECK(result["error"].get<std::string>().find("filePath") != std::string::npos);
    }

    TEST_CASE("execute with empty filePath returns error") {
        SceneGraph graph;
        CaptureViewportAction action(graph);

        auto result = action.execute({{"filePath", ""}}, nullptr);
        CHECK(result["ok"] == false);
    }

    TEST_CASE("execute returns valid metadata") {
        SceneGraph graph;
        CaptureViewportAction action(graph);

        auto result = action.execute(metaParams(), nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "capture_viewport");
        REQUIRE(result.contains("metadata"));

        auto& meta = result["metadata"];
        CHECK(meta.contains("viewport"));
        CHECK(meta["viewport"]["width"] == 1024);
        CHECK(meta["viewport"]["height"] == 768);

        CHECK(meta.contains("camera"));
        CHECK(meta["camera"].contains("eye"));
        CHECK(meta["camera"].contains("target"));
        CHECK(meta["camera"].contains("up"));

        CHECK(meta.contains("visibleShapes"));
        CHECK(meta["visibleShapes"].is_array());
        CHECK(meta["visibleShapes"].empty());

        CHECK(meta.contains("selections"));
        CHECK(meta["selections"].is_array());

        CHECK(meta.contains("labels"));
        CHECK(meta["labels"].is_array());
    }

    TEST_CASE("execute with custom resolution") {
        SceneGraph graph;
        CaptureViewportAction action(graph);

        auto result = action.execute(metaParams({{"width", 512}, {"height", 384}}), nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["metadata"]["viewport"]["width"] == 512);
        CHECK(result["metadata"]["viewport"]["height"] == 384);
    }

    TEST_CASE("execute with includeMetadata=false omits metadata") {
        SceneGraph graph;
        CaptureViewportAction action(graph);

        auto result = action.execute(metaParams({{"includeMetadata", false}}), nullptr);

        CHECK(result["ok"] == true);
        CHECK_FALSE(result.contains("metadata"));
    }

    TEST_CASE("camera state is captured from ViewportState") {
        SceneGraph graph;

        OpenGeoLab::Scene::CameraState cam;
        cam.position = {10.0F, 20.0F, 30.0F};
        cam.target = {1.0F, 2.0F, 3.0F};
        cam.up = {0.0F, 1.0F, 0.0F};
        graph.viewportState().setCamera(cam);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        auto& camera_json = result["metadata"]["camera"];
        auto eye = camera_json["eye"];
        CHECK(eye[0].get<float>() == doctest::Approx(10.0F));
        CHECK(eye[1].get<float>() == doctest::Approx(20.0F));
        CHECK(eye[2].get<float>() == doctest::Approx(30.0F));

        auto target = camera_json["target"];
        CHECK(target[0].get<float>() == doctest::Approx(1.0F));
        CHECK(target[1].get<float>() == doctest::Approx(2.0F));
        CHECK(target[2].get<float>() == doctest::Approx(3.0F));
    }

    TEST_CASE("selections are captured from SelectionState") {
        SceneGraph graph;

        OpenGeoLab::Core::EntityRef entity{1, OpenGeoLab::Core::EntityType::GeoFace, 3};
        graph.selectionState().addSelection(entity);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        auto& sels = result["metadata"]["selections"];
        REQUIRE(sels.size() == 1);
        CHECK(sels[0]["shapeId"] == 1);
        CHECK(sels[0]["type"] == "GeoFace");
        CHECK(sels[0]["localId"] == 3);
    }

    TEST_CASE("labels are captured from LabelManager") {
        SceneGraph graph;

        OpenGeoLab::Core::EntityRef entity{2, OpenGeoLab::Core::EntityType::GeoEdge, 5};
        graph.labelManager().addLabel({entity, "E:5"});

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        auto& labels = result["metadata"]["labels"];
        REQUIRE(labels.size() == 1);
        CHECK(labels[0]["text"] == "E:5");
        CHECK(labels[0]["entity"]["shapeId"] == 2);
    }

    TEST_CASE("visible nodes appear in visibleShapes") {
        SceneGraph graph;

        auto node_id = graph.addNode("Box_1");
        auto* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        node->setSource("geometry", 1);
        node->setVisible(true);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        auto& shapes = result["metadata"]["visibleShapes"];
        REQUIRE(shapes.size() == 1);
        CHECK(shapes[0]["shapeId"] == 1);
        CHECK(shapes[0]["name"] == "Box_1");
    }

    TEST_CASE("invisible nodes are excluded from visibleShapes") {
        SceneGraph graph;

        auto node_id = graph.addNode("Hidden_1");
        auto* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        node->setSource("geometry", 1);
        node->setVisible(false);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        CHECK(result["metadata"]["visibleShapes"].empty());
    }

    TEST_CASE("child of hidden parent is excluded from visibleShapes") {
        SceneGraph graph;

        auto parent_id = graph.addNode("Parent");
        auto* parent = graph.findNode(parent_id);
        REQUIRE(parent != nullptr);
        parent->setSource("geometry", 1);
        parent->setVisible(false);

        auto child_id = graph.addNode("Child", parent_id);
        auto* child = graph.findNode(child_id);
        REQUIRE(child != nullptr);
        child->setSource("geometry", 2);
        child->setVisible(true);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        CHECK(result["metadata"]["visibleShapes"].empty());
    }

    TEST_CASE("hover entity is captured from SelectionState") {
        SceneGraph graph;

        OpenGeoLab::Core::EntityRef entity{3, OpenGeoLab::Core::EntityType::GeoVertex, 7};
        graph.selectionState().setHovered(entity);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        auto& hover = result["metadata"]["hover"];
        CHECK_FALSE(hover.is_null());
        CHECK(hover["shapeId"] == 3);
    }

    TEST_CASE("hover is null when nothing is hovered") {
        SceneGraph graph;

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        CHECK(result["metadata"]["hover"].is_null());
    }

    TEST_CASE("screenBBox is computed for nodes with valid bounds") {
        SceneGraph graph;

        auto node_id = graph.addNode("Box_1");
        auto* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        node->setSource("geometry", 1);
        node->setVisible(true);

        OpenGeoLab::Scene::BoundingBox3D bounds;
        bounds.min = {-1.0F, -1.0F, -1.0F};
        bounds.max = {1.0F, 1.0F, 1.0F};
        node->setLocalBounds(bounds);

        OpenGeoLab::Scene::CameraState cam;
        cam.position = {0.0F, 0.0F, 5.0F};
        cam.target = {0.0F, 0.0F, 0.0F};
        cam.up = {0.0F, 1.0F, 0.0F};
        graph.viewportState().setCamera(cam);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams({{"width", 800}, {"height", 600}}), nullptr);

        auto& shapes = result["metadata"]["visibleShapes"];
        REQUIRE(shapes.size() == 1);
        REQUIRE(shapes[0].contains("screenBBox"));

        auto& bbox = shapes[0]["screenBBox"];
        CHECK(bbox.contains("x"));
        CHECK(bbox.contains("y"));
        CHECK(bbox.contains("w"));
        CHECK(bbox.contains("h"));
        CHECK(bbox["w"].get<int>() > 0);
        CHECK(bbox["h"].get<int>() > 0);
    }

    TEST_CASE("invalid param types use defaults instead of crashing") {
        SceneGraph graph;
        CaptureViewportAction action(graph);

        auto result =
            action.execute({{"filePath", "C:/temp/test.png"}, {"width", "not_a_number"}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["metadata"]["viewport"]["width"] == 1024);
    }

    TEST_CASE("includeTopology adds topology to visibleShapes") {
        SceneGraph graph;
        OpenGeoLab::Geometry::ShapeStore store;
        auto shape_id = store.add("Box", BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape());
        graph.setShapeStore(&store);

        auto node_id = graph.addNode("Box");
        auto* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        node->setSource("geometry", shape_id);
        node->setVisible(true);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams({{"includeTopology", true}}), nullptr);

        CHECK(result["ok"] == true);
        auto& shapes = result["metadata"]["visibleShapes"];
        REQUIRE(shapes.size() == 1);
        REQUIRE(shapes[0].contains("topology"));

        auto& topo = shapes[0]["topology"];
        CHECK(topo["counts"]["faces"] == 6);
        CHECK(topo["counts"]["edges"] == 12);
        CHECK(topo["counts"]["vertices"] == 8);
        CHECK(topo["faces"].is_array());
        CHECK(topo["faces"].size() == 6);
        CHECK(topo["edges"].is_array());
        CHECK(topo["edges"].size() == 12);
    }

    TEST_CASE("includeTopology=false does not add topology") {
        SceneGraph graph;
        OpenGeoLab::Geometry::ShapeStore store;
        auto shape_id = store.add("Box", BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape());
        graph.setShapeStore(&store);

        auto node_id = graph.addNode("Box");
        auto* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        node->setSource("geometry", shape_id);
        node->setVisible(true);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        CHECK(result["ok"] == true);
        auto& shapes = result["metadata"]["visibleShapes"];
        REQUIRE(shapes.size() == 1);
        CHECK_FALSE(shapes[0].contains("topology"));
    }

    TEST_CASE("includeTopology without ShapeStore gracefully omits topology") {
        SceneGraph graph; // no shapeStore set

        auto node_id = graph.addNode("Box");
        auto* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        node->setSource("geometry", 1);
        node->setVisible(true);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams({{"includeTopology", true}}), nullptr);

        CHECK(result["ok"] == true);
        auto& shapes = result["metadata"]["visibleShapes"];
        REQUIRE(shapes.size() == 1);
        CHECK_FALSE(shapes[0].contains("topology"));
    }

    TEST_CASE("worldBounds included for nodes with valid bounds") {
        SceneGraph graph;

        auto node_id = graph.addNode("Box_1");
        auto* node = graph.findNode(node_id);
        REQUIRE(node != nullptr);
        node->setSource("geometry", 1);
        node->setVisible(true);

        OpenGeoLab::Scene::BoundingBox3D bounds;
        bounds.min = {-1.0F, -1.0F, -1.0F};
        bounds.max = {1.0F, 1.0F, 1.0F};
        node->setLocalBounds(bounds);

        CaptureViewportAction action(graph);
        auto result = action.execute(metaParams(), nullptr);

        auto& shapes = result["metadata"]["visibleShapes"];
        REQUIRE(shapes.size() == 1);
        REQUIRE(shapes[0].contains("worldBounds"));
        CHECK(shapes[0]["worldBounds"]["min"][0].get<float>() == doctest::Approx(-1.0F));
        CHECK(shapes[0]["worldBounds"]["max"][0].get<float>() == doctest::Approx(1.0F));
    }

    TEST_CASE("capture times out without render thread but metadata is still valid") {
        SceneGraph graph;
        CaptureViewportAction action(graph);

        auto result = action.execute(metaParams(), nullptr);

        CHECK(result["ok"] == true);
        CHECK(result.contains("metadata"));
        // No render thread → capture times out
        CHECK(result.contains("savedPathError"));
        CHECK_FALSE(result.contains("savedPath"));
    }

} // TEST_SUITE
