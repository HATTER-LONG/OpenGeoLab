/**
 * @file selection_state_test.cpp
 * @brief Unit tests for SelectionState
 */

#include <opengeolab/scene/selection_state.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Core::EntityRef;
using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Core::PickMask;
using OpenGeoLab::Scene::SelectionState;

namespace {
constexpr EntityRef FACE_1{1, EntityType::GeoFace, 3};
constexpr EntityRef FACE_2{1, EntityType::GeoFace, 5};
constexpr EntityRef EDGE_1{1, EntityType::GeoEdge, 2};
constexpr EntityRef VERTEX_1{1, EntityType::GeoVertex, 1};
} // namespace

TEST_SUITE("SelectionState") {
    TEST_CASE("initially empty") {
        SelectionState state;
        CHECK(state.selections().empty());
        CHECK_FALSE(state.hovered().has_value());
        CHECK(state.selectionVersion() == 0);
        CHECK(state.hoverVersion() == 0);
    }

    TEST_CASE("add and query selection") {
        SelectionState state;
        state.addSelection(FACE_1);

        CHECK(state.isSelected(FACE_1));
        CHECK_FALSE(state.isSelected(FACE_2));
        CHECK(state.selections().size() == 1);
        CHECK(state.selectionVersion() == 1);
    }

    TEST_CASE("duplicate add is idempotent") {
        SelectionState state;
        state.addSelection(FACE_1);
        state.addSelection(FACE_1);

        CHECK(state.selections().size() == 1);
        CHECK(state.selectionVersion() == 1);
    }

    TEST_CASE("remove selection") {
        SelectionState state;
        state.addSelection(FACE_1);
        state.addSelection(FACE_2);
        state.removeSelection(FACE_1);

        CHECK_FALSE(state.isSelected(FACE_1));
        CHECK(state.isSelected(FACE_2));
        CHECK(state.selectionVersion() == 3);
    }

    TEST_CASE("remove nonexistent is no-op") {
        SelectionState state;
        state.removeSelection(FACE_1);
        CHECK(state.selectionVersion() == 0);
    }

    TEST_CASE("clear selection") {
        SelectionState state;
        state.addSelection(FACE_1);
        state.addSelection(EDGE_1);
        state.clearSelection();

        CHECK(state.selections().empty());
        CHECK(state.selectionVersion() == 3);
    }

    TEST_CASE("clear empty is no-op") {
        SelectionState state;
        state.clearSelection();
        CHECK(state.selectionVersion() == 0);
    }

    TEST_CASE("invalid entity rejected") {
        SelectionState state;
        state.addSelection(EntityRef{});
        CHECK(state.selections().empty());
        CHECK(state.selectionVersion() == 0);
    }

    TEST_CASE("hover set and clear") {
        SelectionState state;
        state.setHovered(FACE_1);
        CHECK(state.hovered().has_value());
        CHECK(*state.hovered() == FACE_1);
        CHECK(state.hoverVersion() == 1);

        state.clearHover();
        CHECK_FALSE(state.hovered().has_value());
        CHECK(state.hoverVersion() == 2);
    }

    TEST_CASE("hover same entity is idempotent") {
        SelectionState state;
        state.setHovered(FACE_1);
        state.setHovered(FACE_1);
        CHECK(state.hoverVersion() == 1);
    }

    TEST_CASE("invalid hovered entity clears hover") {
        SelectionState state;
        state.setHovered(FACE_1);

        state.setHovered(EntityRef{});

        CHECK_FALSE(state.hovered().has_value());
        CHECK(state.hoverVersion() == 2);
    }

    TEST_CASE("pick configuration") {
        SelectionState state;
        CHECK_FALSE(state.pickEnabled());
        CHECK(state.pickMask() == PickMask::None);

        state.setPickEnabled(true);
        state.setPickMask(PickMask::Vertex | PickMask::Edge | PickMask::Face);

        CHECK(state.pickEnabled());
        CHECK((state.pickMask() & PickMask::Vertex) != PickMask::None);
    }

    TEST_CASE("signal emitted on add") {
        SelectionState state;
        EntityRef captured;
        auto conn = state.entitySelected.connect([&](EntityRef ref) { captured = ref; });

        state.addSelection(FACE_1);
        CHECK(captured == FACE_1);
    }

    TEST_CASE("signal emitted on remove") {
        SelectionState state;
        state.addSelection(FACE_1);

        EntityRef captured;
        auto conn = state.entityDeselected.connect([&](EntityRef ref) { captured = ref; });

        state.removeSelection(FACE_1);
        CHECK(captured == FACE_1);
    }

    TEST_CASE("signal emitted on clear") {
        SelectionState state;
        state.addSelection(FACE_1);

        int clear_count = 0;
        auto conn = state.selectionCleared.connect([&]() { ++clear_count; });

        state.clearSelection();
        CHECK(clear_count == 1);
    }

    TEST_CASE("signal emitted on hover set") {
        SelectionState state;
        std::optional<EntityRef> captured;
        auto conn =
            state.hoverChanged.connect([&](std::optional<EntityRef> ref) { captured = ref; });

        state.setHovered(FACE_1);

        REQUIRE(captured.has_value());
        CHECK(*captured == FACE_1);
    }

    TEST_CASE("signal emitted on hover clear") {
        SelectionState state;
        state.setHovered(FACE_1);

        std::optional<EntityRef> captured = FACE_1;
        auto conn =
            state.hoverChanged.connect([&](std::optional<EntityRef> ref) { captured = ref; });

        state.clearHover();

        CHECK_FALSE(captured.has_value());
    }

    TEST_CASE("selections are sorted") {
        SelectionState state;
        state.addSelection(FACE_2);
        state.addSelection(VERTEX_1);
        state.addSelection(EDGE_1);

        auto sels = state.selections();
        REQUIRE(sels.size() == 3);
        CHECK(sels[0] < sels[1]);
        CHECK(sels[1] < sels[2]);
    }
}

TEST_SUITE("SceneGraph::selectionState") {
    TEST_CASE("accessible from SceneGraph") {
        OpenGeoLab::Scene::SceneGraph graph;
        auto& sel = graph.selectionState();

        sel.addSelection(FACE_1);
        CHECK(graph.selectionState().isSelected(FACE_1));
    }
}
