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
    explicit ShapePickComponent(const ShapeRenderComponent* render_component)
        : m_renderComponent(render_component) {}

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

[[nodiscard]] std::optional<DrawRange> makeRange(uint32_t shape_id,
                                                 PrimitiveTopology topology,
                                                 uint32_t vertex_offset,
                                                 uint32_t vertex_count,
                                                 uint32_t index_offset,
                                                 uint32_t index_count,
                                                 const std::optional<Core::EntityTag>& tag) {
    if(vertex_count == 0U) {
        return std::nullopt;
    }

    DrawRange range;
    range.shapeId = shape_id;
    range.entityType = tag.has_value() ? tag->type : Core::EntityType::SceneNode;
    range.localId = tag.has_value() ? tag->localId : 0U;
    range.vertexOffset = vertex_offset;
    range.vertexCount = vertex_count;
    range.indexOffset = index_offset;
    range.indexCount = index_count;
    range.topology = topology;
    return range;
}

void attachComponents(SceneGraph& scene,
                      SceneNode& node,
                      uint32_t shape_id,
                      const Geometry::ShapeEntry& entry) {
    RenderMeshData mesh_data = GeometrySceneBridge::buildRenderData(shape_id, entry);
    node.setLocalBounds(mesh_data.bounds);

    auto render_component = std::make_unique<ShapeRenderComponent>(std::move(mesh_data));
    ShapeRenderComponent const* render_component_ptr = render_component.get();
    node.setRenderComponent(std::move(render_component));
    node.setPickComponent(std::make_unique<ShapePickComponent>(render_component_ptr));
    node.markDirty();
    scene.nodeUpdated(node.id());
}

} // namespace

GeometrySceneBridge::GeometrySceneBridge(SceneGraph& scene,
                                         Geometry::ShapeStore& store,
                                         TopologyIndex& topo_index)
    : m_scene(scene), m_store(store), m_topoIndex(topo_index) {
    m_connections.push_back(
        store.shapeAdded.connect([this](uint32_t shape_id, const Geometry::ShapeEntry& entry) {
            onShapeAdded(shape_id, entry);
        }));
    m_connections.push_back(
        store.shapeRemoved.connect([this](uint32_t shape_id) { onShapeRemoved(shape_id); }));
    m_connections.push_back(
        store.shapeUpdated.connect([this](uint32_t shape_id, const Geometry::ShapeEntry& entry) {
            onShapeUpdated(shape_id, entry);
        }));
}

GeometrySceneBridge::~GeometrySceneBridge() = default;

RenderMeshData GeometrySceneBridge::buildRenderData(uint32_t shape_id,
                                                    const Geometry::ShapeEntry& entry) {
    RenderMeshData result;
    if(entry.visualData == nullptr) {
        return result;
    }

    const Core::VisualData& visual = *entry.visualData;
    const Core::RenderColor shape_color = Core::colorForShapeId(shape_id);
    uint32_t global_vertex_offset = 0;
    uint32_t global_index_offset = 0;
    std::size_t triangle_tag_index = 0;
    std::size_t edge_tag_index = 0;
    std::size_t vertex_tag_index = 0;

    for(const Core::SurfaceMesh& surface : visual.surfaces) {
        assert(surface.positions.size() / 3U <= std::numeric_limits<uint32_t>::max());
        assert(surface.indices.size() <= std::numeric_limits<uint32_t>::max());
        const uint32_t vertex_count = static_cast<uint32_t>(surface.positions.size() / 3U);
        const uint32_t index_count = static_cast<uint32_t>(surface.indices.size());
        const uint32_t triangle_count = index_count / 3U;
        const bool has_colors = !surface.colors.empty();
        const std::size_t pick_offset = result.pickIds.size();
        const std::size_t range_tag_index = triangle_tag_index;

        result.pickIds.insert(result.pickIds.end(), vertex_count, PickIdEntry{});

        for(uint32_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
            RenderVertex vertex;
            vertex.position[0] = surface.positions[vertex_index * 3U];
            vertex.position[1] = surface.positions[vertex_index * 3U + 1U];
            vertex.position[2] = surface.positions[vertex_index * 3U + 2U];

            if(surface.normals.size() >= (static_cast<std::size_t>(vertex_index) + 1U) * 3U) {
                vertex.normal[0] = surface.normals[vertex_index * 3U];
                vertex.normal[1] = surface.normals[vertex_index * 3U + 1U];
                vertex.normal[2] = surface.normals[vertex_index * 3U + 2U];
            }

            if(has_colors &&
               surface.colors.size() >= (static_cast<std::size_t>(vertex_index) + 1U) * 4U) {
                vertex.color[0] = surface.colors[vertex_index * 4U];
                vertex.color[1] = surface.colors[vertex_index * 4U + 1U];
                vertex.color[2] = surface.colors[vertex_index * 4U + 2U];
                vertex.color[3] = surface.colors[vertex_index * 4U + 3U];
            } else {
                vertex.color[0] = shape_color.r;
                vertex.color[1] = shape_color.g;
                vertex.color[2] = shape_color.b;
                vertex.color[3] = shape_color.a;
            }

            result.bounds.expand(
                glm::vec3{vertex.position[0], vertex.position[1], vertex.position[2]});
            result.vertices.push_back(vertex);
        }

        for(uint32_t triangle_index = 0; triangle_index < triangle_count; ++triangle_index) {
            uint64_t pick_id = 0;
            if(triangle_tag_index < entry.triangleTags.size()) {
                const Core::EntityTag& tag = entry.triangleTags[triangle_tag_index];
                pick_id = PickId::encode(shape_id, tag.type, tag.localId);
            }

            for(uint32_t corner = 0; corner < 3U; ++corner) {
                const uint32_t local_vertex_index = surface.indices[triangle_index * 3U + corner];
                if(local_vertex_index < vertex_count) {
                    result.pickIds[pick_offset + local_vertex_index].pickId = pick_id;
                }
            }

            ++triangle_tag_index;
        }

        for(const uint32_t index : surface.indices) {
            result.indices.push_back(global_vertex_offset + index);
        }

        const std::optional<Core::EntityTag> first_triangle_tag =
            range_tag_index < entry.triangleTags.size()
                ? std::optional<Core::EntityTag>{entry.triangleTags[range_tag_index]}
                : std::nullopt;
        if(const auto range =
               makeRange(shape_id, PrimitiveTopology::Triangles, global_vertex_offset, vertex_count,
                         global_index_offset, index_count, first_triangle_tag)) {
            result.triangleRanges.push_back(*range);
        }

        global_vertex_offset += vertex_count;
        global_index_offset += index_count;
    }

    for(const Core::EdgeMesh& edge : visual.edges) {
        assert(edge.positions.size() / 3U <= std::numeric_limits<uint32_t>::max());
        assert(edge.indices.size() <= std::numeric_limits<uint32_t>::max());
        const uint32_t vertex_count = static_cast<uint32_t>(edge.positions.size() / 3U);
        const uint32_t index_count = static_cast<uint32_t>(edge.indices.size());
        const uint32_t segment_count = index_count / 2U;
        const std::size_t pick_offset = result.pickIds.size();
        const std::size_t range_tag_index = edge_tag_index;

        result.pickIds.insert(result.pickIds.end(), vertex_count, PickIdEntry{});

        for(uint32_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
            RenderVertex vertex;
            vertex.position[0] = edge.positions[vertex_index * 3U];
            vertex.position[1] = edge.positions[vertex_index * 3U + 1U];
            vertex.position[2] = edge.positions[vertex_index * 3U + 2U];
            vertex.color[0] = Core::K_EDGE_COLOR.r;
            vertex.color[1] = Core::K_EDGE_COLOR.g;
            vertex.color[2] = Core::K_EDGE_COLOR.b;
            vertex.color[3] = Core::K_EDGE_COLOR.a;
            result.bounds.expand(
                glm::vec3{vertex.position[0], vertex.position[1], vertex.position[2]});
            result.vertices.push_back(vertex);
        }

        for(uint32_t segment_index = 0; segment_index < segment_count; ++segment_index) {
            uint64_t pick_id = 0;
            if(edge_tag_index < entry.edgeTags.size()) {
                const Core::EntityTag& tag = entry.edgeTags[edge_tag_index];
                pick_id = PickId::encode(shape_id, tag.type, tag.localId);
            }

            for(uint32_t endpoint = 0; endpoint < 2U; ++endpoint) {
                const uint32_t local_vertex_index = edge.indices[segment_index * 2U + endpoint];
                if(local_vertex_index < vertex_count) {
                    result.pickIds[pick_offset + local_vertex_index].pickId = pick_id;
                }
            }

            ++edge_tag_index;
        }

        for(const uint32_t index : edge.indices) {
            result.indices.push_back(global_vertex_offset + index);
        }

        const std::optional<Core::EntityTag> first_edge_tag =
            range_tag_index < entry.edgeTags.size()
                ? std::optional<Core::EntityTag>{entry.edgeTags[range_tag_index]}
                : std::nullopt;
        if(const auto range =
               makeRange(shape_id, PrimitiveTopology::Lines, global_vertex_offset, vertex_count,
                         global_index_offset, index_count, first_edge_tag)) {
            result.lineRanges.push_back(*range);
        }

        global_vertex_offset += vertex_count;
        global_index_offset += index_count;
    }

    for(const Core::PointSet& point_set : visual.points) {
        assert(point_set.positions.size() / 3U <= std::numeric_limits<uint32_t>::max());
        const uint32_t vertex_count = static_cast<uint32_t>(point_set.positions.size() / 3U);
        const std::size_t range_tag_index = vertex_tag_index;

        for(uint32_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
            RenderVertex vertex;
            vertex.position[0] = point_set.positions[vertex_index * 3U];
            vertex.position[1] = point_set.positions[vertex_index * 3U + 1U];
            vertex.position[2] = point_set.positions[vertex_index * 3U + 2U];
            vertex.color[0] = Core::K_VERTEX_COLOR.r;
            vertex.color[1] = Core::K_VERTEX_COLOR.g;
            vertex.color[2] = Core::K_VERTEX_COLOR.b;
            vertex.color[3] = Core::K_VERTEX_COLOR.a;
            result.bounds.expand(
                glm::vec3{vertex.position[0], vertex.position[1], vertex.position[2]});
            result.vertices.push_back(vertex);

            uint64_t pick_id = 0;
            if(vertex_tag_index < entry.vertexTags.size()) {
                const Core::EntityTag& tag = entry.vertexTags[vertex_tag_index];
                pick_id = PickId::encode(shape_id, tag.type, tag.localId);
            }
            result.pickIds.push_back(PickIdEntry{pick_id});
            ++vertex_tag_index;
        }

        const std::optional<Core::EntityTag> first_vertex_tag =
            range_tag_index < entry.vertexTags.size()
                ? std::optional<Core::EntityTag>{entry.vertexTags[range_tag_index]}
                : std::nullopt;
        if(const auto range = makeRange(shape_id, PrimitiveTopology::Points, global_vertex_offset,
                                        vertex_count, 0U, 0U, first_vertex_tag)) {
            result.pointRanges.push_back(*range);
        }

        global_vertex_offset += vertex_count;
    }

    assert(result.pickIds.size() == result.vertices.size());
    return result;
}

void GeometrySceneBridge::onShapeAdded(uint32_t shape_id, const Geometry::ShapeEntry& entry) {
    m_topoIndex.buildForShape(shape_id, entry);
    if(entry.visualData == nullptr || m_shapeToNode.contains(shape_id)) {
        return;
    }

    SceneNode* node = m_scene.addNode(entry.name);
    if(node == nullptr) {
        return;
    }

    m_shapeToNode[shape_id] = node->id();
    node->setSource("geometry", shape_id);
    attachComponents(m_scene, *node, shape_id, entry);
}

void GeometrySceneBridge::onShapeRemoved(uint32_t shape_id) {
    if(const auto iterator = m_shapeToNode.find(shape_id); iterator != m_shapeToNode.end()) {
        m_scene.removeNode(iterator->second);
        m_shapeToNode.erase(iterator);
    }

    m_topoIndex.removeShape(shape_id);
}

void GeometrySceneBridge::onShapeUpdated(uint32_t shape_id, const Geometry::ShapeEntry& entry) {
    m_topoIndex.buildForShape(shape_id, entry);

    SceneNode* node = nullptr;
    if(const auto iterator = m_shapeToNode.find(shape_id); iterator != m_shapeToNode.end()) {
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
        m_shapeToNode[shape_id] = node->id();
        node->setSource("geometry", shape_id);
        attachComponents(m_scene, *node, shape_id, entry);
        return;
    }

    node->setName(entry.name);
    if(entry.visualData == nullptr) {
        node->markDirty();
        m_scene.nodeUpdated(node->id());
        return;
    }

    RenderMeshData mesh_data = buildRenderData(shape_id, entry);
    node->setLocalBounds(mesh_data.bounds);

    if(auto* render_component = dynamic_cast<ShapeRenderComponent*>(node->renderComponent());
       render_component != nullptr) {
        render_component->updateData(std::move(mesh_data));
    } else {
        auto new_render_component = std::make_unique<ShapeRenderComponent>(std::move(mesh_data));
        ShapeRenderComponent const* render_component_ptr = new_render_component.get();
        node->setPickComponent(nullptr); // clear before destroying old render to avoid dangling ref
        node->setRenderComponent(std::move(new_render_component));
        node->setPickComponent(std::make_unique<ShapePickComponent>(render_component_ptr));
    }

    if(node->pickComponent() == nullptr) {
        if(auto* render_component = dynamic_cast<ShapeRenderComponent*>(node->renderComponent());
           render_component != nullptr) {
            node->setPickComponent(std::make_unique<ShapePickComponent>(render_component));
        }
    }

    node->markDirty();
    m_scene.nodeUpdated(node->id());
}

} // namespace OpenGeoLab::Scene
