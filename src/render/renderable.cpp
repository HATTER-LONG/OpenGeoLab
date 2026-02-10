/**
 * @file renderable.cpp
 * @brief RenderableBuffer and RenderBatch implementation
 */

#include "render/renderable.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>

namespace OpenGeoLab::Render {

// =============================================================================
// RenderableBuffer
// =============================================================================

RenderableBuffer::RenderableBuffer()
    : m_vao(std::make_unique<QOpenGLVertexArrayObject>()),
      m_vbo(std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer)),
      m_ebo(std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer)) {}

RenderableBuffer::~RenderableBuffer() { destroy(); }

void RenderableBuffer::destroy() {
    if(m_vao && m_vao->isCreated()) {
        m_vao->destroy();
    }
    if(m_vbo && m_vbo->isCreated()) {
        m_vbo->destroy();
    }
    if(m_ebo && m_ebo->isCreated()) {
        m_ebo->destroy();
    }
}

// =============================================================================
// RenderBatch
// =============================================================================

void RenderBatch::upload(QOpenGLFunctions& gl, const DocumentRenderData& data) {
    clear();
    uploadMeshList(gl, data.m_faceMeshes, m_faceMeshBuffers, true);
    uploadMeshList(gl, data.m_edgeMeshes, m_edgeMeshBuffers, true);
    uploadMeshList(gl, data.m_vertexMeshes, m_vertexMeshBuffers, false);
    uploadMeshList(gl, data.m_meshElementMeshes, m_meshElementMeshBuffers, true);
    uploadMeshList(gl, data.m_meshNodeMeshes, m_meshNodeMeshBuffers, false);

    LOG_DEBUG("RenderBatch: Uploaded {} face, {} edge, {} vertex, {} meshElem, {} meshNode buffers",
              m_faceMeshBuffers.size(), m_edgeMeshBuffers.size(), m_vertexMeshBuffers.size(),
              m_meshElementMeshBuffers.size(), m_meshNodeMeshBuffers.size());
}

void RenderBatch::clear() {
    m_faceMeshBuffers.clear();
    m_edgeMeshBuffers.clear();
    m_vertexMeshBuffers.clear();
    m_meshElementMeshBuffers.clear();
    m_meshNodeMeshBuffers.clear();
}

void RenderBatch::draw(QOpenGLFunctions& gl, RenderableBuffer& buf, GLenum primitive_override) {
    const GLenum primitive = primitive_override ? primitive_override : [&]() -> GLenum {
        switch(buf.m_primitiveType) {
        case RenderPrimitiveType::Points:
            return GL_POINTS;
        case RenderPrimitiveType::Lines:
            return GL_LINES;
        case RenderPrimitiveType::LineStrip:
            return GL_LINE_STRIP;
        case RenderPrimitiveType::Triangles:
            return GL_TRIANGLES;
        case RenderPrimitiveType::TriangleStrip:
            return GL_TRIANGLE_STRIP;
        case RenderPrimitiveType::TriangleFan:
            return GL_TRIANGLE_FAN;
        default:
            return GL_TRIANGLES;
        }
    }();

    buf.m_vao->bind();
    if(buf.m_indexCount > 0) {
        buf.m_ebo->bind();
        gl.glDrawElements(primitive, buf.m_indexCount, GL_UNSIGNED_INT, nullptr);
    } else {
        gl.glDrawArrays(primitive, 0, buf.m_vertexCount);
    }
    buf.m_vao->release();
}

void RenderBatch::uploadMeshList(QOpenGLFunctions& gl,
                                 const std::vector<RenderMesh>& meshes,
                                 std::vector<RenderableBuffer>& out,
                                 bool need_index) {
    for(const auto& mesh : meshes) {
        if(!mesh.isValid()) {
            continue;
        }

        RenderableBuffer buf;
        buf.m_vao->create();
        buf.m_vbo->create();
        if(need_index && mesh.isIndexed()) {
            buf.m_ebo->create();
        }

        uploadSingleMesh(gl, mesh, *buf.m_vao, *buf.m_vbo, *buf.m_ebo);

        buf.m_vertexCount = static_cast<int>(mesh.vertexCount());
        buf.m_indexCount = need_index ? static_cast<int>(mesh.indexCount()) : 0;
        buf.m_primitiveType = mesh.m_primitiveType;
        buf.m_entityType = mesh.m_entityType;
        buf.m_entityUid = mesh.m_entityUid;
        buf.m_owningPartUid = mesh.m_owningPart.m_uid;
        buf.m_owningSolidUid = mesh.m_owningSolid.m_uid;
        for(const Geometry::EntityKey& key : mesh.m_owningWire) {
            buf.m_owningWireUid.emplace(key.m_uid);
        }
        buf.m_hoverColor = QVector4D(mesh.m_hoverColor.m_r, mesh.m_hoverColor.m_g,
                                     mesh.m_hoverColor.m_b, mesh.m_hoverColor.m_a);
        buf.m_selectedColor = QVector4D(mesh.m_selectedColor.m_r, mesh.m_selectedColor.m_g,
                                        mesh.m_selectedColor.m_b, mesh.m_selectedColor.m_a);

        // Compute centroid from bounding box center for correct outline scaling
        if(mesh.m_boundingBox.isValid()) {
            const auto center = mesh.m_boundingBox.center();
            buf.m_centroid = QVector3D(static_cast<float>(center.x), static_cast<float>(center.y),
                                       static_cast<float>(center.z));
        }

        out.push_back(std::move(buf));
    }
}

void RenderBatch::uploadSingleMesh(QOpenGLFunctions& gl,
                                   const RenderMesh& mesh,
                                   QOpenGLVertexArrayObject& vao,
                                   QOpenGLBuffer& vbo,
                                   QOpenGLBuffer& ebo) {
    vao.bind();
    vbo.bind();

    vbo.allocate(mesh.m_vertices.data(),
                 static_cast<int>(mesh.m_vertices.size() * sizeof(RenderVertex)));

    // Position (location 0)
    gl.glEnableVertexAttribArray(0);
    gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex),
                             reinterpret_cast<void*>(offsetof(RenderVertex, m_position)));

    // Normal (location 1)
    gl.glEnableVertexAttribArray(1);
    gl.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex),
                             reinterpret_cast<void*>(offsetof(RenderVertex, m_normal)));

    // Color (location 2)
    gl.glEnableVertexAttribArray(2);
    gl.glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(RenderVertex),
                             reinterpret_cast<void*>(offsetof(RenderVertex, m_color)));

    // Upload index data if present
    if(mesh.isIndexed() && ebo.isCreated()) {
        ebo.bind();
        ebo.allocate(mesh.m_indices.data(),
                     static_cast<int>(mesh.m_indices.size() * sizeof(uint32_t)));
    }

    vao.release();
    vbo.release();
    if(ebo.isCreated()) {
        ebo.release();
    }
}

} // namespace OpenGeoLab::Render
