/**
 * @file gpu_buffer.hpp
 * @brief VAO / VBO / IBO resource management for render passes
 */

#pragma once

#include <QOpenGLExtraFunctions>
#include <cstdint>

namespace OpenGeoLab::Render {

struct RenderPassData; // forward declaration

/**
 * @brief Manages a VAO + VBO + IBO triplet for uploading and drawing
 *        RenderPassData on the GPU.
 *
 * Vertex layout matches RenderVertex (48 bytes per vertex):
 *   location 0  — position  (vec3,  offset  0)
 *   location 1  — normal    (vec3,  offset 12)
 *   location 2  — color     (vec4,  offset 24)
 *   location 3  — pickId    (uvec2, offset 40)
 */
class GpuBuffer {
public:
    GpuBuffer() = default;
    ~GpuBuffer();

    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;

    /** @brief Create VAO, VBO, and IBO GL objects */
    void initialize();

    /** @brief Delete all GL objects and reset state */
    void cleanup();

    /**
     * @brief Upload vertex and index data from a RenderPassData snapshot.
     *
     * Binds the VAO, uploads buffer data via glBufferData, and configures
     * vertex attribute pointers.
     */
    void upload(const RenderPassData& data);

    /** @brief Bind the VAO for drawing */
    void bindForDraw();

    /** @brief Unbind the VAO */
    void unbind();

    // ── Accessors ────────────────────────────────────────────────────────

    [[nodiscard]] uint32_t vertexCount() const { return m_vertexCount; }
    [[nodiscard]] uint32_t indexCount() const { return m_indexCount; }
    [[nodiscard]] bool hasIndices() const { return m_indexCount > 0; }

private:
    GLuint m_vao{0};
    GLuint m_vbo{0};
    GLuint m_ibo{0};
    uint32_t m_vertexCount{0};
    uint32_t m_indexCount{0};
    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
