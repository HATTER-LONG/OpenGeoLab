#include "mesh_scene_bridge.hpp"

#include "mesh_render_builder.hpp"

#include <opengeolab/scene/pick_component.hpp>
#include <opengeolab/scene/render_component.hpp>
#include <opengeolab/scene/scene_node.hpp>

#include <memory>
#include <string>
#include <utility>

namespace OpenGeoLab::Mesh {

namespace {

[[nodiscard]] bool isMeshEntity(Core::EntityType type) {
    return type == Core::EntityType::MeshNode || type == Core::EntityType::MeshEdge ||
           type == Core::EntityType::MeshElement;
}

class MeshRenderComponent final : public Scene::IRenderComponent {
public:
    explicit MeshRenderComponent(Scene::RenderMeshData data) : m_data(std::move(data)) {}

    [[nodiscard]] const Scene::RenderMeshData& meshData() const override { return m_data; }

    [[nodiscard]] uint64_t dataVersion() const override { return m_data.version; }

private:
    Scene::RenderMeshData m_data;
};

class MeshPickComponent final : public Scene::IPickComponent {
public:
    explicit MeshPickComponent(const MeshRenderComponent* render_component)
        : m_renderComponent(render_component) {}

    [[nodiscard]] Scene::PickStrategy strategy() const override { return Scene::PickStrategy::Gpu; }

    [[nodiscard]] std::span<const Scene::PickIdEntry> pickEntries() const override {
        if(m_renderComponent == nullptr) {
            return {};
        }
        return m_renderComponent->meshData().pickIds;
    }

private:
    const MeshRenderComponent* m_renderComponent;
};

} // namespace

MeshSceneBridge::MeshSceneBridge(Scene::SceneGraph& scene, MeshStore& store)
    : m_scene(scene), m_store(store) {
    m_connections.push_back(store.meshAdded.connect(
        [this](uint32_t id, const MeshEntry& entry) { onMeshAdded(id, entry); }));
    m_connections.push_back(
        store.meshModified.connect([this](uint32_t id) { onMeshModified(id); }));
    m_connections.push_back(store.meshRemoved.connect([this](uint32_t id) { onMeshRemoved(id); }));
    m_connections.push_back(store.storeCleared.connect([this]() { onStoreCleared(); }));
}

MeshSceneBridge::~MeshSceneBridge() = default;

void MeshSceneBridge::onMeshAdded(uint32_t shape_id, const MeshEntry& entry) {
    if(const auto iterator = m_meshToNode.find(shape_id); iterator != m_meshToNode.end()) {
        m_scene.removeNode(iterator->second);
        m_meshToNode.erase(iterator);
    }

    auto mesh_data = MeshRenderBuilder::build(shape_id, entry);
    const auto node_count = mesh_data.pointRanges.size();
    const auto edge_count = mesh_data.lineRanges.size();
    const auto element_count = mesh_data.triangleRanges.size();
    m_scene.labelManager().removeWhere([=](const Scene::Label3D& label) {
        if(label.entity.shapeId != shape_id || !isMeshEntity(label.entity.entityType)) {
            return false;
        }
        switch(label.entity.entityType) {
        case Core::EntityType::MeshNode:
            return label.entity.localId == 0 || label.entity.localId > node_count;
        case Core::EntityType::MeshEdge:
            return label.entity.localId == 0 || label.entity.localId > edge_count;
        case Core::EntityType::MeshElement:
            return label.entity.localId == 0 || label.entity.localId > element_count;
        default:
            return false;
        }
    });
    if(mesh_data.vertices.empty()) {
        return;
    }

    const auto node_id = m_scene.addNode("mesh_" + std::to_string(shape_id));
    if(node_id == 0) {
        return;
    }

    m_meshToNode[shape_id] = node_id;
    m_scene.setNodeSource(node_id, "mesh", shape_id);

    m_scene.configureNode(node_id, [&](Scene::SceneNode& node) {
        node.setLocalBounds(mesh_data.bounds);
        auto render_component = std::make_unique<MeshRenderComponent>(std::move(mesh_data));
        const MeshRenderComponent* render_component_ptr = render_component.get();
        node.setRenderComponent(std::move(render_component));
        node.setPickComponent(std::make_unique<MeshPickComponent>(render_component_ptr));
    });
}

void MeshSceneBridge::onMeshModified(uint32_t shape_id) {
    const auto entry = m_store.meshCopy(shape_id);
    if(entry.has_value()) {
        onMeshAdded(shape_id, *entry);
    }
}

void MeshSceneBridge::onMeshRemoved(uint32_t shape_id) {
    m_scene.labelManager().removeWhere([shape_id](const Scene::Label3D& label) {
        return label.entity.shapeId == shape_id && isMeshEntity(label.entity.entityType);
    });
    if(const auto iterator = m_meshToNode.find(shape_id); iterator != m_meshToNode.end()) {
        m_scene.removeNode(iterator->second);
        m_meshToNode.erase(iterator);
    }
}

void MeshSceneBridge::onStoreCleared() {
    m_scene.labelManager().removeWhere(
        [](const Scene::Label3D& label) { return isMeshEntity(label.entity.entityType); });
    for(const auto& [shape_id, node_id] : m_meshToNode) {
        static_cast<void>(shape_id);
        m_scene.removeNode(node_id);
    }
    m_meshToNode.clear();
}

} // namespace OpenGeoLab::Mesh
