/**
 * @file label_actions_test.cpp
 * @brief Tests for scene label management actions.
 */

#include <opengeolab/scene/add_label_action.hpp>
#include <opengeolab/scene/clear_labels_action.hpp>
#include <opengeolab/scene/label_manager.hpp>
#include <opengeolab/scene/remove_label_action.hpp>
#include <opengeolab/scene/scene_module.hpp>
#include <opengeolab/scene/set_auto_label_action.hpp>
#include <opengeolab/scene/set_labels_visible_action.hpp>

#include <opengeolab/core/label_colors.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

using OpenGeoLab::Core::EntityRef;
using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Core::ProgressCallback;
using OpenGeoLab::Scene::AddLabelAction;
using OpenGeoLab::Scene::ClearLabelsAction;
using OpenGeoLab::Scene::LabelManager;
using OpenGeoLab::Scene::RemoveLabelAction;
using OpenGeoLab::Scene::SceneModule;
using OpenGeoLab::Scene::SetAutoLabelAction;
using OpenGeoLab::Scene::SetLabelsVisibleAction;

namespace {
const ProgressCallback NO_PROGRESS;
constexpr EntityRef FACE_1{1, EntityType::GeoFace, 3};
constexpr EntityRef EDGE_1{2, EntityType::GeoEdge, 7};
} // namespace

TEST_SUITE("LabelActions") {
    TEST_CASE("AddLabelAction creates formatted label with colors") {
        LabelManager manager;
        AddLabelAction action(manager);

        const auto result = action.execute(
            {{"shapeId", FACE_1.shapeId}, {"entityType", "GeoFace"}, {"localId", FACE_1.localId}},
            NO_PROGRESS);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "add_label");
        CHECK(result["text"] ==
              OpenGeoLab::Core::formatLabelText(FACE_1.shapeId, FACE_1.entityType, FACE_1.localId));

        const auto labels = manager.labels();
        REQUIRE(labels.size() == 1);
        CHECK(labels[0].entity == FACE_1);
        CHECK(labels[0].text == result["text"].get<std::string>());
        CHECK(labels[0].textColor == OpenGeoLab::Core::labelColor(EntityType::GeoFace));
        CHECK(labels[0].bgColor == OpenGeoLab::Core::K_LABEL_BG_COLOR);
    }

    TEST_CASE("AddLabelAction replaces existing label for entity") {
        LabelManager manager;
        manager.addLabel({FACE_1, "stale", {}, {}});
        AddLabelAction action(manager);

        const auto result = action.execute(
            {{"shapeId", FACE_1.shapeId}, {"entityType", "GeoFace"}, {"localId", FACE_1.localId}},
            NO_PROGRESS);

        CHECK(result["ok"] == true);
        const auto labels = manager.labels();
        REQUIRE(labels.size() == 1);
        CHECK(labels[0].text == result["text"].get<std::string>());
        CHECK(labels[0].text != "stale");
    }

    TEST_CASE("AddLabelAction rejects unknown entity type") {
        LabelManager manager;
        AddLabelAction action(manager);

        const auto result = action.execute(
            {{"shapeId", 1}, {"entityType", "UnknownType"}, {"localId", 9}}, NO_PROGRESS);

        CHECK(result["ok"] == false);
        CHECK(result["action"] == "add_label");
        CHECK(manager.labels().empty());
    }

    TEST_CASE("AddLabelAction describe returns schema") {
        LabelManager manager;
        AddLabelAction action(manager);

        const auto desc = action.describe();

        CHECK(desc["name"] == "add_label");
        CHECK(desc.contains("params"));
        CHECK(desc["params"].contains("shapeId"));
        CHECK(desc["params"].contains("entityType"));
        CHECK(desc["params"].contains("localId"));
        CHECK(desc.contains("returns"));
        CHECK(desc["returns"].contains("text"));
    }

    TEST_CASE("RemoveLabelAction removes existing label") {
        LabelManager manager;
        manager.addLabel({FACE_1, OpenGeoLab::Core::formatLabelText(
                                      FACE_1.shapeId, FACE_1.entityType, FACE_1.localId)});
        RemoveLabelAction action(manager);

        const auto result = action.execute(
            {{"shapeId", FACE_1.shapeId}, {"entityType", "GeoFace"}, {"localId", FACE_1.localId}},
            NO_PROGRESS);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "remove_label");
        CHECK(result["removed"] == true);
        CHECK(manager.labels().empty());
    }

    TEST_CASE("RemoveLabelAction reports false for missing label") {
        LabelManager manager;
        RemoveLabelAction action(manager);

        const auto result = action.execute(
            {{"shapeId", EDGE_1.shapeId}, {"entityType", "GeoEdge"}, {"localId", EDGE_1.localId}},
            NO_PROGRESS);

        CHECK(result["ok"] == true);
        CHECK(result["removed"] == false);
        CHECK(manager.labels().empty());
    }

    TEST_CASE("RemoveLabelAction rejects unknown entity type") {
        LabelManager manager;
        RemoveLabelAction action(manager);

        const auto result = action.execute(
            {{"shapeId", 1}, {"entityType", "Unknown"}, {"localId", 1}}, NO_PROGRESS);

        CHECK(result["ok"] == false);
        CHECK(result["removed"] == false);
        CHECK(manager.labels().empty());
    }

    TEST_CASE("ClearLabelsAction clears labels and reports count") {
        LabelManager manager;
        manager.addLabel({FACE_1, "A"});
        manager.addLabel({EDGE_1, "B"});
        ClearLabelsAction action(manager);

        const auto result = action.execute({}, NO_PROGRESS);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "clear_labels");
        CHECK(result["cleared"] == 2);
        CHECK(manager.labels().empty());
    }

    TEST_CASE("SetLabelsVisibleAction toggles visibility and validates param") {
        LabelManager manager;
        SetLabelsVisibleAction action(manager);

        const auto invalid = action.execute({}, NO_PROGRESS);
        CHECK(invalid["ok"] == false);

        const auto enabled = action.execute({{"visible", true}}, NO_PROGRESS);
        CHECK(enabled["ok"] == true);
        CHECK(enabled["action"] == "set_labels_visible");
        CHECK(manager.isVisible());
    }

    TEST_CASE("SetAutoLabelAction toggles auto label and validates param") {
        LabelManager manager;
        SetAutoLabelAction action(manager);

        const auto invalid = action.execute({{"enabled", "yes"}}, NO_PROGRESS);
        CHECK(invalid["ok"] == false);

        const auto enabled = action.execute({{"enabled", true}}, NO_PROGRESS);
        CHECK(enabled["ok"] == true);
        CHECK(enabled["action"] == "set_auto_label");
        CHECK(manager.autoLabel());

        const auto disabled = action.execute({{"enabled", false}}, NO_PROGRESS);
        CHECK(disabled["ok"] == true);
        CHECK_FALSE(manager.autoLabel());
    }
}

TEST_SUITE("SceneModuleLabelActions") {
    TEST_CASE("label actions dispatch through SceneModule process") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule module(factory);

        const auto add_result = module.process({{"action", "add_label"},
                                                {"param",
                                                 {{"shapeId", FACE_1.shapeId},
                                                  {"entityType", "GeoFace"},
                                                  {"localId", FACE_1.localId}}}},
                                               NO_PROGRESS);
        CHECK(add_result["ok"] == true);
        CHECK(module.sceneGraph().labelManager().labels().size() == 1);

        const auto visible_result = module.process(
            {{"action", "set_labels_visible"}, {"param", {{"visible", true}}}}, NO_PROGRESS);
        CHECK(visible_result["ok"] == true);
        CHECK(module.sceneGraph().labelManager().isVisible());

        const auto auto_result = module.process(
            {{"action", "set_auto_label"}, {"param", {{"enabled", true}}}}, NO_PROGRESS);
        CHECK(auto_result["ok"] == true);
        CHECK(module.sceneGraph().labelManager().autoLabel());

        const auto clear_result = module.process(
            {{"action", "clear_labels"}, {"param", nlohmann::json::object()}}, NO_PROGRESS);
        CHECK(clear_result["ok"] == true);
        CHECK(clear_result["cleared"] == 1);
        CHECK(module.sceneGraph().labelManager().labels().empty());
    }
}
