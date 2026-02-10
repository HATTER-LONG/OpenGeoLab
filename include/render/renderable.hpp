/**
 * @file renderable.hpp
 * @brief Renderable and RenderBatch definitions for batched GPU drawing
 *
 * Encapsulates drawable objects and supports batching/instancing to reduce
 * draw calls. Each RenderBatch groups meshes with compatible GPU state
 * (same VAO layout, same material) for efficient rendering.
 */

#pragma once

#include "geometry/geometry_types.hpp"
#include "render/render_data.hpp"

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QVector4D>
#include <memory>
#include <unordered_set>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief GPU-side buffers for a single renderable mesh
 *
 * Holds VAO/VBO/EBO and metadata identifying the source entity.
 */
struct RenderableBuffer {
    std::unique_ptr<QOpenGLVertexArrayObject> m_vao;
    std::unique_ptr<QOpenGLBuffer> m_vbo;
    std::unique_ptr<QOpenGLBuffer> m_ebo;

    int m_vertexCount{0};
    int m_indexCount{0};
    RenderPrimitiveType m_primitiveType{RenderPrimitiveType::Triangles};

    Geometry::EntityType m_entityType{Geometry::EntityType::None};
    Geometry::EntityUID m_entityUid{Geometry::INVALID_ENTITY_UID};

    Geometry::EntityUID m_owningPartUid{Geometry::INVALID_ENTITY_UID};
    Geometry::EntityUID m_owningSolidUid{Geometry::INVALID_ENTITY_UID};
    std::unordered_set<Geometry::EntityUID> m_owningWireUid{};

    QVector3D m_centroid{0.0f, 0.0f, 0.0f}; ///< Geometric centroid for outline scaling

    QVector4D m_hoverColor{1.0f, 1.0f, 1.0f, 1.0f};
    QVector4D m_selectedColor{1.0f, 1.0f, 1.0f, 1.0f};

    RenderableBuffer();
    ~RenderableBuffer();

    RenderableBuffer(RenderableBuffer&&) noexcept = default;
    RenderableBuffer& operator=(RenderableBuffer&&) noexcept = default;
    RenderableBuffer(const RenderableBuffer&) = delete;
    RenderableBuffer& operator=(const RenderableBuffer&) = delete;

    void destroy();
};

/**
 * @brief Collection of renderable buffers organized by entity type
 *
 * RenderBatch holds all uploaded GPU buffers and provides methods
 * to upload from DocumentRenderData and draw individual meshes.
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
     * @brief Upload all mesh data from DocumentRenderData to the GPU.
     * @param gl OpenGL functions
     * @param data Source render data
     */
    void upload(QOpenGLFunctions& gl, const DocumentRenderData& data);

    /**
     * @brief Release all GPU resources.
     */
    void clear();

    [[nodiscard]] std::vector<RenderableBuffer>& faceMeshes() { return m_faceMeshBuffers; }
    [[nodiscard]] std::vector<RenderableBuffer>& edgeMeshes() { return m_edgeMeshBuffers; }
    [[nodiscard]] std::vector<RenderableBuffer>& vertexMeshes() { return m_vertexMeshBuffers; }
    [[nodiscard]] std::vector<RenderableBuffer>& meshElementMeshes() {
        return m_meshElementMeshBuffers;
    }
    [[nodiscard]] std::vector<RenderableBuffer>& meshNodeMeshes() { return m_meshNodeMeshBuffers; }

    [[nodiscard]] const std::vector<RenderableBuffer>& faceMeshes() const {
        return m_faceMeshBuffers;
    }
    [[nodiscard]] const std::vector<RenderableBuffer>& edgeMeshes() const {
        return m_edgeMeshBuffers;
    }
    [[nodiscard]] const std::vector<RenderableBuffer>& vertexMeshes() const {
        return m_vertexMeshBuffers;
    }
    [[nodiscard]] const std::vector<RenderableBuffer>& meshElementMeshes() const {
        return m_meshElementMeshBuffers;
    }
    [[nodiscard]] const std::vector<RenderableBuffer>& meshNodeMeshes() const {
        return m_meshNodeMeshBuffers;
    }

    [[nodiscard]] bool empty() const {
        return m_faceMeshBuffers.empty() && m_edgeMeshBuffers.empty() &&
               m_vertexMeshBuffers.empty() && m_meshElementMeshBuffers.empty() &&
               m_meshNodeMeshBuffers.empty();
    }

    /**
     * @brief Draw a single renderable buffer.
     * @param gl OpenGL functions
     * @param buf The buffer to draw
     * @param primitive_override If non-zero, override the buffer's primitive type
     */
    static void draw(QOpenGLFunctions& gl, RenderableBuffer& buf, GLenum primitive_override = 0);

private:
    void uploadMeshList(QOpenGLFunctions& gl,
                        const std::vector<RenderMesh>& meshes,
                        std::vector<RenderableBuffer>& out,
                        bool need_index);

    static void uploadSingleMesh(QOpenGLFunctions& gl,
                                 const RenderMesh& mesh,
                                 QOpenGLVertexArrayObject& vao,
                                 QOpenGLBuffer& vbo,
                                 QOpenGLBuffer& ebo);

    std::vector<RenderableBuffer> m_faceMeshBuffers;
    std::vector<RenderableBuffer> m_edgeMeshBuffers;
    std::vector<RenderableBuffer> m_vertexMeshBuffers;
    std::vector<RenderableBuffer> m_meshElementMeshBuffers;
    std::vector<RenderableBuffer> m_meshNodeMeshBuffers;
};

} // namespace OpenGeoLab::Render
