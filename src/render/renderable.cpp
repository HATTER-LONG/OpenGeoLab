/**
 * @file renderable.cpp
 * @brief RenderableBuffer and RenderBatch implementation (batched design)
 */

#include "render/renderable.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

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

namespace {
GLenum toGlPrimitive(RenderPrimitiveType type) {
    switch(type) {
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
}
} // namespace

void RenderBatch::upload(QOpenGLFunctions& gl, const DocumentRenderData& data) {
    clear();

    uploadCategory(gl, data.m_faceBatch, m_faceBuffer);
    uploadCategory(gl, data.m_edgeBatch, m_edgeBuffer);
    uploadCategory(gl, data.m_vertexBatch, m_vertexBuffer);
    uploadCategory(gl, data.m_meshElementBatch, m_meshElementBuffer);
    uploadCategory(gl, data.m_meshNodeBatch, m_meshNodeBuffer);

    // Copy entity info maps
    m_faceEntities = data.m_faceEntities;
    m_edgeEntities = data.m_edgeEntities;
    m_vertexEntities = data.m_vertexEntities;
    m_meshElementEntities = data.m_meshElementEntities;
    m_meshNodeEntities = data.m_meshNodeEntities;

    LOG_DEBUG("RenderBatch: Uploaded face={} edge={} vertex={} meshElem={} meshNode={} vertices",
              m_faceBuffer.m_vertexCount, m_edgeBuffer.m_vertexCount, m_vertexBuffer.m_vertexCount,
              m_meshElementBuffer.m_vertexCount, m_meshNodeBuffer.m_vertexCount);
}

void RenderBatch::clear() {
    m_faceBuffer.destroy();
    m_edgeBuffer.destroy();
    m_vertexBuffer.destroy();
    m_meshElementBuffer.destroy();
    m_meshNodeBuffer.destroy();

    // Re-create GPU objects for next upload
    m_faceBuffer = RenderableBuffer();
    m_edgeBuffer = RenderableBuffer();
    m_vertexBuffer = RenderableBuffer();
    m_meshElementBuffer = RenderableBuffer();
    m_meshNodeBuffer = RenderableBuffer();

    m_faceEntities.clear();
    m_edgeEntities.clear();
    m_vertexEntities.clear();
    m_meshElementEntities.clear();
    m_meshNodeEntities.clear();
}

void RenderBatch::drawAll(QOpenGLFunctions& gl, RenderableBuffer& buf, GLenum primitive_override) {
    if(!buf.isValid()) {
        return;
    }

    const GLenum primitive =
        primitive_override ? primitive_override : toGlPrimitive(buf.m_primitiveType);

    buf.m_vao->bind();
    if(buf.m_indexCount > 0) {
        buf.m_ebo->bind();
        gl.glDrawElements(primitive, buf.m_indexCount, GL_UNSIGNED_INT, nullptr);
    } else {
        gl.glDrawArrays(primitive, 0, buf.m_vertexCount);
    }
    buf.m_vao->release();
}

void RenderBatch::drawIndexRange(QOpenGLFunctions& gl,
                                 RenderableBuffer& buf,
                                 uint32_t index_offset,
                                 uint32_t index_count) {
    if(!buf.isValid() || index_count == 0) {
        return;
    }

    const GLenum primitive = toGlPrimitive(buf.m_primitiveType);

    buf.m_vao->bind();
    buf.m_ebo->bind();
    gl.glDrawElements(
        primitive, static_cast<GLsizei>(index_count), GL_UNSIGNED_INT,
        reinterpret_cast<void*>(static_cast<uintptr_t>(index_offset) * sizeof(uint32_t)));
    buf.m_vao->release();
}

void RenderBatch::drawVertexRange(QOpenGLFunctions& gl,
                                  RenderableBuffer& buf,
                                  uint32_t vertex_offset,
                                  uint32_t vertex_count,
                                  GLenum primitive) {
    if(!buf.isValid() || vertex_count == 0) {
        return;
    }

    buf.m_vao->bind();
    gl.glDrawArrays(primitive, static_cast<GLint>(vertex_offset),
                    static_cast<GLsizei>(vertex_count));
    buf.m_vao->release();
}

void RenderBatch::uploadCategory(QOpenGLFunctions& gl,
                                 const RenderMesh& mesh,
                                 RenderableBuffer& buf) {
    if(!mesh.isValid()) {
        return;
    }

    buf.m_vao->create();
    buf.m_vbo->create();
    if(mesh.isIndexed()) {
        buf.m_ebo->create();
    }

    buf.m_vao->bind();
    buf.m_vbo->bind();

    buf.m_vbo->allocate(mesh.m_vertices.data(),
                        static_cast<int>(mesh.m_vertices.size() * sizeof(RenderVertex)));

    setupVertexAttributes(gl);

    // Upload index data if present
    if(mesh.isIndexed() && buf.m_ebo->isCreated()) {
        buf.m_ebo->bind();
        buf.m_ebo->allocate(mesh.m_indices.data(),
                            static_cast<int>(mesh.m_indices.size() * sizeof(uint32_t)));
    }

    buf.m_vao->release();
    buf.m_vbo->release();
    if(buf.m_ebo->isCreated()) {
        buf.m_ebo->release();
    }

    buf.m_vertexCount = static_cast<int>(mesh.vertexCount());
    buf.m_indexCount = static_cast<int>(mesh.indexCount());
    buf.m_primitiveType = mesh.m_primitiveType;
}

void RenderBatch::setupVertexAttributes(QOpenGLFunctions& gl) {
    // Position (location 0) - 3 floats
    gl.glEnableVertexAttribArray(0);
    gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex),
                             reinterpret_cast<void*>(offsetof(RenderVertex, m_position)));

    // Normal (location 1) - 3 floats
    gl.glEnableVertexAttribArray(1);
    gl.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex),
                             reinterpret_cast<void*>(offsetof(RenderVertex, m_normal)));

    // Color (location 2) - 4 floats
    gl.glEnableVertexAttribArray(2);
    gl.glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(RenderVertex),
                             reinterpret_cast<void*>(offsetof(RenderVertex, m_color)));

    // UID low (location 3) - 1 uint32 (integer attribute - must use glVertexAttribIPointer)
    auto* extra = QOpenGLContext::currentContext()->extraFunctions();
    if(extra) {
        extra->glEnableVertexAttribArray(3);
        extra->glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(RenderVertex),
                                      reinterpret_cast<void*>(offsetof(RenderVertex, m_uidLow)));

        // UID high (location 4) - 1 uint32 (integer attribute)
        extra->glEnableVertexAttribArray(4);
        extra->glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(RenderVertex),
                                      reinterpret_cast<void*>(offsetof(RenderVertex, m_uidHigh)));
    }
}

} // namespace OpenGeoLab::Render
