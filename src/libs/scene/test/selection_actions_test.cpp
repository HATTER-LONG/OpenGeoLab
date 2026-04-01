/**
 * @file selection_actions_test.cpp
 * @brief Tests for scene selection actions and SceneModule dispatch.
 */

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/pick_mask.hpp>
#include <opengeolab/scene/clear_selection_action.hpp>
#include <opengeolab/scene/deselect_action.hpp>
#include <opengeolab/scene/query_selection_action.hpp>
#include <opengeolab/scene/scene_module.hpp>
#include <opengeolab/scene/select_action.hpp>
#include <opengeolab/scene/selection_state.hpp>
#include <opengeolab/scene/set_hover_action.hpp>
#include <opengeolab/scene/set_pick_mode_action.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Core::EntityRef;
using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Core::PickMask;
using OpenGeoLab::Scene::ClearSelectionAction;
using OpenGeoLab::Scene::DeselectAction;
using OpenGeoLab::Scene::QuerySelectionAction;
using OpenGeoLab::Scene::SceneModule;
using OpenGeoLab::Scene::SelectAction;
using OpenGeoLab::Scene::SelectionState;
using OpenGeoLab::Scene::SetHoverAction;
using OpenGeoLab::Scene::SetPickModeAction;

namespace {
constexpr EntityRef FACE_1{1, EntityType::GeoFace, 3};
constexpr EntityRef FACE_2{1, EntityType::GeoFace, 5};
constexpr EntityRef EDGE_1{1, EntityType::GeoEdge, 2};
} // namespace

TEST_SUITE("SelectionActions") {
    TEST_CASE("SelectAction selects single entity and describe returns schema") {
        SelectionState state;
        SelectAction action(state);

        const auto result = action.execute(
            {{"entities",
              {{{"shapeId", FACE_1.shapeId}, {"type", "GeoFace"}, {"localId", FACE_1.localId}}}},
             {"append", true}},
            nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "select");
        CHECK(result["selected"] == 1);
        CHECK(state.isSelected(FACE_1));

        const auto desc = action.describe();
        CHECK(desc["name"] == "select");
        CHECK(desc.contains("params"));
        CHECK(desc["params"].contains("entities"));
        CHECK(desc["params"].contains("append"));
        CHECK(desc.contains("returns"));
    }

    TEST_CASE("SelectAction clears selection when append is false") {
        SelectionState state;
        state.addSelection(FACE_1);

        SelectAction action(state);
        const auto result = action.execute(
            {{"entities",
              {{{"shapeId", EDGE_1.shapeId}, {"type", "GeoEdge"}, {"localId", EDGE_1.localId}}}},
             {"append", false}},
            nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["selected"] == 1);
        CHECK_FALSE(state.isSelected(FACE_1));
        CHECK(state.isSelected(EDGE_1));
    }

    TEST_CASE("DeselectAction removes selected entity") {
        SelectionState state;
        state.addSelection(FACE_1);
        state.addSelection(EDGE_1);

        DeselectAction action(state);
        const auto result = action.execute(
            {{"entities",
              {{{"shapeId", FACE_1.shapeId}, {"type", "GeoFace"}, {"localId", FACE_1.localId}}}}},
            nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "deselect");
        CHECK(result["removed"] == 1);
        CHECK_FALSE(state.isSelected(FACE_1));
        CHECK(state.isSelected(EDGE_1));
    }

    TEST_CASE("ClearSelectionAction clears all selections") {
        SelectionState state;
        state.addSelection(FACE_1);
        state.addSelection(EDGE_1);

        ClearSelectionAction action(state);
        const auto result = action.execute({}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "clear_selection");
        CHECK(state.selections().empty());
    }

    TEST_CASE("QuerySelectionAction returns selections as strings") {
        SelectionState state;
        state.addSelection(FACE_1);
        state.addSelection(EDGE_1);

        QuerySelectionAction action(state);
        const auto result = action.execute({}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "query_selection");
        REQUIRE(result["selections"].is_array());
        REQUIRE(result["selections"].size() == 2);
        CHECK(result["selections"][0]["shapeId"] == EDGE_1.shapeId);
        CHECK(result["selections"][0]["type"] == "GeoEdge");
        CHECK(result["selections"][0]["localId"] == EDGE_1.localId);
        CHECK(result["selections"][1]["shapeId"] == FACE_1.shapeId);
        CHECK(result["selections"][1]["type"] == "GeoFace");
        CHECK(result["selections"][1]["localId"] == FACE_1.localId);
    }

    TEST_CASE("SetPickModeAction applies mask and enabled state") {
        SelectionState state;
        SetPickModeAction action(state);

        const auto result = action.execute({{"pickMask", 9U}, {"enabled", true}}, nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["action"] == "set_pick_mode");
        CHECK(state.pickEnabled());
        CHECK(static_cast<uint32_t>(state.pickMask()) == 9U);
    }

    TEST_CASE("SetHoverAction sets and clears hovered entity") {
        SelectionState state;
        SetHoverAction action(state);

        const auto set_result = action.execute(
            {{"entity",
              {{"shapeId", FACE_2.shapeId}, {"type", "GeoFace"}, {"localId", FACE_2.localId}}}},
            nullptr);

        CHECK(set_result["ok"] == true);
        REQUIRE(state.hovered().has_value());
        CHECK(*state.hovered() == FACE_2);

        const auto clear_result = action.execute({{"entity", nullptr}}, nullptr);
        CHECK(clear_result["ok"] == true);
        CHECK_FALSE(state.hovered().has_value());
    }
}

TEST_SUITE("SceneModuleSelectionActions") {
    TEST_CASE("select dispatches through module process") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule mod(factory);

        const auto result = mod.process({{"action", "select"},
                                         {"param",
                                          {{"entities",
                                            {{{"shapeId", FACE_1.shapeId},
                                              {"type", "GeoFace"},
                                              {"localId", FACE_1.localId}}}}}}},
                                        nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["selected"] == 1);
        CHECK(mod.sceneGraph().selectionState().isSelected(FACE_1));
    }

    TEST_CASE("query_selection dispatches through module process") {
        Kangaroo::Util::PluginComponentFactory factory;
        SceneModule mod(factory);
        mod.sceneGraph().selectionState().addSelection(FACE_1);
        mod.sceneGraph().selectionState().addSelection(EDGE_1);

        const auto result = mod.process({{"action", "query_selection"}, {"param", {}}}, nullptr);

        CHECK(result["ok"] == true);
        REQUIRE(result["selections"].is_array());
        REQUIRE(result["selections"].size() == 2);
        CHECK(result["selections"][0]["type"] == "GeoEdge");
        CHECK(result["selections"][1]["type"] == "GeoFace");
    }
}
