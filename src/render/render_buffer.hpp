#pragma once
#include "render/render_data.hpp"

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QVector4D>

namespace OpenGeoLab::Render {
struct RenderableBuffer {
    std::unique_ptr<QOpenGLVertexArrayObject> m_vao;
    std::unique_ptr<QOpenGLBuffer> m_vbo;
    std::unique_ptr<QOpenGLBuffer> m_ebo;

    int m_vertexCount{0};
    int m_indexCount{0};
    RenderPrimitiveType m_primitiveType{RenderPrimitiveType::Triangles};

    uint32_t m_uid{0};
    uint8_t m_type{0};

    Geometry::EntityUID m_owningPartUid{Geometry::INVALID_ENTITY_UID};
    Geometry::EntityUID m_owningSolidUid{Geometry::INVALID_ENTITY_UID};
    std::unordered_set<Geometry::EntityUID> m_owningWireUid{};

    QVector3D m_centroid{0.0f, 0.0f, 0.0f};

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

class RenderBatch {
public:
    RenderBatch() = default;
    ~RenderBatch() = default;

    RenderBatch(RenderBatch&&) = default;
    RenderBatch& operator=(RenderBatch&&) = default;
    RenderBatch(const RenderBatch&) = delete;
    RenderBatch& operator=(const RenderBatch&) = delete;

    void upload(QOpenGLFunctions& gl, const DocumentRenderData& data);

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