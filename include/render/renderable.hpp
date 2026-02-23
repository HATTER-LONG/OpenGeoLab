/**
 * @file renderable.hpp
 * @brief Renderable and RenderBatch definitions for batched GPU drawing
 *
 * Each category (faces, edges, vertices, mesh elements, mesh nodes)
 * uses a single RenderableBuffer (VAO/VBO/EBO). RenderBatch holds all
 * category buffers and per-entity metadata for sub-draw operations.
 */

#pragma once

#include "render/render_data.hpp"
#include "render/render_types.hpp"

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <memory>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief GPU-side buffers for a batched category of entities
 *
 * Holds VAO/VBO/EBO and aggregate draw counts. No per-entity metadata;
 * entity info is stored in RenderEntityInfoMap within RenderBatch.
 */
struct RenderableBuffer {
    std::unique_ptr<QOpenGLVertexArrayObject> m_vao;
    std::unique_ptr<QOpenGLBuffer> m_vbo;
    std::unique_ptr<QOpenGLBuffer> m_ebo;

    int m_vertexCount{0};
    int m_indexCount{0};
    RenderPrimitiveType m_primitiveType{RenderPrimitiveType::Triangles};

    RenderableBuffer();
    ~RenderableBuffer();

    RenderableBuffer(RenderableBuffer&&) noexcept = default;
    RenderableBuffer& operator=(RenderableBuffer&&) noexcept = default;
    RenderableBuffer(const RenderableBuffer&) = delete;
    RenderableBuffer& operator=(const RenderableBuffer&) = delete;

    /** @brief Release GPU resources (VAO, VBO, EBO). */
    void destroy();

    [[nodiscard]] bool isValid() const { return m_vertexCount > 0; }
};

/**
 * @brief Batched render data organized by entity category
 *
 * One RenderableBuffer per category, plus RenderEntityInfoMaps
 * for per-entity sub-draw, selection, and hover operations.
 */
class RenderBatch {
public:
    RenderBatch() = default;
    ~RenderBatch() = default;

    RenderBatch(RenderBatch&&) = default;
    RenderBatch& operator=(RenderBatch&&) = default;
    RenderBatch(const RenderBatch&) = delete;
    RenderBatch& operator=(const RenderBatch&) = delete;

    /**
     * @brief Upload all batched mesh data from DocumentRenderData to the GPU.
     */
    void upload(QOpenGLFunctions& gl, const DocumentRenderData& data);

    /** @brief Destroy all category buffers and clear entity maps. */
    void clear();

    [[nodiscard]] RenderableBuffer& faceBuffer() { return m_faceBuffer; }
    [[nodiscard]] RenderableBuffer& edgeBuffer() { return m_edgeBuffer; }
    [[nodiscard]] RenderableBuffer& vertexBuffer() { return m_vertexBuffer; }
    [[nodiscard]] RenderableBuffer& meshElementBuffer() { return m_meshElementBuffer; }
    [[nodiscard]] RenderableBuffer& meshNodeBuffer() { return m_meshNodeBuffer; }

    [[nodiscard]] const RenderableBuffer& faceBuffer() const { return m_faceBuffer; }
    [[nodiscard]] const RenderableBuffer& edgeBuffer() const { return m_edgeBuffer; }
    [[nodiscard]] const RenderableBuffer& vertexBuffer() const { return m_vertexBuffer; }
    [[nodiscard]] const RenderableBuffer& meshElementBuffer() const { return m_meshElementBuffer; }
    [[nodiscard]] const RenderableBuffer& meshNodeBuffer() const { return m_meshNodeBuffer; }

    [[nodiscard]] RenderEntityInfoMap& faceEntities() { return m_faceEntities; }
    [[nodiscard]] RenderEntityInfoMap& edgeEntities() { return m_edgeEntities; }
    [[nodiscard]] RenderEntityInfoMap& vertexEntities() { return m_vertexEntities; }
    [[nodiscard]] RenderEntityInfoMap& meshElementEntities() { return m_meshElementEntities; }
    [[nodiscard]] RenderEntityInfoMap& meshNodeEntities() { return m_meshNodeEntities; }

    [[nodiscard]] const RenderEntityInfoMap& faceEntities() const { return m_faceEntities; }
    [[nodiscard]] const RenderEntityInfoMap& edgeEntities() const { return m_edgeEntities; }
    [[nodiscard]] const RenderEntityInfoMap& vertexEntities() const { return m_vertexEntities; }
    [[nodiscard]] const RenderEntityInfoMap& meshElementEntities() const {
        return m_meshElementEntities;
    }
    [[nodiscard]] const RenderEntityInfoMap& meshNodeEntities() const { return m_meshNodeEntities; }

    [[nodiscard]] bool empty() const {
        return !m_faceBuffer.isValid() && !m_edgeBuffer.isValid() && !m_vertexBuffer.isValid() &&
               !m_meshElementBuffer.isValid() && !m_meshNodeBuffer.isValid();
    }

    /**
     * @brief Draw all geometry in a buffer with one draw call.
     * @param gl Active OpenGL functions context
     * @param buf RenderableBuffer to draw
     * @param primitive_override Override primitive type (0 = use buffer default)
     */
    static void drawAll(QOpenGLFunctions& gl, RenderableBuffer& buf, GLenum primitive_override = 0);

    /**
     * @brief Draw a sub-range of the index buffer (for indexed categories like faces/edges).
     * @param gl Active OpenGL functions context
     * @param buf RenderableBuffer to draw from
     * @param index_offset Starting index offset in the EBO
     * @param index_count Number of indices to draw
     */
    static void drawIndexRange(QOpenGLFunctions& gl,
                               RenderableBuffer& buf,
                               uint32_t index_offset,
                               uint32_t index_count);

    /**
     * @brief Draw a sub-range of the vertex buffer (for non-indexed categories like points).
     * @param gl Active OpenGL functions context
     * @param buf RenderableBuffer to draw from
     * @param vertex_offset Starting vertex offset in the VBO
     * @param vertex_count Number of vertices to draw
     * @param primitive OpenGL primitive type (e.g. GL_POINTS, GL_LINES)
     */
    static void drawVertexRange(QOpenGLFunctions& gl,
                                RenderableBuffer& buf,
                                uint32_t vertex_offset,
                                uint32_t vertex_count,
                                GLenum primitive);

private:
    void uploadCategory(QOpenGLFunctions& gl, const RenderMesh& mesh, RenderableBuffer& buf);

    static void setupVertexAttributes(QOpenGLFunctions& gl);

    RenderableBuffer m_faceBuffer;
    RenderableBuffer m_edgeBuffer;
    RenderableBuffer m_vertexBuffer;
    RenderableBuffer m_meshElementBuffer;
    RenderableBuffer m_meshNodeBuffer;

    RenderEntityInfoMap m_faceEntities;
    RenderEntityInfoMap m_edgeEntities;
    RenderEntityInfoMap m_vertexEntities;
    RenderEntityInfoMap m_meshElementEntities;
    RenderEntityInfoMap m_meshNodeEntities;
};

} // namespace OpenGeoLab::Render
