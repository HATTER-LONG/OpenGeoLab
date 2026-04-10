/**
 * @file selection_state_test.cpp
 * @brief Unit tests for SelectionState
 */

#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/selection_state.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Core::EntityRef;
using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Core::PickMask;
using OpenGeoLab::Scene::SelectionState;
namespace Core = OpenGeoLab::Core;

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
        std::vector<Core::EntityRef> captured;
        auto conn = state.entitiesSelected.connect(
            [&](std::vector<Core::EntityRef> refs) { captured = std::move(refs); });

        state.addSelection(FACE_1);
        REQUIRE(captured.size() == 1);
        CHECK(captured[0] == FACE_1);
    }

    TEST_CASE("signal emitted on remove") {
        SelectionState state;
        state.addSelection(FACE_1);

        std::vector<Core::EntityRef> captured;
        auto conn = state.entitiesDeselected.connect(
            [&](std::vector<Core::EntityRef> refs) { captured = std::move(refs); });

        state.removeSelection(FACE_1);
        REQUIRE(captured.size() == 1);
        CHECK(captured[0] == FACE_1);
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

    TEST_CASE("addSelections batch — basic") {
        SelectionState state;
        const std::vector<EntityRef> batch = {FACE_1, EDGE_1, FACE_2};
        state.addSelections(batch);

        CHECK(state.selections().size() == 3);
        CHECK(state.isSelected(FACE_1));
        CHECK(state.isSelected(EDGE_1));
        CHECK(state.isSelected(FACE_2));
        CHECK(state.selectionVersion() == 1);
    }

    TEST_CASE("addSelections batch — empty input") {
        SelectionState state;
        state.addSelections({});
        CHECK(state.selections().empty());
        CHECK(state.selectionVersion() == 0);
    }

    TEST_CASE("addSelections batch — duplicates in input") {
        SelectionState state;
        const std::vector<EntityRef> batch = {FACE_1, FACE_1, FACE_2};
        state.addSelections(batch);

        CHECK(state.selections().size() == 2);
        CHECK(state.selectionVersion() == 1);
    }

    TEST_CASE("addSelections batch — partial overlap with existing") {
        SelectionState state;
        state.addSelection(FACE_1);

        const std::vector<EntityRef> batch = {FACE_1, FACE_2, EDGE_1};
        state.addSelections(batch);

        CHECK(state.selections().size() == 3);
        CHECK(state.selectionVersion() == 2);
    }

    TEST_CASE("addSelections batch — signal emits actually added") {
        SelectionState state;
        state.addSelection(FACE_1);

        std::vector<Core::EntityRef> captured;
        auto conn = state.entitiesSelected.connect(
            [&](std::vector<Core::EntityRef> refs) { captured = std::move(refs); });

        const std::vector<EntityRef> batch = {FACE_1, FACE_2, EDGE_1};
        state.addSelections(batch);

        REQUIRE(captured.size() == 2);
        CHECK(std::find(captured.begin(), captured.end(), FACE_2) != captured.end());
        CHECK(std::find(captured.begin(), captured.end(), EDGE_1) != captured.end());
    }

    TEST_CASE("addSelections batch — invalid entities filtered") {
        SelectionState state;
        const std::vector<EntityRef> batch = {FACE_1, EntityRef{}, EDGE_1};
        state.addSelections(batch);

        CHECK(state.selections().size() == 2);
        CHECK(state.selectionVersion() == 1);
    }

    TEST_CASE("removeSelections batch — basic") {
        SelectionState state;
        state.addSelections({FACE_1, FACE_2, EDGE_1, VERTEX_1});

        const std::vector<EntityRef> to_remove = {FACE_1, EDGE_1};
        state.removeSelections(to_remove);

        CHECK(state.selections().size() == 2);
        CHECK(state.isSelected(FACE_2));
        CHECK(state.isSelected(VERTEX_1));
        CHECK_FALSE(state.isSelected(FACE_1));
        CHECK_FALSE(state.isSelected(EDGE_1));
    }

    TEST_CASE("removeSelections batch — empty input") {
        SelectionState state;
        state.addSelection(FACE_1);
        state.removeSelections({});

        CHECK(state.selections().size() == 1);
        CHECK(state.selectionVersion() == 1);
    }

    TEST_CASE("removeSelections batch — none present") {
        SelectionState state;
        state.addSelection(FACE_1);

        state.removeSelections({FACE_2, EDGE_1});
        CHECK(state.selections().size() == 1);
        CHECK(state.selectionVersion() == 1);
    }

    TEST_CASE("removeSelections batch — signal emits actually removed") {
        SelectionState state;
        state.addSelections({FACE_1, FACE_2, EDGE_1});

        std::vector<Core::EntityRef> captured;
        auto conn = state.entitiesDeselected.connect(
            [&](std::vector<Core::EntityRef> refs) { captured = std::move(refs); });

        state.removeSelections({FACE_1, VERTEX_1});

        REQUIRE(captured.size() == 1);
        CHECK(captured[0] == FACE_1);
    }

    TEST_CASE("removeSelections batch — version increments once") {
        SelectionState state;
        state.addSelections({FACE_1, FACE_2, EDGE_1});
        const uint64_t before = state.selectionVersion();

        state.removeSelections({FACE_1, FACE_2});
        CHECK(state.selectionVersion() == before + 1);
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
