#include <opengeolab/render/render_scene_snapshot.hpp>

#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/scene_node.hpp>

#include <algorithm>
#include <functional>

namespace OpenGeoLab::Render {

std::size_t
RenderSceneSnapshot::EntityRefKeyHash::operator()(const EntityRefKey& key) const noexcept {
    auto hash = std::hash<uint32_t>{}(key.shapeId);
    hash ^= std::hash<uint8_t>{}(static_cast<uint8_t>(key.entityType)) + 0x9e3779b9U +
            (hash << 6U) + (hash >> 2U);
    hash ^= std::hash<uint32_t>{}(key.localId) + 0x9e3779b9U + (hash << 6U) + (hash >> 2U);
    return hash;
}

void RenderSceneSnapshot::clear() {
    m_sceneVersion = 0;
    m_vertices.clear();
    m_pickIds.clear();
    m_indices.clear();
    m_triangleRanges.clear();
    m_lineRanges.clear();
    m_pointRanges.clear();
    m_entityIndex.clear();
}

void RenderSceneSnapshot::rebuild(const Scene::SceneGraph& scene) {
    clear();

    const auto scene_snapshot = scene.visibleRenderSnapshot();
    for(const auto& mesh : scene_snapshot.meshes) {

        const auto vertex_base = static_cast<uint32_t>(m_vertices.size());
        const auto index_base = static_cast<uint32_t>(m_indices.size());
        m_vertices.insert(m_vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
        m_pickIds.insert(m_pickIds.end(), mesh.pickIds.begin(), mesh.pickIds.end());
        for(const uint32_t index : mesh.indices) {
            m_indices.push_back(index + vertex_base);
        }

        const auto append_ranges = [vertex_base,
                                    index_base](const std::vector<Scene::DrawRange>& source,
                                                std::vector<Scene::DrawRange>& destination) {
            for(auto range : source) {
                range.vertexOffset += vertex_base;
                range.indexOffset += index_base;
                destination.push_back(range);
            }
        };
        append_ranges(mesh.triangleRanges, m_triangleRanges);
        append_ranges(mesh.lineRanges, m_lineRanges);
        append_ranges(mesh.pointRanges, m_pointRanges);
    }

    m_sceneVersion = scene_snapshot.sceneVersion;
    rebuildEntityIndex();
}

void RenderSceneSnapshot::rebuildEntityIndex() {
    const auto add_ranges = [this](const std::vector<Scene::DrawRange>& ranges) {
        for(const auto& range : ranges) {
            m_entityIndex[{range.shapeId, range.entityType, range.localId}].push_back(range);
        }
    };
    add_ranges(m_triangleRanges);
    add_ranges(m_lineRanges);
    add_ranges(m_pointRanges);
}

std::span<const Scene::DrawRange> RenderSceneSnapshot::lookupEntity(uint32_t shape_id,
                                                                    Core::EntityType entity_type,
                                                                    uint32_t local_id) const {
    const auto it = m_entityIndex.find({shape_id, entity_type, local_id});
    return it != m_entityIndex.end() ? std::span<const Scene::DrawRange>{it->second}
                                     : std::span<const Scene::DrawRange>{};
}

std::vector<glm::vec3> RenderSceneSnapshot::readVertexPositions(size_t offset, size_t count) const {
    if(offset >= m_vertices.size()) {
        return {};
    }
    const size_t end = std::min(offset + count, m_vertices.size());
    std::vector<glm::vec3> positions;
    positions.reserve(end - offset);
    for(size_t i = offset; i < end; ++i) {
        const auto& position = m_vertices[i].position;
        positions.emplace_back(position[0], position[1], position[2]);
    }
    return positions;
}

} // namespace OpenGeoLab::Render
