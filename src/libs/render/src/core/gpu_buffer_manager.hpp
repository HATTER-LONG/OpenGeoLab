/**
 * @file gpu_buffer_manager.hpp
 * @brief Manages VAO/VBO/IBO for scene geometry on the GPU
 */

#pragma once

#include <opengeolab/scene/render_mesh_data.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <glad/gl.h>

#include <cstdint>
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

    [[nodiscard]] bool hasData() const noexcept;

private:
    void rebuildBuffers(const Scene::SceneGraph& scene);
    void setupMainVao();
    void setupPickVao();

    GLuint m_mainVao{0};
    GLuint m_pickVao{0};
    GLuint m_mainVbo{0};
    GLuint m_pickVbo{0};
    GLuint m_ibo{0};

    uint64_t m_uploadedVersion{0};

    std::vector<Scene::DrawRange> m_triangleRanges;
    std::vector<Scene::DrawRange> m_lineRanges;
    std::vector<Scene::DrawRange> m_pointRanges;

    bool m_hasData{false};
};

} // namespace OpenGeoLab::Render
