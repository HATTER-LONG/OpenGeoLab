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

/** @brief Range offset/count pair for building DrawRange. */
struct RangeSpan {
    uint32_t vertexOffset;
    uint32_t vertexCount;
    uint32_t indexOffset;
    uint32_t indexCount;
};

/** @brief Resolve tag for the first primitive in a range. */
DrawRange taggedRange(uint32_t shape_id,
                      PrimitiveTopology topology,
                      const RangeSpan& span,
                      const std::optional<Core::EntityTag>& tag) {
    return {
        .shapeId = shape_id,
        .entityType = tag.has_value() ? tag->type : Core::EntityType::SceneNode,
        .localId = tag.has_value() ? tag->localId : 0U,
        .vertexOffset = span.vertexOffset,
        .vertexCount = span.vertexCount,
        .indexOffset = span.indexOffset,
        .indexCount = span.indexCount,
        .topology = topology,
    };
}

void attachComponents(SceneGraph& scene,
                      NodeId node_id,
                      uint32_t shape_id,
                      const Geometry::ShapeEntry& entry) {
    RenderMeshData mesh_data = GeometrySceneBridge::buildRenderData(shape_id, entry);
    scene.configureNode(node_id, [&](SceneNode& node) {
        node.setLocalBounds(mesh_data.bounds);
        auto render_component = std::make_unique<ShapeRenderComponent>(std::move(mesh_data));
        ShapeRenderComponent const* render_component_ptr = render_component.get();
        node.setRenderComponent(std::move(render_component));
        node.setPickComponent(std::make_unique<ShapePickComponent>(render_component_ptr));
    });
}

/** @brief Shared state for incremental mesh building inside buildRenderData. */
struct BuildState {
    RenderMeshData& result;
    uint32_t shapeId;
    Core::RenderColor shapeColor;
    uint32_t globalVertexOffset{0};
    uint32_t globalIndexOffset{0};
};

void processSurface(BuildState& state,
                    const Core::SurfaceMesh& surface,
                    const std::vector<Core::EntityTag>& triangle_tags,
                    std::size_t& triangle_tag_index) {
    assert(surface.positions.size() / 3U <= std::numeric_limits<uint32_t>::max());
    assert(surface.indices.size() <= std::numeric_limits<uint32_t>::max());
    const uint32_t vertex_count = static_cast<uint32_t>(surface.positions.size() / 3U);
    const uint32_t index_count = static_cast<uint32_t>(surface.indices.size());
    const uint32_t triangle_count = index_count / 3U;
    const bool has_colors = !surface.colors.empty();
    const std::size_t pick_offset = state.result.pickIds.size();
    const std::size_t range_tag_index = triangle_tag_index;

    state.result.pickIds.insert(state.result.pickIds.end(), vertex_count, PickIdEntry{});

    for(uint32_t vi = 0; vi < vertex_count; ++vi) {
        RenderVertex vertex;
        vertex.position[0] = surface.positions[vi * 3U];
        vertex.position[1] = surface.positions[vi * 3U + 1U];
        vertex.position[2] = surface.positions[vi * 3U + 2U];

        if(surface.normals.size() >= (static_cast<std::size_t>(vi) + 1U) * 3U) {
            vertex.normal[0] = surface.normals[vi * 3U];
            vertex.normal[1] = surface.normals[vi * 3U + 1U];
            vertex.normal[2] = surface.normals[vi * 3U + 2U];
        }

        if(has_colors && surface.colors.size() >= (static_cast<std::size_t>(vi) + 1U) * 4U) {
            vertex.color[0] = surface.colors[vi * 4U];
            vertex.color[1] = surface.colors[vi * 4U + 1U];
            vertex.color[2] = surface.colors[vi * 4U + 2U];
            vertex.color[3] = surface.colors[vi * 4U + 3U];
        } else {
            vertex.color[0] = state.shapeColor.r;
            vertex.color[1] = state.shapeColor.g;
            vertex.color[2] = state.shapeColor.b;
            vertex.color[3] = state.shapeColor.a;
        }

        state.result.bounds.expand(
            glm::vec3{vertex.position[0], vertex.position[1], vertex.position[2]});
        state.result.vertices.push_back(vertex);
    }

    for(uint32_t ti = 0; ti < triangle_count; ++ti) {
        uint64_t pick_id = 0;
        if(triangle_tag_index < triangle_tags.size()) {
            const Core::EntityTag& tag = triangle_tags[triangle_tag_index];
            pick_id = PickId::encode(state.shapeId, tag.type, tag.localId);
        }

        for(uint32_t corner = 0; corner < 3U; ++corner) {
            const uint32_t lvi = surface.indices[ti * 3U + corner];
            if(lvi < vertex_count) {
                state.result.pickIds[pick_offset + lvi].pickId = pick_id;
            }
        }

        ++triangle_tag_index;
    }

    for(const uint32_t index : surface.indices) {
        state.result.indices.push_back(state.globalVertexOffset + index);
    }

    const std::optional<Core::EntityTag> first_tag =
        range_tag_index < triangle_tags.size()
            ? std::optional<Core::EntityTag>{triangle_tags[range_tag_index]}
            : std::nullopt;
    if(vertex_count > 0U) {
        const RangeSpan span{state.globalVertexOffset, vertex_count, state.globalIndexOffset,
                             index_count};
        state.result.triangleRanges.push_back(
            taggedRange(state.shapeId, PrimitiveTopology::Triangles, span, first_tag));
    }

    state.globalVertexOffset += vertex_count;
    state.globalIndexOffset += index_count;
}

void processEdge(BuildState& state,
                 const Core::EdgeMesh& edge,
                 const std::vector<Core::EntityTag>& edge_tags,
                 std::size_t& edge_tag_index) {
    assert(edge.positions.size() / 3U <= std::numeric_limits<uint32_t>::max());
    assert(edge.indices.size() <= std::numeric_limits<uint32_t>::max());
    const uint32_t vertex_count = static_cast<uint32_t>(edge.positions.size() / 3U);
    const uint32_t index_count = static_cast<uint32_t>(edge.indices.size());
    const uint32_t segment_count = index_count / 2U;
    const std::size_t pick_offset = state.result.pickIds.size();
    const std::size_t range_tag_index = edge_tag_index;

    state.result.pickIds.insert(state.result.pickIds.end(), vertex_count, PickIdEntry{});

    for(uint32_t vi = 0; vi < vertex_count; ++vi) {
        RenderVertex vertex;
        vertex.position[0] = edge.positions[vi * 3U];
        vertex.position[1] = edge.positions[vi * 3U + 1U];
        vertex.position[2] = edge.positions[vi * 3U + 2U];
        vertex.color[0] = Core::K_EDGE_COLOR.r;
        vertex.color[1] = Core::K_EDGE_COLOR.g;
        vertex.color[2] = Core::K_EDGE_COLOR.b;
        vertex.color[3] = Core::K_EDGE_COLOR.a;
        state.result.bounds.expand(
            glm::vec3{vertex.position[0], vertex.position[1], vertex.position[2]});
        state.result.vertices.push_back(vertex);
    }

    for(uint32_t si = 0; si < segment_count; ++si) {
        uint64_t pick_id = 0;
        if(edge_tag_index < edge_tags.size()) {
            const Core::EntityTag& tag = edge_tags[edge_tag_index];
            pick_id = PickId::encode(state.shapeId, tag.type, tag.localId);
        }

        for(uint32_t endpoint = 0; endpoint < 2U; ++endpoint) {
            const uint32_t lvi = edge.indices[si * 2U + endpoint];
            if(lvi < vertex_count) {
                state.result.pickIds[pick_offset + lvi].pickId = pick_id;
            }
        }

        ++edge_tag_index;
    }

    for(const uint32_t index : edge.indices) {
        state.result.indices.push_back(state.globalVertexOffset + index);
    }

    const std::optional<Core::EntityTag> first_tag =
        range_tag_index < edge_tags.size()
            ? std::optional<Core::EntityTag>{edge_tags[range_tag_index]}
            : std::nullopt;
    if(vertex_count > 0U) {
        const RangeSpan span{state.globalVertexOffset, vertex_count, state.globalIndexOffset,
                             index_count};
        state.result.lineRanges.push_back(
            taggedRange(state.shapeId, PrimitiveTopology::Lines, span, first_tag));
    }

    state.globalVertexOffset += vertex_count;
    state.globalIndexOffset += index_count;
}

void processPointSet(BuildState& state,
                     const Core::PointSet& point_set,
                     const std::vector<Core::EntityTag>& vertex_tags,
                     std::size_t& vertex_tag_index) {
    assert(point_set.positions.size() / 3U <= std::numeric_limits<uint32_t>::max());
    const uint32_t vertex_count = static_cast<uint32_t>(point_set.positions.size() / 3U);
    const std::size_t range_tag_index = vertex_tag_index;

    for(uint32_t vi = 0; vi < vertex_count; ++vi) {
        RenderVertex vertex;
        vertex.position[0] = point_set.positions[vi * 3U];
        vertex.position[1] = point_set.positions[vi * 3U + 1U];
        vertex.position[2] = point_set.positions[vi * 3U + 2U];
        vertex.color[0] = Core::K_VERTEX_COLOR.r;
        vertex.color[1] = Core::K_VERTEX_COLOR.g;
        vertex.color[2] = Core::K_VERTEX_COLOR.b;
        vertex.color[3] = Core::K_VERTEX_COLOR.a;
        state.result.bounds.expand(
            glm::vec3{vertex.position[0], vertex.position[1], vertex.position[2]});
        state.result.vertices.push_back(vertex);

        uint64_t pick_id = 0;
        if(vertex_tag_index < vertex_tags.size()) {
            const Core::EntityTag& tag = vertex_tags[vertex_tag_index];
            pick_id = PickId::encode(state.shapeId, tag.type, tag.localId);
        }
        state.result.pickIds.push_back(PickIdEntry{pick_id});
        ++vertex_tag_index;
    }

    const std::optional<Core::EntityTag> first_tag =
        range_tag_index < vertex_tags.size()
            ? std::optional<Core::EntityTag>{vertex_tags[range_tag_index]}
            : std::nullopt;
    if(vertex_count > 0U) {
        const RangeSpan span{state.globalVertexOffset, vertex_count, 0U, 0U};
        state.result.pointRanges.push_back(
            taggedRange(state.shapeId, PrimitiveTopology::Points, span, first_tag));
    }

    state.globalVertexOffset += vertex_count;
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
    BuildState state{result, shape_id, Core::colorForShapeId(shape_id)};
    std::size_t triangle_tag_index = 0;
    std::size_t edge_tag_index = 0;
    std::size_t vertex_tag_index = 0;

    for(const Core::SurfaceMesh& surface : visual.surfaces) {
        processSurface(state, surface, entry.triangleTags, triangle_tag_index);
    }

    for(const Core::EdgeMesh& edge : visual.edges) {
        processEdge(state, edge, entry.edgeTags, edge_tag_index);
    }

    for(const Core::PointSet& point_set : visual.points) {
        processPointSet(state, point_set, entry.vertexTags, vertex_tag_index);
    }

    assert(result.pickIds.size() == result.vertices.size());
    return result;
}

void GeometrySceneBridge::onShapeAdded(uint32_t shape_id, const Geometry::ShapeEntry& entry) {
    m_topoIndex.buildForShape(shape_id, entry);
    if(entry.visualData == nullptr || m_shapeToNode.contains(shape_id)) {
        return;
    }

    const NodeId node_id = m_scene.addNode(entry.name);
    if(node_id == 0) {
        return;
    }

    m_shapeToNode[shape_id] = node_id;
    m_scene.setNodeSource(node_id, "geometry", shape_id);
    attachComponents(m_scene, node_id, shape_id, entry);
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

    NodeId node_id = 0;
    if(const auto iterator = m_shapeToNode.find(shape_id); iterator != m_shapeToNode.end()) {
        if(m_scene.findNode(iterator->second) != nullptr) {
            node_id = iterator->second;
        } else {
            m_shapeToNode.erase(iterator);
        }
    }

    if(node_id == 0) {
        if(entry.visualData == nullptr) {
            return;
        }

        node_id = m_scene.addNode(entry.name);
        if(node_id == 0) {
            return;
        }
        m_shapeToNode[shape_id] = node_id;
        m_scene.setNodeSource(node_id, "geometry", shape_id);
        attachComponents(m_scene, node_id, shape_id, entry);
        return;
    }

    if(entry.visualData == nullptr) {
        m_scene.configureNode(node_id, [&](SceneNode& node) { node.setName(entry.name); });
        return;
    }

    RenderMeshData mesh_data = buildRenderData(shape_id, entry);
    m_scene.configureNode(node_id, [&](SceneNode& node) {
        node.setName(entry.name);
        node.setLocalBounds(mesh_data.bounds);

        if(auto* render_component = dynamic_cast<ShapeRenderComponent*>(node.renderComponent());
           render_component != nullptr) {
            render_component->updateData(std::move(mesh_data));
        } else {
            auto new_render_component =
                std::make_unique<ShapeRenderComponent>(std::move(mesh_data));
            ShapeRenderComponent const* render_component_ptr = new_render_component.get();
            node.setPickComponent(nullptr);
            node.setRenderComponent(std::move(new_render_component));
            node.setPickComponent(std::make_unique<ShapePickComponent>(render_component_ptr));
        }

        if(node.pickComponent() == nullptr) {
            if(auto* render_component = dynamic_cast<ShapeRenderComponent*>(node.renderComponent());
               render_component != nullptr) {
                node.setPickComponent(std::make_unique<ShapePickComponent>(render_component));
            }
        }
    });
}

} // namespace OpenGeoLab::Scene
