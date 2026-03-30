/**
 * @file scene_graph_test.cpp
 * @brief Unit tests for SceneGraph
 */

#include <opengeolab/scene/scene_graph.hpp>

#include <doctest/doctest.h>

#include <algorithm>
#include <glm/glm.hpp>

namespace OpenGeoLab::Scene::Tests {

namespace {

void checkVec3(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

bool containsNodeId(const std::vector<NodeId>& ids, NodeId id) {
    return std::ranges::find(ids, id) != ids.end();
}

BoundingBox3D makeBounds(const glm::vec3& min, const glm::vec3& max) {
    BoundingBox3D bounds;
    bounds.expand(min);
    bounds.expand(max);
    return bounds;
}

} // namespace

TEST_CASE("SceneGraph root exists with reserved id") {
    SceneGraph graph;

    REQUIRE(graph.root() != nullptr);
    CHECK(graph.root()->id() == 0);
    CHECK(std::string{graph.root()->name()} == "root");
}

TEST_CASE("SceneGraph adds nodes to root") {
    SceneGraph graph;

    SceneNode* node = graph.addNode("A");

    REQUIRE(node != nullptr);
    CHECK(node->parent() == graph.root());
    CHECK(graph.findNode(node->id()) == node);
}

TEST_CASE("SceneGraph adds nodes to explicit parent") {
    SceneGraph graph;
    SceneNode* parent = graph.addNode("Parent");
    REQUIRE(parent != nullptr);

    SceneNode* child = graph.addNode("Child", parent->id());

    REQUIRE(child != nullptr);
    CHECK(child->parent() == parent);
    REQUIRE(parent->children().size() == 1);
    CHECK(parent->children().front().get() == child);
}

TEST_CASE("SceneGraph removes nodes and descendants") {
    SceneGraph graph;
    SceneNode* parent = graph.addNode("Parent");
    REQUIRE(parent != nullptr);
    SceneNode* child = graph.addNode("Child", parent->id());
    REQUIRE(child != nullptr);

    CHECK(graph.removeNode(parent->id()));
    CHECK(graph.findNode(parent->id()) == nullptr);
    CHECK(graph.findNode(child->id()) == nullptr);
}

TEST_CASE("SceneGraph refuses to remove root") {
    SceneGraph graph;

    CHECK_FALSE(graph.removeNode(0));
}

TEST_CASE("SceneGraph selects nodes and reports selection") {
    SceneGraph graph;
    SceneNode* node = graph.addNode("Selected");
    REQUIRE(node != nullptr);

    graph.selectNode(node->id());

    CHECK(node->isSelected());
    CHECK(containsNodeId(graph.selectedNodes(), node->id()));
}

TEST_CASE("SceneGraph replacing selection clears previous node") {
    SceneGraph graph;
    SceneNode* first = graph.addNode("First");
    SceneNode* second = graph.addNode("Second");
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    graph.selectNode(first->id());
    graph.selectNode(second->id(), false);

    CHECK_FALSE(first->isSelected());
    CHECK(second->isSelected());
    const std::vector<NodeId> selected = graph.selectedNodes();
    CHECK(selected.size() == 1);
    CHECK(selected.front() == second->id());
}

TEST_CASE("SceneGraph append selection keeps existing nodes") {
    SceneGraph graph;
    SceneNode* first = graph.addNode("First");
    SceneNode* second = graph.addNode("Second");
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    graph.selectNode(first->id());
    graph.selectNode(second->id(), true);

    CHECK(first->isSelected());
    CHECK(second->isSelected());
    const std::vector<NodeId> selected = graph.selectedNodes();
    CHECK(selected.size() == 2);
    CHECK(containsNodeId(selected, first->id()));
    CHECK(containsNodeId(selected, second->id()));
}

TEST_CASE("SceneGraph deselects a single node") {
    SceneGraph graph;
    SceneNode* node = graph.addNode("Node");
    REQUIRE(node != nullptr);
    graph.selectNode(node->id());

    graph.deselectNode(node->id());

    CHECK_FALSE(node->isSelected());
    CHECK(graph.selectedNodes().empty());
}

TEST_CASE("SceneGraph clears selection") {
    SceneGraph graph;
    SceneNode* first = graph.addNode("First");
    SceneNode* second = graph.addNode("Second");
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    graph.selectNode(first->id());
    graph.selectNode(second->id(), true);

    graph.clearSelection();

    CHECK_FALSE(first->isSelected());
    CHECK_FALSE(second->isSelected());
    CHECK(graph.selectedNodes().empty());
}

TEST_CASE("SceneGraph tracks hovered node") {
    SceneGraph graph;
    SceneNode* node = graph.addNode("Hovered");
    REQUIRE(node != nullptr);

    graph.setHoveredNode(node->id());
    CHECK(graph.hoveredNode() == std::optional<NodeId>{node->id()});

    graph.setHoveredNode(std::nullopt);
    CHECK(graph.hoveredNode() == std::nullopt);
}

TEST_CASE("SceneGraph visible traversal skips invisible subtrees") {
    SceneGraph graph;
    SceneNode* visible = graph.addNode("Visible");
    SceneNode* hidden = graph.addNode("Hidden");
    REQUIRE(visible != nullptr);
    REQUIRE(hidden != nullptr);
    SceneNode* hidden_child = graph.addNode("HiddenChild", hidden->id());
    REQUIRE(hidden_child != nullptr);
    hidden->setVisible(false);

    std::vector<NodeId> visited;
    graph.traverseVisible([&](const SceneNode& node) { visited.push_back(node.id()); });

    CHECK(containsNodeId(visited, graph.root()->id()));
    CHECK(containsNodeId(visited, visible->id()));
    CHECK_FALSE(containsNodeId(visited, hidden->id()));
    CHECK_FALSE(containsNodeId(visited, hidden_child->id()));
}

TEST_CASE("SceneGraph dirty traversal visits only nodes newer than threshold") {
    SceneGraph graph;
    SceneNode* clean = graph.addNode("Clean");
    SceneNode* dirty = graph.addNode("Dirty");
    REQUIRE(clean != nullptr);
    REQUIRE(dirty != nullptr);
    dirty->markDirty();
    dirty->markDirty();

    std::vector<NodeId> visited;
    graph.traverseDirty(1, [&](const SceneNode& node) { visited.push_back(node.id()); });

    CHECK_FALSE(containsNodeId(visited, clean->id()));
    CHECK(containsNodeId(visited, dirty->id()));
}

TEST_CASE("SceneGraph scene bounds aggregate visible nodes") {
    SceneGraph graph;
    SceneNode* first = graph.addNode("First");
    SceneNode* second = graph.addNode("Second");
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    first->setLocalBounds(makeBounds(glm::vec3{0.0F, 0.0F, 0.0F}, glm::vec3{1.0F, 1.0F, 1.0F}));
    second->setLocalBounds(makeBounds(glm::vec3{-2.0F, 3.0F, 1.0F}, glm::vec3{4.0F, 5.0F, 6.0F}));

    const BoundingBox3D bounds = graph.sceneBounds();

    CHECK(bounds.isValid());
    checkVec3(bounds.min, glm::vec3{-2.0F, 0.0F, 0.0F});
    checkVec3(bounds.max, glm::vec3{4.0F, 5.0F, 6.0F});
}

TEST_CASE("SceneGraph emits nodeAdded signal when inserting node") {
    SceneGraph graph;
    NodeId added_id = 0;
    auto conn = graph.nodeAdded.connect([&](NodeId id) { added_id = id; });

    SceneNode* node = graph.addNode("Signal");

    REQUIRE(node != nullptr);
    CHECK(added_id == node->id());
}

TEST_CASE("SceneGraph emits nodeRemoved signal when removing node") {
    SceneGraph graph;
    SceneNode* node = graph.addNode("Signal");
    REQUIRE(node != nullptr);
    const NodeId node_id = node->id();
    NodeId removed_id = 0;
    auto conn = graph.nodeRemoved.connect([&](NodeId id) { removed_id = id; });

    CHECK(graph.removeNode(node_id));
    CHECK(removed_id == node_id);
}

TEST_CASE("SceneGraph emits selectionChanged signal for select and clear") {
    SceneGraph graph;
    SceneNode* node = graph.addNode("Selectable");
    REQUIRE(node != nullptr);
    int signal_count = 0;
    auto conn = graph.selectionChanged.connect([&]() { ++signal_count; });

    graph.selectNode(node->id());
    graph.clearSelection();

    CHECK(signal_count == 2);
}

} // namespace OpenGeoLab::Scene::Tests
