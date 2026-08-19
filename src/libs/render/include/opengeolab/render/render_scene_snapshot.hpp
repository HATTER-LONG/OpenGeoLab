/**
 * @file render_scene_snapshot.hpp
 * @brief Backend-neutral, flattened render data for one scene version.
 */

#pragma once

#include <opengeolab/render/render_export.hpp>

#include <opengeolab/scene/render_mesh_data.hpp>

#include <glm/vec3.hpp>

#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Scene {
class SceneGraph;
}

namespace OpenGeoLab::Render {

/**
 * @brief CPU-side scene representation shared by graphics backends.
 *
 * The snapshot flattens visible render components, fixes their vertex/index
 * offsets, and builds the entity lookup used by highlighting and labels. It
 * intentionally owns no OpenGL, Vulkan, Metal, or D3D resource. This keeps
 * scene traversal and entity resolution identical for the legacy OpenGL and
 * the Qt RHI renderer.
 */
class OPENGEOLAB_RENDER_EXPORT RenderSceneSnapshot final {
public:
    /** Rebuild from an atomic copy of the visible portion of @p scene. */
    void rebuild(const Scene::SceneGraph& scene);

    /** Clear all geometry and reset the tracked scene version. */
    void clear();

    [[nodiscard]] bool empty() const noexcept { return m_vertices.empty(); }
    [[nodiscard]] uint64_t sceneVersion() const noexcept { return m_sceneVersion; }

    [[nodiscard]] std::span<const Scene::RenderVertex> vertices() const noexcept {
        return m_vertices;
    }
    [[nodiscard]] std::span<const Scene::PickIdEntry> pickIds() const noexcept { return m_pickIds; }
    [[nodiscard]] std::span<const uint32_t> indices() const noexcept { return m_indices; }
    [[nodiscard]] std::span<const Scene::DrawRange> triangleRanges() const noexcept {
        return m_triangleRanges;
    }
    [[nodiscard]] std::span<const Scene::DrawRange> lineRanges() const noexcept {
        return m_lineRanges;
    }
    [[nodiscard]] std::span<const Scene::DrawRange> pointRanges() const noexcept {
        return m_pointRanges;
    }

    [[nodiscard]] std::span<const Scene::DrawRange>
    lookupEntity(uint32_t shape_id, Core::EntityType entity_type, uint32_t local_id) const;

    [[nodiscard]] std::vector<glm::vec3> readVertexPositions(size_t offset, size_t count) const;

private:
    struct EntityRefKey {
        uint32_t shapeId{};
        Core::EntityType entityType{};
        uint32_t localId{};
        bool operator==(const EntityRefKey&) const = default;
    };

    struct EntityRefKeyHash {
        std::size_t operator()(const EntityRefKey& key) const noexcept;
    };

    void rebuildEntityIndex();

    uint64_t m_sceneVersion{0};
    std::vector<Scene::RenderVertex> m_vertices;
    std::vector<Scene::PickIdEntry> m_pickIds;
    std::vector<uint32_t> m_indices;
    std::vector<Scene::DrawRange> m_triangleRanges;
    std::vector<Scene::DrawRange> m_lineRanges;
    std::vector<Scene::DrawRange> m_pointRanges;
    std::unordered_map<EntityRefKey, std::vector<Scene::DrawRange>, EntityRefKeyHash> m_entityIndex;
};

} // namespace OpenGeoLab::Render
