/**
 * @file gpu_mesh.hpp
 * @brief RAII wrapper for OpenGL VAO / VBO / EBO.
 */

#pragma once

#include <opengeolab/core/visual_data.hpp>
#include <opengeolab/render/render_export.hpp>

#include <glad/gl.h>

#include <cstdint>

namespace OpenGeoLab::Render {

/// GPU buffer set for a single mesh (surface or edge).
///
/// Non-copyable, move-only.  Destructor releases GL objects.
///
/// Vertex layout for surfaces (interleaved):
///   attrib 0 (position): 3 floats, stride 24, offset 0
///   attrib 1 (normal):   3 floats, stride 24, offset 12
///
/// Vertex layout for edges:
///   attrib 0 (position): 3 floats, stride 12, offset 0
class GpuMesh {
public:
    /// Upload a SurfaceMesh (triangles with position + normal).
    static GpuMesh fromSurface(const Core::SurfaceMesh& mesh);

    /// Upload an EdgeMesh (line segments with position only).
    static GpuMesh fromEdges(const Core::EdgeMesh& mesh);

    /// Upload a PointSet (points with position only).
    static GpuMesh fromPoints(const Core::PointSet& points);

    ~GpuMesh();

    GpuMesh(GpuMesh&& other) noexcept;
    GpuMesh& operator=(GpuMesh&& other) noexcept;

    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;

    /// Draw using GL_TRIANGLES.
    void draw() const;

    /// Draw using GL_LINES.
    void drawLines() const;

    /// Draw using GL_POINTS.
    void drawPoints() const;

    [[nodiscard]] int indexCount() const;
    [[nodiscard]] bool isValid() const;

private:
    GpuMesh() = default;

    GLuint m_vao{0};
    GLuint m_vbo{0};
    GLuint m_ebo{0};
    int m_indexCount{0};
    int m_vertexCount{0};
    GLenum m_mode{GL_TRIANGLES};

    void destroy();
};

} // namespace OpenGeoLab::Render
