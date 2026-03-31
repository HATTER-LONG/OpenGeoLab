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

    const NodeId node_id = graph.addNode("A");

    REQUIRE(node_id != 0);
    SceneNode* node = graph.findNode(node_id);
    REQUIRE(node != nullptr);
    CHECK(node->parent() == graph.root());
    CHECK(graph.findNode(node->id()) == node);
}

TEST_CASE("SceneGraph adds nodes to explicit parent") {
    SceneGraph graph;
    const NodeId parent_id = graph.addNode("Parent");
    REQUIRE(parent_id != 0);
    SceneNode* parent = graph.findNode(parent_id);
    REQUIRE(parent != nullptr);

    const NodeId child_id = graph.addNode("Child", parent_id);

    REQUIRE(child_id != 0);
    SceneNode* child = graph.findNode(child_id);
    REQUIRE(child != nullptr);
    CHECK(child->parent() == parent);
    REQUIRE(parent->children().size() == 1);
    CHECK(parent->children().front().get() == child);
}

TEST_CASE("SceneGraph finds node by source metadata") {
    SceneGraph graph;
    const NodeId first_id = graph.addNode("First");
    REQUIRE(first_id != 0);
    const NodeId second_id = graph.addNode("Second");
    REQUIRE(second_id != 0);
    SceneNode* second = graph.findNode(second_id);
    REQUIRE(second != nullptr);
    graph.setNodeSource(second->id(), "geometry", 42U);

    CHECK(graph.findNodeBySource("geometry", 42U) == second);
    CHECK(graph.findNodeBySource("geometry", 99U) == nullptr);
    CHECK(graph.findNodeBySource("mesh", 42U) == nullptr);
}

TEST_CASE("SceneGraph removes nodes and descendants") {
    SceneGraph graph;
    const NodeId parent_id = graph.addNode("Parent");
    REQUIRE(parent_id != 0);
    const NodeId child_id = graph.addNode("Child", parent_id);
    REQUIRE(child_id != 0);

    CHECK(graph.removeNode(parent_id));
    CHECK(graph.findNode(parent_id) == nullptr);
    CHECK(graph.findNode(child_id) == nullptr);
}

TEST_CASE("SceneGraph refuses to remove root") {
    SceneGraph graph;

    CHECK_FALSE(graph.removeNode(0));
}

TEST_CASE("SceneGraph selects nodes and reports selection") {
    SceneGraph graph;
    const NodeId node_id = graph.addNode("Selected");
    REQUIRE(node_id != 0);
    SceneNode* node = graph.findNode(node_id);
    REQUIRE(node != nullptr);

    graph.selectNode(node_id);

    CHECK(node->isSelected());
    CHECK(containsNodeId(graph.selectedNodes(), node_id));
}

TEST_CASE("SceneGraph replacing selection clears previous node") {
    SceneGraph graph;
    const NodeId first_id = graph.addNode("First");
    const NodeId second_id = graph.addNode("Second");
    REQUIRE(first_id != 0);
    REQUIRE(second_id != 0);
    SceneNode* first = graph.findNode(first_id);
    SceneNode* second = graph.findNode(second_id);
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    graph.selectNode(first_id);
    graph.selectNode(second_id, false);

    CHECK_FALSE(first->isSelected());
    CHECK(second->isSelected());
    const std::vector<NodeId> selected = graph.selectedNodes();
    CHECK(selected.size() == 1);
    CHECK(selected.front() == second_id);
}

TEST_CASE("SceneGraph append selection keeps existing nodes") {
    SceneGraph graph;
    const NodeId first_id = graph.addNode("First");
    const NodeId second_id = graph.addNode("Second");
    REQUIRE(first_id != 0);
    REQUIRE(second_id != 0);
    SceneNode* first = graph.findNode(first_id);
    SceneNode* second = graph.findNode(second_id);
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    graph.selectNode(first_id);
    graph.selectNode(second_id, true);

    CHECK(first->isSelected());
    CHECK(second->isSelected());
    const std::vector<NodeId> selected = graph.selectedNodes();
    CHECK(selected.size() == 2);
    CHECK(containsNodeId(selected, first_id));
    CHECK(containsNodeId(selected, second_id));
}

TEST_CASE("SceneGraph deselects a single node") {
    SceneGraph graph;
    const NodeId node_id = graph.addNode("Node");
    REQUIRE(node_id != 0);
    SceneNode* node = graph.findNode(node_id);
    REQUIRE(node != nullptr);
    graph.selectNode(node_id);

    graph.deselectNode(node_id);

    CHECK_FALSE(node->isSelected());
    CHECK(graph.selectedNodes().empty());
}

TEST_CASE("SceneGraph clears selection") {
    SceneGraph graph;
    const NodeId first_id = graph.addNode("First");
    const NodeId second_id = graph.addNode("Second");
    REQUIRE(first_id != 0);
    REQUIRE(second_id != 0);
    SceneNode* first = graph.findNode(first_id);
    SceneNode* second = graph.findNode(second_id);
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    graph.selectNode(first_id);
    graph.selectNode(second_id, true);

    graph.clearSelection();

    CHECK_FALSE(first->isSelected());
    CHECK_FALSE(second->isSelected());
    CHECK(graph.selectedNodes().empty());
}

TEST_CASE("SceneGraph tracks hovered node") {
    SceneGraph graph;
    const NodeId node_id = graph.addNode("Hovered");
    REQUIRE(node_id != 0);

    graph.setHoveredNode(node_id);
    CHECK(graph.hoveredNode() == std::optional<NodeId>{node_id});

    graph.setHoveredNode(std::nullopt);
    CHECK(graph.hoveredNode() == std::nullopt);
}

TEST_CASE("SceneGraph visible traversal skips invisible subtrees") {
    SceneGraph graph;
    const NodeId visible_id = graph.addNode("Visible");
    const NodeId hidden_id = graph.addNode("Hidden");
    REQUIRE(visible_id != 0);
    REQUIRE(hidden_id != 0);
    SceneNode* hidden = graph.findNode(hidden_id);
    REQUIRE(hidden != nullptr);
    const NodeId hidden_child_id = graph.addNode("HiddenChild", hidden_id);
    REQUIRE(hidden_child_id != 0);
    hidden->setVisible(false);

    std::vector<NodeId> visited;
    graph.traverseVisible([&](const SceneNode& node) { visited.push_back(node.id()); });

    CHECK(containsNodeId(visited, graph.root()->id()));
    CHECK(containsNodeId(visited, visible_id));
    CHECK_FALSE(containsNodeId(visited, hidden_id));
    CHECK_FALSE(containsNodeId(visited, hidden_child_id));
}

TEST_CASE("SceneGraph dirty traversal visits only nodes newer than threshold") {
    SceneGraph graph;
    const NodeId clean_id = graph.addNode("Clean");
    const NodeId dirty_id = graph.addNode("Dirty");
    REQUIRE(clean_id != 0);
    REQUIRE(dirty_id != 0);
    SceneNode* dirty = graph.findNode(dirty_id);
    REQUIRE(dirty != nullptr);
    dirty->markDirty();
    dirty->markDirty();

    std::vector<NodeId> visited;
    graph.traverseDirty(1, [&](const SceneNode& node) { visited.push_back(node.id()); });

    CHECK_FALSE(containsNodeId(visited, clean_id));
    CHECK(containsNodeId(visited, dirty_id));
}

TEST_CASE("SceneGraph scene bounds aggregate visible nodes") {
    SceneGraph graph;
    const NodeId first_id = graph.addNode("First");
    const NodeId second_id = graph.addNode("Second");
    REQUIRE(first_id != 0);
    REQUIRE(second_id != 0);
    SceneNode* first = graph.findNode(first_id);
    SceneNode* second = graph.findNode(second_id);
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

    const NodeId node_id = graph.addNode("Signal");

    REQUIRE(node_id != 0);
    CHECK(added_id == node_id);
}

TEST_CASE("SceneGraph emits nodeRemoved signal when removing node") {
    SceneGraph graph;
    const NodeId node_id = graph.addNode("Signal");
    REQUIRE(node_id != 0);
    NodeId removed_id = 0;
    auto conn = graph.nodeRemoved.connect([&](NodeId id) { removed_id = id; });

    CHECK(graph.removeNode(node_id));
    CHECK(removed_id == node_id);
}

TEST_CASE("SceneGraph emits selectionChanged signal for select and clear") {
    SceneGraph graph;
    const NodeId node_id = graph.addNode("Selectable");
    REQUIRE(node_id != 0);
    int signal_count = 0;
    auto conn = graph.selectionChanged.connect([&]() { ++signal_count; });

    graph.selectNode(node_id);
    graph.clearSelection();

    CHECK(signal_count == 2);
}

} // namespace OpenGeoLab::Scene::Tests
