/**
 * @file scene_node_test.cpp
 * @brief Unit tests for SceneNode
 */

#include <opengeolab/scene/scene_node.hpp>

#include <doctest/doctest.h>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <utility>

namespace OpenGeoLab::Scene::Tests {

namespace {

class TestRenderComponent final : public IRenderComponent {
public:
    [[nodiscard]] const RenderMeshData& meshData() const override { return m_meshData; }

    [[nodiscard]] uint64_t dataVersion() const override { return 7; }

private:
    RenderMeshData m_meshData;
};

class TestPickComponent final : public IPickComponent {
public:
    [[nodiscard]] PickStrategy strategy() const override { return PickStrategy::Gpu; }

    [[nodiscard]] std::span<const PickIdEntry> pickEntries() const override { return m_entries; }

private:
    std::array<PickIdEntry, 1> m_entries{PickIdEntry{1}};
};

void checkVec3(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

void checkMat4(const glm::mat4& actual, const glm::mat4& expected) {
    for(int column = 0; column < 4; ++column) {
        for(int row = 0; row < 4; ++row) {
            CHECK(actual[column][row] == doctest::Approx(expected[column][row]));
        }
    }
}

} // namespace

TEST_CASE("SceneNode defaults expose configured state") {
    const SceneNode node{42, "root"};

    CHECK(node.id() == 42);
    CHECK(std::string{node.name()} == "root");
    CHECK(node.parent() == nullptr);
    CHECK(node.children().empty());
    CHECK(node.isVisible());
    CHECK(node.displayMode() == DisplayMode::SolidWithEdges);
    CHECK_FALSE(node.isSelected());
    CHECK_FALSE(node.isHovered());
    CHECK(node.version() == 0);
    CHECK(node.renderComponent() == nullptr);
    CHECK(node.pickComponent() == nullptr);
    checkMat4(node.localTransform(), glm::mat4{1.0F});
}

TEST_CASE("SceneNode computes world matrix and transformed bounds from parent chain") {
    auto parent = std::make_unique<SceneNode>(1, "parent");
    parent->localTransform() = glm::translate(glm::mat4{1.0F}, glm::vec3{10.0F, 0.0F, 0.0F});

    auto child = std::make_unique<SceneNode>(2, "child");
    child->localTransform() = glm::translate(glm::mat4{1.0F}, glm::vec3{0.0F, 5.0F, 0.0F});

    BoundingBox3D bounds;
    bounds.expand(glm::vec3{-1.0F, -1.0F, -1.0F});
    bounds.expand(glm::vec3{2.0F, 3.0F, 4.0F});
    child->setLocalBounds(bounds);

    SceneNode* child_ptr = parent->addChild(std::move(child));
    const glm::mat4 expected_world = glm::translate(glm::mat4{1.0F}, glm::vec3{10.0F, 5.0F, 0.0F});

    checkMat4(child_ptr->worldMatrix(), expected_world);
    checkVec3(child_ptr->worldBounds().min, glm::vec3{9.0F, 4.0F, -1.0F});
    checkVec3(child_ptr->worldBounds().max, glm::vec3{12.0F, 8.0F, 4.0F});
}

TEST_CASE("SceneNode manages child ownership and parent pointers") {
    SceneNode node{1, "node"};
    auto child = std::make_unique<SceneNode>(2, "child");

    SceneNode* child_ptr = node.addChild(std::move(child));

    REQUIRE(child_ptr != nullptr);
    CHECK(child_ptr->parent() == &node);
    CHECK(node.findChild(2) == child_ptr);
    CHECK(node.findChild(99) == nullptr);
    REQUIRE(node.children().size() == 1);
    CHECK(node.children().front().get() == child_ptr);

    std::unique_ptr<SceneNode> removed = node.removeChild(2);

    REQUIRE(removed != nullptr);
    CHECK(removed->id() == 2);
    CHECK(removed->parent() == nullptr);
    CHECK(node.children().empty());
    CHECK(node.removeChild(77) == nullptr);
}

TEST_CASE("SceneNode stores components and interaction state") {
    SceneNode node{3, "node"};
    auto render_component = std::make_unique<TestRenderComponent>();
    auto pick_component = std::make_unique<TestPickComponent>();
    TestRenderComponent* render_ptr = render_component.get();
    TestPickComponent* pick_ptr = pick_component.get();

    node.setName("renamed");
    node.setVisible(false);
    node.setDisplayMode(DisplayMode::Wireframe);
    node.setSelected(true);
    node.setHovered(true);
    node.setRenderComponent(std::move(render_component));
    node.setPickComponent(std::move(pick_component));
    node.markDirty();
    node.markDirty();

    CHECK(std::string{node.name()} == "renamed");
    CHECK_FALSE(node.isVisible());
    CHECK(node.displayMode() == DisplayMode::Wireframe);
    CHECK(node.isSelected());
    CHECK(node.isHovered());
    CHECK(node.renderComponent() == render_ptr);
    CHECK(node.pickComponent() == pick_ptr);
    CHECK(node.version() == 2);
}

} // namespace OpenGeoLab::Scene::Tests
