/**
 * @file gpu_buffer_manager.hpp
 * @brief Manages VAO/VBO/IBO for scene geometry on the GPU
 */

#pragma once

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <glad/gl.h>

#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Uploads scene RenderMeshData to GPU buffers.
 *
 * Maintains two VAO configurations:
 *   Main VAO: position + normal + color (for visible passes)
 *   Pick VAO: position + pickId (for selection pass)
 *
 * Checks SceneGraph version on each synchronize() and re-uploads
 * only when data has changed.
 */
class GpuBufferManager final {
public:
    void initialize();
    void cleanup();

    /**
     * @brief Traverse visible nodes and upload changed data to GPU.
     *
     * Caller must hold SceneGraph read lock.
     */
    void synchronize(const Scene::SceneGraph& scene);

    /** @brief Bind the main rendering VAO (pos+normal+color, IBO). */
    void bindMainVao() const;

    /** @brief Bind the pick VAO (pos+pickId, IBO). */
    void bindPickVao() const;

    /** @brief Unbind VAO. */
    void unbind() const;

    [[nodiscard]] const std::vector<Scene::DrawRange>& triangleRanges() const noexcept;
    [[nodiscard]] const std::vector<Scene::DrawRange>& lineRanges() const noexcept;
    [[nodiscard]] const std::vector<Scene::DrawRange>& pointRanges() const noexcept;

    /**
     * @brief Look up all DrawRanges for an entity.
     * @return Span of DrawRanges, empty if entity not found.
     */
    [[nodiscard]] std::span<const Scene::DrawRange>
    lookupEntity(uint32_t shape_id, Core::EntityType entity_type, uint32_t local_id) const;

    [[nodiscard]] bool hasData() const noexcept;
    /** @brief Raw handle to the main interleaved VBO (pos+normal+color). */
    [[nodiscard]] GLuint mainVbo() const noexcept { return m_mainVbo; }

    /** @brief Raw handle to the shared index buffer (uint32_t). */
    [[nodiscard]] GLuint ibo() const noexcept { return m_ibo; }

private:
    void rebuildBuffers(const Scene::SceneGraph& scene);
    void rebuildEntityIndex();
    void setupMainVao();
    void setupPickVao();

    struct EntityRefKey {
        uint32_t shapeId{};
        Core::EntityType entityType{};
        uint32_t localId{};
        bool operator==(const EntityRefKey&) const = default;
    };

    struct EntityRefKeyHash {
        std::size_t operator()(const EntityRefKey& k) const noexcept {
            auto h = std::hash<uint32_t>{}(k.shapeId);
            h ^= std::hash<uint8_t>{}(static_cast<uint8_t>(k.entityType)) + 0x9e3779b9 + (h << 6) +
                 (h >> 2);
            h ^= std::hash<uint32_t>{}(k.localId) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    GLuint m_mainVao{0};
    GLuint m_pickVao{0};
    GLuint m_mainVbo{0};
    GLuint m_pickVbo{0};
    GLuint m_ibo{0};

    uint64_t m_uploadedVersion{0};

    std::vector<Scene::DrawRange> m_triangleRanges;
    std::vector<Scene::DrawRange> m_lineRanges;
    std::vector<Scene::DrawRange> m_pointRanges;

    std::unordered_map<EntityRefKey, std::vector<Scene::DrawRange>, EntityRefKeyHash> m_entityIndex;

    bool m_hasData{false};
};

} // namespace OpenGeoLab::Render
