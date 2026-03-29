#include <opengeolab/render/gpu_mesh.hpp>

#include <cstring>
#include <utility>
#include <vector>

namespace OpenGeoLab::Render {

GpuMesh GpuMesh::fromSurface(const Core::SurfaceMesh& mesh) {
    if(mesh.positions.empty() || mesh.indices.empty()) {
        return {};
    }

    // Interleave position + normal: [px, py, pz, nx, ny, nz] per vertex
    size_t vertex_count = mesh.positions.size() / 3;
    std::vector<float> interleaved(vertex_count * 6);
    for(size_t i = 0; i < vertex_count; ++i) {
        interleaved[i * 6 + 0] = mesh.positions[i * 3 + 0];
        interleaved[i * 6 + 1] = mesh.positions[i * 3 + 1];
        interleaved[i * 6 + 2] = mesh.positions[i * 3 + 2];

        if(i * 3 + 2 < mesh.normals.size()) {
            interleaved[i * 6 + 3] = mesh.normals[i * 3 + 0];
            interleaved[i * 6 + 4] = mesh.normals[i * 3 + 1];
            interleaved[i * 6 + 5] = mesh.normals[i * 3 + 2];
        }
    }

    GpuMesh gpu;
    gpu.m_mode = GL_TRIANGLES;
    gpu.m_indexCount = static_cast<int>(mesh.indices.size());

    glGenVertexArrays(1, &gpu.m_vao);
    glGenBuffers(1, &gpu.m_vbo);
    glGenBuffers(1, &gpu.m_ebo);

    glBindVertexArray(gpu.m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, gpu.m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(interleaved.size() * sizeof(float)),
                 interleaved.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(uint32_t)),
                 mesh.indices.data(), GL_STATIC_DRAW);

    // attrib 0: position (3 floats, stride 24, offset 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), static_cast<void*>(nullptr));

    // attrib 1: normal (3 floats, stride 24, offset 12)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));

    glBindVertexArray(0);
    return gpu;
}

GpuMesh GpuMesh::fromEdges(const Core::EdgeMesh& mesh) {
    if(mesh.positions.empty() || mesh.indices.empty()) {
        return {};
    }

    GpuMesh gpu;
    gpu.m_mode = GL_LINES;
    gpu.m_indexCount = static_cast<int>(mesh.indices.size());

    glGenVertexArrays(1, &gpu.m_vao);
    glGenBuffers(1, &gpu.m_vbo);
    glGenBuffers(1, &gpu.m_ebo);

    glBindVertexArray(gpu.m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, gpu.m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.positions.size() * sizeof(float)),
                 mesh.positions.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(uint32_t)),
                 mesh.indices.data(), GL_STATIC_DRAW);

    // attrib 0: position (3 floats, stride 12, offset 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(nullptr));

    glBindVertexArray(0);
    return gpu;
}

GpuMesh GpuMesh::fromPoints(const Core::PointSet& points) {
    if(points.positions.empty()) {
        return {};
    }

    GpuMesh gpu;
    gpu.m_mode = GL_POINTS;
    gpu.m_vertexCount = static_cast<int>(points.positions.size() / 3);

    glGenVertexArrays(1, &gpu.m_vao);
    glGenBuffers(1, &gpu.m_vbo);

    glBindVertexArray(gpu.m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, gpu.m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(points.positions.size() * sizeof(float)),
                 points.positions.data(), GL_STATIC_DRAW);

    // attrib 0: position (3 floats, stride 12, offset 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(nullptr));

    glBindVertexArray(0);
    return gpu;
}

GpuMesh::~GpuMesh() { destroy(); }

GpuMesh::GpuMesh(GpuMesh&& other) noexcept
    : m_vao(other.m_vao), m_vbo(other.m_vbo), m_ebo(other.m_ebo), m_indexCount(other.m_indexCount),
      m_vertexCount(other.m_vertexCount), m_mode(other.m_mode) {
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_ebo = 0;
    other.m_indexCount = 0;
    other.m_vertexCount = 0;
}

GpuMesh& GpuMesh::operator=(GpuMesh&& other) noexcept {
    if(this != &other) {
        destroy();
        m_vao = std::exchange(other.m_vao, 0);
        m_vbo = std::exchange(other.m_vbo, 0);
        m_ebo = std::exchange(other.m_ebo, 0);
        m_indexCount = std::exchange(other.m_indexCount, 0);
        m_vertexCount = std::exchange(other.m_vertexCount, 0);
        m_mode = other.m_mode;
    }
    return *this;
}

void GpuMesh::draw() const {
    if(m_vao == 0) {
        return;
    }
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void GpuMesh::drawLines() const {
    if(m_vao == 0) {
        return;
    }
    glBindVertexArray(m_vao);
    glDrawElements(GL_LINES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void GpuMesh::drawPoints() const {
    if(m_vao == 0 || m_vertexCount == 0) {
        return;
    }
    glBindVertexArray(m_vao);
    glDrawArrays(GL_POINTS, 0, m_vertexCount);
    glBindVertexArray(0);
}

int GpuMesh::indexCount() const { return m_indexCount; }

bool GpuMesh::isValid() const { return m_vao != 0; }

void GpuMesh::destroy() {
    if(m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    if(m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if(m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    m_indexCount = 0;
    m_vertexCount = 0;
}

} // namespace OpenGeoLab::Render
