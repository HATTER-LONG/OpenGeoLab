/**
 * @file scene_graph_test.cpp
 * @brief Unit tests for Transform, BoundingBox, and SceneGraph.
 */

#include <opengeolab/scene/bounding_box.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/transform.hpp>

#include <doctest/doctest.h>

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

#include <cmath>

namespace {

constexpr float kEps = 1e-5f;

bool vec3Near(const glm::vec3& a, const glm::vec3& b, float eps = kEps) {
    return glm::all(glm::epsilonEqual(a, b, eps));
}

bool mat4Near(const glm::mat4& a, const glm::mat4& b, float eps = kEps) {
    for(int c = 0; c < 4; ++c) {
        for(int r = 0; r < 4; ++r) {
            if(std::abs(a[c][r] - b[c][r]) > eps) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

// ── Transform ───────────────────────────────────────────────────────────────

TEST_CASE("Transform: default is identity") {
    OpenGeoLab::Scene::Transform t;
    CHECK(mat4Near(t.matrix(), glm::mat4{1.0f}));
}

TEST_CASE("Transform: setPosition") {
    OpenGeoLab::Scene::Transform t;
    t.setPosition({3.0f, 4.0f, 5.0f});
    auto m = t.matrix();
    CHECK(vec3Near(glm::vec3(m[3]), {3.0f, 4.0f, 5.0f}));
}

TEST_CASE("Transform: setScale") {
    OpenGeoLab::Scene::Transform t;
    t.setScale({2.0f, 3.0f, 4.0f});
    auto m = t.matrix();
    CHECK(std::abs(m[0][0] - 2.0f) < kEps);
    CHECK(std::abs(m[1][1] - 3.0f) < kEps);
    CHECK(std::abs(m[2][2] - 4.0f) < kEps);
}

TEST_CASE("Transform: setRotation") {
    OpenGeoLab::Scene::Transform t;
    // 90-degree rotation around Z axis
    auto q = glm::angleAxis(glm::radians(90.0f), glm::vec3{0, 0, 1});
    t.setRotation(q);
    auto m = t.matrix();
    // X-axis should map to Y-axis (approx)
    CHECK(std::abs(m[0][1] - 1.0f) < kEps);
    CHECK(std::abs(m[1][0] - (-1.0f)) < kEps);
}

TEST_CASE("Transform: combined TRS") {
    OpenGeoLab::Scene::Transform t;
    t.setPosition({1, 2, 3});
    t.setScale({2, 2, 2});
    auto m = t.matrix();
    // Diagonal should be 2 (scale), translation column should be {1,2,3}
    CHECK(std::abs(m[0][0] - 2.0f) < kEps);
    CHECK(vec3Near(glm::vec3(m[3]), {1.0f, 2.0f, 3.0f}));
}

TEST_CASE("Transform: reset") {
    OpenGeoLab::Scene::Transform t;
    t.setPosition({5, 5, 5});
    t.reset();
    CHECK(mat4Near(t.matrix(), glm::mat4{1.0f}));
}

// ── BoundingBox ─────────────────────────────────────────────────────────────

TEST_CASE("BoundingBox: default is invalid") {
    OpenGeoLab::Scene::BoundingBox bb;
    CHECK_FALSE(bb.isValid());
}

TEST_CASE("BoundingBox: expand with point") {
    OpenGeoLab::Scene::BoundingBox bb;
    bb.expand({1, 2, 3});
    CHECK(bb.isValid());
    CHECK(vec3Near(bb.center(), {1, 2, 3}));
}

TEST_CASE("BoundingBox: expand with two points") {
    OpenGeoLab::Scene::BoundingBox bb;
    bb.expand({0, 0, 0});
    bb.expand({4, 6, 8});
    CHECK(vec3Near(bb.center(), {2, 3, 4}));
    CHECK(bb.radius() > 0.0f);
}

TEST_CASE("BoundingBox: expand with other box") {
    OpenGeoLab::Scene::BoundingBox a;
    a.expand({0, 0, 0});
    a.expand({1, 1, 1});

    OpenGeoLab::Scene::BoundingBox b;
    b.expand({2, 2, 2});
    b.expand({3, 3, 3});

    a.expand(b);
    CHECK(vec3Near(a.center(), {1.5f, 1.5f, 1.5f}));
}

TEST_CASE("BoundingBox: fromPositions") {
    // 3 vertices: (0,0,0), (2,0,0), (0,4,0)
    float data[] = {0, 0, 0, 2, 0, 0, 0, 4, 0};
    auto bb = OpenGeoLab::Scene::BoundingBox::fromPositions(data, 3, sizeof(float) * 3);
    CHECK(bb.isValid());
    CHECK(vec3Near(bb.center(), {1, 2, 0}));
}

TEST_CASE("BoundingBox: reset") {
    OpenGeoLab::Scene::BoundingBox bb;
    bb.expand({1, 2, 3});
    bb.reset();
    CHECK_FALSE(bb.isValid());
}

// ── SceneGraph ──────────────────────────────────────────────────────────────

TEST_CASE("SceneGraph: addNode and findNode") {
    OpenGeoLab::Scene::SceneGraph graph;
    OpenGeoLab::Scene::SceneNode node;
    node.id = "test-node";

    graph.addNode(node);
    auto* found = graph.findNode("test-node");
    CHECK(found != nullptr);
    CHECK(found->id == "test-node");
}

TEST_CASE("SceneGraph: addNode records in changeset") {
    OpenGeoLab::Scene::SceneGraph graph;
    OpenGeoLab::Scene::SceneNode node;
    node.id = "n1";
    graph.addNode(node);

    CHECK(graph.hasChanges());
    auto cs = graph.consumeChangeset();
    CHECK(cs.added.size() == 1);
    CHECK(cs.added[0] == "n1");
}

TEST_CASE("SceneGraph: updateVisual records in changeset") {
    OpenGeoLab::Scene::SceneGraph graph;
    OpenGeoLab::Scene::SceneNode node;
    node.id = "n1";
    graph.addNode(node);
    static_cast<void>(graph.consumeChangeset()); // clear

    OpenGeoLab::Core::VisualData visual;
    graph.updateVisual("n1", visual);

    auto cs = graph.consumeChangeset();
    CHECK(cs.updated.size() == 1);
    CHECK(cs.updated[0] == "n1");
}

TEST_CASE("SceneGraph: removeNode") {
    OpenGeoLab::Scene::SceneGraph graph;
    OpenGeoLab::Scene::SceneNode node;
    node.id = "n1";
    graph.addNode(node);
    static_cast<void>(graph.consumeChangeset());

    graph.removeNode("n1");
    CHECK(graph.findNode("n1") == nullptr);

    auto cs = graph.consumeChangeset();
    CHECK(cs.removed.size() == 1);
    CHECK(cs.removed[0] == "n1");
}

TEST_CASE("SceneGraph: consumeChangeset clears pending") {
    OpenGeoLab::Scene::SceneGraph graph;
    OpenGeoLab::Scene::SceneNode node;
    node.id = "n1";
    graph.addNode(node);

    auto cs1 = graph.consumeChangeset();
    CHECK_FALSE(cs1.empty());

    CHECK_FALSE(graph.hasChanges());
    auto cs2 = graph.consumeChangeset();
    CHECK(cs2.empty());
}

TEST_CASE("SceneGraph: allNodeIds") {
    OpenGeoLab::Scene::SceneGraph graph;
    OpenGeoLab::Scene::SceneNode n1;
    n1.id = "a";
    OpenGeoLab::Scene::SceneNode n2;
    n2.id = "b";

    graph.addNode(n1);
    graph.addNode(n2);

    auto ids = graph.allNodeIds();
    CHECK(ids.size() == 2);
}

TEST_CASE("SceneGraph: sceneBounds merges visible nodes") {
    OpenGeoLab::Scene::SceneGraph graph;

    OpenGeoLab::Scene::SceneNode n1;
    n1.id = "a";
    n1.bounds.expand({0, 0, 0});
    n1.visible = true;

    OpenGeoLab::Scene::SceneNode n2;
    n2.id = "b";
    n2.bounds.expand({10, 10, 10});
    n2.visible = true;

    graph.addNode(n1);
    graph.addNode(n2);

    auto bounds = graph.sceneBounds();
    CHECK(bounds.isValid());
    CHECK(vec3Near(bounds.center(), {5, 5, 5}));
}
