/**
 * @file vertex_array_object.cpp
 * @brief RAII VAO implementation using GL 4.5 Direct State Access.
 */
#include <opengeolab/render/vertex_array_object.hpp>

#include <glad/gl.h>

#include <utility>

namespace OpenGeoLab::Render {

VertexArrayObject::~VertexArrayObject() { release(); }

VertexArrayObject::VertexArrayObject(VertexArrayObject&& other) noexcept
    : vao_(std::exchange(other.vao_, 0U)), positionVbo_(std::exchange(other.positionVbo_, 0U)),
      normalVbo_(std::exchange(other.normalVbo_, 0U)), ebo_(std::exchange(other.ebo_, 0U)),
      indexCount_(std::exchange(other.indexCount_, 0)),
      drawMode_(std::exchange(other.drawMode_, 0U)) {}

VertexArrayObject& VertexArrayObject::operator=(VertexArrayObject&& other) noexcept {
    if(this != &other) {
        release();
        vao_ = std::exchange(other.vao_, 0U);
        positionVbo_ = std::exchange(other.positionVbo_, 0U);
        normalVbo_ = std::exchange(other.normalVbo_, 0U);
        ebo_ = std::exchange(other.ebo_, 0U);
        indexCount_ = std::exchange(other.indexCount_, 0);
        drawMode_ = std::exchange(other.drawMode_, 0U);
    }
    return *this;
}

void VertexArrayObject::upload(const Scene::RenderMeshData& mesh) {
    release();

    switch(mesh.topology) {
    case Scene::PrimitiveType::Triangles:
        drawMode_ = GL_TRIANGLES;
        break;
    case Scene::PrimitiveType::Lines:
        drawMode_ = GL_LINES;
        break;
    case Scene::PrimitiveType::Points:
        drawMode_ = GL_POINTS;
        break;
    }

    indexCount_ = static_cast<std::int32_t>(mesh.indices.size());

    glCreateVertexArrays(1, &vao_);

    glCreateBuffers(1, &positionVbo_);
    glNamedBufferStorage(positionVbo_,
                         static_cast<GLsizeiptr>(mesh.positions.size() * sizeof(float)),
                         mesh.positions.data(), 0);
    glVertexArrayVertexBuffer(vao_, 0, positionVbo_, 0, 3 * static_cast<GLsizei>(sizeof(float)));
    glEnableVertexArrayAttrib(vao_, 0);
    glVertexArrayAttribFormat(vao_, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao_, 0, 0);

    if(!mesh.normals.empty()) {
        glCreateBuffers(1, &normalVbo_);
        glNamedBufferStorage(normalVbo_,
                             static_cast<GLsizeiptr>(mesh.normals.size() * sizeof(float)),
                             mesh.normals.data(), 0);
        glVertexArrayVertexBuffer(vao_, 1, normalVbo_, 0, 3 * static_cast<GLsizei>(sizeof(float)));
        glEnableVertexArrayAttrib(vao_, 1);
        glVertexArrayAttribFormat(vao_, 1, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao_, 1, 1);
    }

    glCreateBuffers(1, &ebo_);
    glNamedBufferStorage(ebo_, static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(std::uint32_t)),
                         mesh.indices.data(), 0);
    glVertexArrayElementBuffer(vao_, ebo_);
}

void VertexArrayObject::draw() const {
    if(!isValid()) {
        return;
    }

    glBindVertexArray(vao_);
    glDrawElements(drawMode_, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void VertexArrayObject::release() {
    if(ebo_ != 0U) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0U;
    }
    if(normalVbo_ != 0U) {
        glDeleteBuffers(1, &normalVbo_);
        normalVbo_ = 0U;
    }
    if(positionVbo_ != 0U) {
        glDeleteBuffers(1, &positionVbo_);
        positionVbo_ = 0U;
    }
    if(vao_ != 0U) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0U;
    }

    indexCount_ = 0;
    drawMode_ = 0U;
}

bool VertexArrayObject::isValid() const { return vao_ != 0U; }

} // namespace OpenGeoLab::Render
