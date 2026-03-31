/**
 * @file geometry_scene_bridge.cpp
 * @brief GeometrySceneBridge implementation
 */

#include <opengeolab/scene/geometry_scene_bridge.hpp>

#include <opengeolab/core/color_map.hpp>
#include <opengeolab/geometry/shape_entry.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <glm/vec3.hpp>

#include <cassert>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

namespace OpenGeoLab::Scene {

namespace {

class ShapeRenderComponent final : public IRenderComponent {
public:
    explicit ShapeRenderComponent(RenderMeshData data) : m_data(std::move(data)) {}

    [[nodiscard]] const RenderMeshData& meshData() const override { return m_data; }

    [[nodiscard]] uint64_t dataVersion() const override { return m_data.version; }

    void updateData(RenderMeshData data) {
        m_data = std::move(data);
        m_data.markUpdated();
    }

private:
    RenderMeshData m_data;
};

class ShapePickComponent final : public IPickComponent {
public:
    explicit ShapePickComponent(const ShapeRenderComponent* renderComponent)
        : m_renderComponent(renderComponent) {}

    [[nodiscard]] PickStrategy strategy() const override { return PickStrategy::Gpu; }

    [[nodiscard]] std::span<const PickIdEntry> pickEntries() const override {
        if(m_renderComponent == nullptr) {
            return {};
        }
        return m_renderComponent->meshData().pickIds;
    }

private:
    const ShapeRenderComponent* m_renderComponent;
};

[[nodiscard]] std::optional<DrawRange> makeRange(uint32_t shapeId,
                                                 PrimitiveTopology topology,
                                                 uint32_t vertexOffset,
                                                 uint32_t vertexCount,
                                                 uint32_t indexOffset,
                                                 uint32_t indexCount,
                                                 const std::optional<Core::EntityTag>& tag) {
    if(vertexCount == 0U) {
        return std::nullopt;
    }

    DrawRange range;
    range.shapeId = shapeId;
    range.entityType = tag.has_value() ? tag->type : Core::EntityType::SceneNode;
    range.localId = tag.has_value() ? tag->localId : 0U;
    range.vertexOffset = vertexOffset;
    range.vertexCount = vertexCount;
    range.indexOffset = indexOffset;
    range.indexCount = indexCount;
    range.topology = topology;
    return range;
}

void attachComponents(SceneGraph& scene,
                      SceneNode& node,
                      uint32_t shapeId,
                      const Geometry::ShapeEntry& entry) {
    RenderMeshData meshData = GeometrySceneBridge::buildRenderData(shapeId, entry);
    node.setLocalBounds(meshData.bounds);

    auto renderComponent = std::make_unique<ShapeRenderComponent>(std::move(meshData));
    ShapeRenderComponent* renderComponentPtr = renderComponent.get();
    node.setRenderComponent(std::move(renderComponent));
    node.setPickComponent(std::make_unique<ShapePickComponent>(renderComponentPtr));
    node.markDirty();
    scene.nodeUpdated(node.id());
}

} // namespace

GeometrySceneBridge::GeometrySceneBridge(SceneGraph& scene,
                                         Geometry::ShapeStore& store,
                                         TopologyIndex& topoIndex)
    : m_scene(scene), m_store(store), m_topoIndex(topoIndex) {
    m_connections.push_back(
        store.shapeAdded.connect([this](uint32_t shapeId, const Geometry::ShapeEntry& entry) {
            onShapeAdded(shapeId, entry);
        }));
    m_connections.push_back(
        store.shapeRemoved.connect([this](uint32_t shapeId) { onShapeRemoved(shapeId); }));
    m_connections.push_back(
        store.shapeUpdated.connect([this](uint32_t shapeId, const Geometry::ShapeEntry& entry) {
            onShapeUpdated(shapeId, entry);
        }));
}

GeometrySceneBridge::~GeometrySceneBridge() = default;

RenderMeshData GeometrySceneBridge::buildRenderData(uint32_t shapeId,
                                                    const Geometry::ShapeEntry& entry) {
    RenderMeshData result;
    if(entry.visualData == nullptr) {
        return result;
    }

    const Core::VisualData& visual = *entry.visualData;
    const Core::RenderColor shapeColor = Core::colorForShapeId(shapeId);
    uint32_t globalVertexOffset = 0;
    uint32_t globalIndexOffset = 0;
    std::size_t triangleTagIndex = 0;
    std::size_t edgeTagIndex = 0;
    std::size_t vertexTagIndex = 0;

    for(const Core::SurfaceMesh& surface : visual.surfaces) {
        assert(surface.positions.size() / 3U <= std::numeric_limits<uint32_t>::max());
        assert(surface.indices.size() <= std::numeric_limits<uint32_t>::max());
        const uint32_t vertexCount = static_cast<uint32_t>(surface.positions.size() / 3U);
        const uint32_t indexCount = static_cast<uint32_t>(surface.indices.size());
        const uint32_t triangleCount = indexCount / 3U;
        const bool hasColors = !surface.colors.empty();
        const std::size_t pickOffset = result.pickIds.size();
        const std::size_t rangeTagIndex = triangleTagIndex;

        result.pickIds.insert(result.pickIds.end(), vertexCount, PickIdEntry{});

        for(uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
            RenderVertex vertex;
            vertex.position[0] = surface.positions[vertexIndex * 3U];
            vertex.position[1] = surface.positions[vertexIndex * 3U + 1U];
            vertex.position[2] = surface.positions[vertexIndex * 3U + 2U];

            if(surface.normals.size() >= (static_cast<std::size_t>(vertexIndex) + 1U) * 3U) {
                vertex.normal[0] = surface.normals[vertexIndex * 3U];
                vertex.normal[1] = surface.normals[vertexIndex * 3U + 1U];
                vertex.normal[2] = surface.normals[vertexIndex * 3U + 2U];
            }

            if(hasColors &&
               surface.colors.size() >= (static_cast<std::size_t>(vertexIndex) + 1U) * 4U) {
                vertex.color[0] = surface.colors[vertexIndex * 4U];
                vertex.color[1] = surface.colors[vertexIndex * 4U + 1U];
                vertex.color[2] = surface.colors[vertexIndex * 4U + 2U];
                vertex.color[3] = surface.colors[vertexIndex * 4U + 3U];
            } else {
                vertex.color[0] = shapeColor.r;
                vertex.color[1] = shapeColor.g;
                vertex.color[2] = shapeColor.b;
                vertex.color[3] = shapeColor.a;
            }

            result.bounds.expand(
                glm::vec3{vertex.position[0], vertex.position[1], vertex.position[2]});
            result.vertices.push_back(vertex);
        }

        for(uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex) {
            uint64_t pickId = 0;
            if(triangleTagIndex < entry.triangleTags.size()) {
                const Core::EntityTag& tag = entry.triangleTags[triangleTagIndex];
                pickId = PickId::encode(shapeId, tag.type, tag.localId);
            }

            for(uint32_t corner = 0; corner < 3U; ++corner) {
                const uint32_t localVertexIndex = surface.indices[triangleIndex * 3U + corner];
                if(localVertexIndex < vertexCount) {
                    result.pickIds[pickOffset + localVertexIndex].pickId = pickId;
                }
            }

            ++triangleTagIndex;
        }

        for(const uint32_t index : surface.indices) {
            result.indices.push_back(globalVertexOffset + index);
        }

        const std::optional<Core::EntityTag> firstTriangleTag =
            rangeTagIndex < entry.triangleTags.size()
                ? std::optional<Core::EntityTag>{entry.triangleTags[rangeTagIndex]}
                : std::nullopt;
        if(const auto range =
               makeRange(shapeId, PrimitiveTopology::Triangles, globalVertexOffset, vertexCount,
                         globalIndexOffset, indexCount, firstTriangleTag)) {
            result.triangleRanges.push_back(*range);
        }

        globalVertexOffset += vertexCount;
        globalIndexOffset += indexCount;
    }

    for(const Core::EdgeMesh& edge : visual.edges) {
        assert(edge.positions.size() / 3U <= std::numeric_limits<uint32_t>::max());
        assert(edge.indices.size() <= std::numeric_limits<uint32_t>::max());
        const uint32_t vertexCount = static_cast<uint32_t>(edge.positions.size() / 3U);
        const uint32_t indexCount = static_cast<uint32_t>(edge.indices.size());
        const uint32_t segmentCount = indexCount / 2U;
        const std::size_t pickOffset = result.pickIds.size();
        const std::size_t rangeTagIndex = edgeTagIndex;

        result.pickIds.insert(result.pickIds.end(), vertexCount, PickIdEntry{});

        for(uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
            RenderVertex vertex;
            vertex.position[0] = edge.positions[vertexIndex * 3U];
            vertex.position[1] = edge.positions[vertexIndex * 3U + 1U];
            vertex.position[2] = edge.positions[vertexIndex * 3U + 2U];
            vertex.color[0] = Core::K_EDGE_COLOR.r;
            vertex.color[1] = Core::K_EDGE_COLOR.g;
            vertex.color[2] = Core::K_EDGE_COLOR.b;
            vertex.color[3] = Core::K_EDGE_COLOR.a;
            result.bounds.expand(
                glm::vec3{vertex.position[0], vertex.position[1], vertex.position[2]});
            result.vertices.push_back(vertex);
        }

        for(uint32_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
            uint64_t pickId = 0;
            if(edgeTagIndex < entry.edgeTags.size()) {
                const Core::EntityTag& tag = entry.edgeTags[edgeTagIndex];
                pickId = PickId::encode(shapeId, tag.type, tag.localId);
            }

            for(uint32_t endpoint = 0; endpoint < 2U; ++endpoint) {
                const uint32_t localVertexIndex = edge.indices[segmentIndex * 2U + endpoint];
                if(localVertexIndex < vertexCount) {
                    result.pickIds[pickOffset + localVertexIndex].pickId = pickId;
                }
            }

            ++edgeTagIndex;
        }

        for(const uint32_t index : edge.indices) {
            result.indices.push_back(globalVertexOffset + index);
        }

        const std::optional<Core::EntityTag> firstEdgeTag =
            rangeTagIndex < entry.edgeTags.size()
                ? std::optional<Core::EntityTag>{entry.edgeTags[rangeTagIndex]}
                : std::nullopt;
        if(const auto range = makeRange(shapeId, PrimitiveTopology::Lines, globalVertexOffset,
                                        vertexCount, globalIndexOffset, indexCount, firstEdgeTag)) {
            result.lineRanges.push_back(*range);
        }

        globalVertexOffset += vertexCount;
        globalIndexOffset += indexCount;
    }

    for(const Core::PointSet& pointSet : visual.points) {
        assert(pointSet.positions.size() / 3U <= std::numeric_limits<uint32_t>::max());
        const uint32_t vertexCount = static_cast<uint32_t>(pointSet.positions.size() / 3U);
        const std::size_t rangeTagIndex = vertexTagIndex;

        for(uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
            RenderVertex vertex;
            vertex.position[0] = pointSet.positions[vertexIndex * 3U];
            vertex.position[1] = pointSet.positions[vertexIndex * 3U + 1U];
            vertex.position[2] = pointSet.positions[vertexIndex * 3U + 2U];
            vertex.color[0] = Core::K_VERTEX_COLOR.r;
            vertex.color[1] = Core::K_VERTEX_COLOR.g;
            vertex.color[2] = Core::K_VERTEX_COLOR.b;
            vertex.color[3] = Core::K_VERTEX_COLOR.a;
            result.bounds.expand(
                glm::vec3{vertex.position[0], vertex.position[1], vertex.position[2]});
            result.vertices.push_back(vertex);

            uint64_t pickId = 0;
            if(vertexTagIndex < entry.vertexTags.size()) {
                const Core::EntityTag& tag = entry.vertexTags[vertexTagIndex];
                pickId = PickId::encode(shapeId, tag.type, tag.localId);
            }
            result.pickIds.push_back(PickIdEntry{pickId});
            ++vertexTagIndex;
        }

        const std::optional<Core::EntityTag> firstVertexTag =
            rangeTagIndex < entry.vertexTags.size()
                ? std::optional<Core::EntityTag>{entry.vertexTags[rangeTagIndex]}
                : std::nullopt;
        if(const auto range = makeRange(shapeId, PrimitiveTopology::Points, globalVertexOffset,
                                        vertexCount, 0U, 0U, firstVertexTag)) {
            result.pointRanges.push_back(*range);
        }

        globalVertexOffset += vertexCount;
    }

    assert(result.pickIds.size() == result.vertices.size());
    return result;
}

void GeometrySceneBridge::onShapeAdded(uint32_t shapeId, const Geometry::ShapeEntry& entry) {
    m_topoIndex.buildForShape(shapeId, entry);
    if(entry.visualData == nullptr || m_shapeToNode.contains(shapeId)) {
        return;
    }

    SceneNode* node = m_scene.addNode(entry.name);
    if(node == nullptr) {
        return;
    }

    m_shapeToNode[shapeId] = node->id();
    attachComponents(m_scene, *node, shapeId, entry);
}

void GeometrySceneBridge::onShapeRemoved(uint32_t shapeId) {
    if(const auto iterator = m_shapeToNode.find(shapeId); iterator != m_shapeToNode.end()) {
        m_scene.removeNode(iterator->second);
        m_shapeToNode.erase(iterator);
    }

    m_topoIndex.removeShape(shapeId);
}

void GeometrySceneBridge::onShapeUpdated(uint32_t shapeId, const Geometry::ShapeEntry& entry) {
    m_topoIndex.buildForShape(shapeId, entry);

    SceneNode* node = nullptr;
    if(const auto iterator = m_shapeToNode.find(shapeId); iterator != m_shapeToNode.end()) {
        node = m_scene.findNode(iterator->second);
        if(node == nullptr) {
            m_shapeToNode.erase(iterator);
        }
    }

    if(node == nullptr) {
        if(entry.visualData == nullptr) {
            return;
        }

        node = m_scene.addNode(entry.name);
        if(node == nullptr) {
            return;
        }
        m_shapeToNode[shapeId] = node->id();
        attachComponents(m_scene, *node, shapeId, entry);
        return;
    }

    node->setName(entry.name);
    if(entry.visualData == nullptr) {
        node->markDirty();
        m_scene.nodeUpdated(node->id());
        return;
    }

    RenderMeshData meshData = buildRenderData(shapeId, entry);
    node->setLocalBounds(meshData.bounds);

    if(auto* renderComponent = dynamic_cast<ShapeRenderComponent*>(node->renderComponent());
       renderComponent != nullptr) {
        renderComponent->updateData(std::move(meshData));
    } else {
        auto newRenderComponent = std::make_unique<ShapeRenderComponent>(std::move(meshData));
        ShapeRenderComponent* renderComponentPtr = newRenderComponent.get();
        node->setPickComponent(nullptr); // clear before destroying old render to avoid dangling ref
        node->setRenderComponent(std::move(newRenderComponent));
        node->setPickComponent(std::make_unique<ShapePickComponent>(renderComponentPtr));
    }

    if(node->pickComponent() == nullptr) {
        if(auto* renderComponent = dynamic_cast<ShapeRenderComponent*>(node->renderComponent());
           renderComponent != nullptr) {
            node->setPickComponent(std::make_unique<ShapePickComponent>(renderComponent));
        }
    }

    node->markDirty();
    m_scene.nodeUpdated(node->id());
}

} // namespace OpenGeoLab::Scene
