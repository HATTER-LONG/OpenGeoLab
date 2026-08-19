#include <opengeolab/render/render_scene_snapshot.hpp>

#include <opengeolab/scene/render_component.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/scene_node.hpp>

#include <doctest/doctest.h>

#include <memory>

namespace OpenGeoLab::Render::Tests {

namespace {

class TestRenderComponent final : public Scene::IRenderComponent {
public:
    explicit TestRenderComponent(Scene::RenderMeshData data) : m_data(std::move(data)) {}
    [[nodiscard]] const Scene::RenderMeshData& meshData() const override { return m_data; }
    [[nodiscard]] uint64_t dataVersion() const override { return m_data.version; }

private:
    Scene::RenderMeshData m_data;
};

Scene::RenderMeshData makeTriangle(uint32_t shape_id, float x_offset) {
    Scene::RenderMeshData data;
    for(int i = 0; i < 3; ++i) {
        Scene::RenderVertex vertex;
        vertex.position[0] = x_offset + static_cast<float>(i);
        data.vertices.push_back(vertex);
        data.pickIds.push_back({static_cast<uint64_t>(shape_id * 10U + static_cast<uint32_t>(i))});
    }
    data.indices = {0, 1, 2};
    data.triangleRanges.push_back({shape_id, Core::EntityType::GeoFace, 7, 0, 3, 0, 3,
                                   Scene::PrimitiveTopology::Triangles});
    return data;
}

} // namespace

TEST_CASE("RenderSceneSnapshot flattens visible components and adjusts offsets") {
    Scene::SceneGraph scene;
    const auto first = scene.addNode("first");
    const auto second = scene.addNode("second");
    REQUIRE(scene.configureNode(first, [](Scene::SceneNode& node) {
        node.setRenderComponent(std::make_unique<TestRenderComponent>(makeTriangle(1, 0.0F)));
    }));
    REQUIRE(scene.configureNode(second, [](Scene::SceneNode& node) {
        node.setRenderComponent(std::make_unique<TestRenderComponent>(makeTriangle(2, 10.0F)));
    }));

    RenderSceneSnapshot snapshot;
    {
        const auto lock = scene.readLock();
        snapshot.rebuild(scene);
    }

    CHECK(snapshot.sceneVersion() == scene.version());
    CHECK(snapshot.vertices().size() == 6);
    CHECK(snapshot.pickIds().size() == 6);
    CHECK(snapshot.indices().size() == 6);
    CHECK(snapshot.indices()[3] == 3);
    REQUIRE(snapshot.triangleRanges().size() == 2);
    CHECK(snapshot.triangleRanges()[1].vertexOffset == 3);
    CHECK(snapshot.triangleRanges()[1].indexOffset == 3);

    const auto second_face = snapshot.lookupEntity(2, Core::EntityType::GeoFace, 7);
    REQUIRE(second_face.size() == 1);
    CHECK(second_face.front().vertexOffset == 3);

    const auto positions = snapshot.readVertexPositions(3, 2);
    REQUIRE(positions.size() == 2);
    CHECK(positions.front().x == doctest::Approx(10.0F));
}

TEST_CASE("RenderSceneSnapshot excludes invisible nodes") {
    Scene::SceneGraph scene;
    const auto node_id = scene.addNode("hidden");
    REQUIRE(scene.configureNode(node_id, [](Scene::SceneNode& node) {
        node.setRenderComponent(std::make_unique<TestRenderComponent>(makeTriangle(3, 0.0F)));
    }));
    REQUIRE(scene.setNodeVisible(node_id, false));

    RenderSceneSnapshot snapshot;
    {
        const auto lock = scene.readLock();
        snapshot.rebuild(scene);
    }
    CHECK(snapshot.empty());
    CHECK(snapshot.lookupEntity(3, Core::EntityType::GeoFace, 7).empty());
}

} // namespace OpenGeoLab::Render::Tests
