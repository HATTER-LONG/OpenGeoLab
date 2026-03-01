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
 * Vertex layout matches RenderVertex (48 bytes per vertex, packed):
 *   location 0  — position  (vec3,  offset  0)
 *   location 1  — normal    (vec3,  offset 12)
 *   location 2  — color     (vec4,  offset 24)
 *   location 3  — pickId    (uvec2, offset 40)
 *
 * pickId (uint64_t on CPU) is split into two GL_UNSIGNED_INT components
 * on the GPU. The encoding is: [56-bit UID | 8-bit type], treated as
 * little-endian uint64_t.
 *
 * Thread-safety: GpuBuffer requires GL context and is NOT thread-safe.
 * All operations must occur on the GL rendering thread.
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
     * Binds the VAO, uploads buffer data via glBufferData, configures
     * vertex attribute pointers, and performs GL error checking.
     * Tracks the uploaded version number internally for efficient re-upload detection.
     *
     * @param data RenderPassData to upload (const - version tracking done internally)
     * @return true on successful upload, false if GL error occurred
     */
    [[nodiscard]] bool upload(const RenderPassData& data);

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
    uint32_t m_uploadedDataVersion{
        0}; ///< Tracks the last uploaded data version for re-upload detection
    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
