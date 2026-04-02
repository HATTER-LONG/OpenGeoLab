/**
 * @file describe_labels_action_test.cpp
 * @brief Tests for the describe_labels scene action
 */

#include <doctest/doctest.h>

#include <opengeolab/scene/describe_labels_action.hpp>
#include <opengeolab/scene/label_manager.hpp>

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/label_colors.hpp>

using OpenGeoLab::Core::EntityRef;
using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Scene::DescribeLabelsAction;
using OpenGeoLab::Scene::Label3D;
using OpenGeoLab::Scene::LabelManager;

TEST_CASE("describe_labels: empty label manager returns legend and empty list") {
    LabelManager mgr;
    DescribeLabelsAction action(mgr);

    auto result = action.execute({}, nullptr);

    CHECK(result["ok"] == true);
    CHECK(result["action"] == "describe_labels");
    CHECK(result["totalLabels"] == 0);
    CHECK(result["labels"].is_array());
    CHECK(result["labels"].empty());

    // Color legend is always present
    CHECK(result["colorLegend"].is_object());
    CHECK(result["colorLegend"].contains("GeoVertex"));
    CHECK(result["colorLegend"].contains("GeoEdge"));
    CHECK(result["colorLegend"].contains("GeoFace"));
    CHECK(result["colorLegend"].contains("GeoSolid"));
}

TEST_CASE("describe_labels: returns active labels with entity details") {
    LabelManager mgr;
    Label3D label;
    label.entity = {1, EntityType::GeoFace, 3};
    label.text = "F:3";
    mgr.addLabel(label);

    DescribeLabelsAction action(mgr);
    auto result = action.execute({}, nullptr);

    CHECK(result["totalLabels"] == 1);
    REQUIRE(result["labels"].size() == 1);
    CHECK(result["labels"][0]["text"] == "F:3");
    CHECK(result["labels"][0]["shapeId"] == 1);
    CHECK(result["labels"][0]["entityType"] == "GeoFace");
    CHECK(result["labels"][0]["localId"] == 3);
    CHECK(result["labels"][0]["color"] == "#5CB85C");
}

TEST_CASE("describe_labels: color legend matches label_colors.hpp") {
    LabelManager mgr;
    DescribeLabelsAction action(mgr);
    auto result = action.execute({}, nullptr);

    auto vertex_entry = result["colorLegend"]["GeoVertex"];
    CHECK(vertex_entry["prefix"] == "V");
    CHECK(vertex_entry["color"] == "#E85454");

    auto edge_entry = result["colorLegend"]["GeoEdge"];
    CHECK(edge_entry["prefix"] == "E");
    CHECK(edge_entry["color"] == "#4A90D9");

    auto face_entry = result["colorLegend"]["GeoFace"];
    CHECK(face_entry["prefix"] == "F");
    CHECK(face_entry["color"] == "#5CB85C");

    auto solid_entry = result["colorLegend"]["GeoSolid"];
    CHECK(solid_entry["prefix"] == "S");
    CHECK(solid_entry["color"] == "#E8A654");
}

TEST_CASE("describe_labels: describe() returns valid schema") {
    LabelManager mgr;
    DescribeLabelsAction action(mgr);
    auto desc = action.describe();

    CHECK(desc["name"] == "describe_labels");
    CHECK(desc.contains("description"));
    CHECK(desc.contains("params"));
    CHECK(desc.contains("returns"));
}
