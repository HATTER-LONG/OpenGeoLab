/**
 * @file scene_graph_test.cpp
 * @brief Tests for SceneGraph tree management and bounds aggregation.
 */
#include <opengeolab/scene/scene_graph.hpp>

#include <doctest/doctest.h>
#include <glm/vec3.hpp>

namespace OpenGeoLab::Scene::Tests {

namespace {

[[nodiscard]] auto makeBounds(const glm::vec3& min, const glm::vec3& max) -> BoundingBox {
    BoundingBox bounds;
    bounds.expand(min);
    bounds.expand(max);
    return bounds;
}

} // namespace

TEST_CASE("SceneGraph") {
    SUBCASE("root node exists with id 0") {
        SceneGraph graph;

        CHECK(graph.root().id == 0);
        CHECK(graph.root().children.empty());
    }

    SUBCASE("addNode returns unique id") {
        SceneGraph graph;

        SceneNode first;
        SceneNode second;

        const int first_id = graph.addNode(first);
        const int second_id = graph.addNode(second);

        CHECK(first_id == 1);
        CHECK(second_id == 2);
        CHECK(first_id != second_id);
    }

    SUBCASE("findById returns correct node") {
        SceneGraph graph;

        SceneNode node;
        node.name = "mesh";

        const int id = graph.addNode(node);
        SceneNode* const found = graph.findById(id);

        REQUIRE(found != nullptr);
        CHECK(found->id == id);
        CHECK(found->name == "mesh");
    }

    SUBCASE("findById returns nullptr for missing id") {
        SceneGraph graph;

        CHECK(graph.findById(42) == nullptr);
    }

    SUBCASE("removeNode removes node and children") {
        SceneGraph graph;

        SceneNode parent;
        parent.name = "parent";
        const int parent_id = graph.addNode(parent);

        SceneNode child;
        child.name = "child";
        const int child_id = graph.addNode(child, parent_id);

        CHECK(graph.removeNode(parent_id));
        CHECK(graph.findById(parent_id) == nullptr);
        CHECK(graph.findById(child_id) == nullptr);
    }

    SUBCASE("removeNode on root returns false") {
        SceneGraph graph;

        CHECK_FALSE(graph.removeNode(0));
    }

    SUBCASE("addNode with missing parent returns zero") {
        SceneGraph graph;

        SceneNode child;

        CHECK(graph.addNode(child, 999) == 0);
    }

    SUBCASE("removeNode with missing id returns false") {
        SceneGraph graph;

        CHECK_FALSE(graph.removeNode(999));
    }

    SUBCASE("addNode to non-root parent") {
        SceneGraph graph;

        SceneNode parent;
        const int parent_id = graph.addNode(parent);

        SceneNode child;
        child.name = "child";
        const int child_id = graph.addNode(child, parent_id);

        SceneNode* const parent_node = graph.findById(parent_id);
        REQUIRE(parent_node != nullptr);
        REQUIRE(parent_node->children.size() == 1);
        CHECK(parent_node->children.front().id == child_id);
        CHECK(parent_node->children.front().name == "child");
    }

    SUBCASE("worldBounds covers all nodes") {
        SceneGraph graph;

        SceneNode first;
        first.bounds = makeBounds(glm::vec3{-1.0F, -2.0F, -3.0F}, glm::vec3{0.0F, 0.5F, 1.0F});
        graph.addNode(first);

        SceneNode second;
        second.bounds = makeBounds(glm::vec3{2.0F, 1.0F, -1.0F}, glm::vec3{4.0F, 3.0F, 5.0F});
        graph.addNode(second);

        const BoundingBox world = graph.worldBounds();

        CHECK(world.isValid());
        CHECK(world.min.x == doctest::Approx(-1.0F));
        CHECK(world.min.y == doctest::Approx(-2.0F));
        CHECK(world.min.z == doctest::Approx(-3.0F));
        CHECK(world.max.x == doctest::Approx(4.0F));
        CHECK(world.max.y == doctest::Approx(3.0F));
        CHECK(world.max.z == doctest::Approx(5.0F));
    }

    SUBCASE("onChanged callback fires on addNode") {
        SceneGraph graph;
        int changed_count = 0;
        graph.onChanged = [&changed_count]() { ++changed_count; };

        SceneNode node;
        graph.addNode(node);

        CHECK(changed_count == 1);
    }

    SUBCASE("onChanged callback fires on removeNode") {
        SceneGraph graph;
        int changed_count = 0;
        graph.onChanged = [&changed_count]() { ++changed_count; };

        SceneNode node;
        const int id = graph.addNode(node);

        changed_count = 0;
        CHECK(graph.removeNode(id));
        CHECK(changed_count == 1);
    }
}

} // namespace OpenGeoLab::Scene::Tests
