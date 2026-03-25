/**
 * @file vertex_array_object.hpp
 * @brief RAII wrapper for OpenGL Vertex Array Object with DSA.
 */
#pragma once

#include <opengeolab/render/render_export.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>

#include <cstdint>

namespace OpenGeoLab::Render {

/**
 * @brief RAII wrapper for a GL 4.5 Vertex Array Object.
 *
 * Manages VAO + position VBO + normal VBO + EBO lifecycle.
 * Move-only. Uses Direct State Access (DSA) for buffer setup.
 */
class OPENGEOLAB_RENDER_EXPORT VertexArrayObject {
public:
    VertexArrayObject() = default;
    ~VertexArrayObject();

    VertexArrayObject(const VertexArrayObject&) = delete;
    VertexArrayObject& operator=(const VertexArrayObject&) = delete;
    VertexArrayObject(VertexArrayObject&& other) noexcept;
    VertexArrayObject& operator=(VertexArrayObject&& other) noexcept;

    /**
     * @brief Upload mesh data to GPU buffers.
     * @param mesh Render mesh data with positions, normals, and indices.
     *
     * Creates VAO, VBOs for positions (location 0) and normals (location 1),
     * and an EBO. Replaces any previously uploaded data.
     */
    void upload(const Scene::RenderMeshData& mesh);

    /** @brief Issue the draw call (binds VAO, calls glDrawElements). */
    void draw() const;

    /** @brief Release all GL resources. */
    void release();

    /** @brief Check if VAO holds valid GL resources. */
    [[nodiscard]] bool isValid() const;

private:
    std::uint32_t vao_ = 0;
    std::uint32_t positionVbo_ = 0;
    std::uint32_t normalVbo_ = 0;
    std::uint32_t ebo_ = 0;
    std::int32_t indexCount_ = 0;
    std::uint32_t drawMode_ = 0; ///< GL_TRIANGLES, GL_LINES, etc.
};

} // namespace OpenGeoLab::Render
