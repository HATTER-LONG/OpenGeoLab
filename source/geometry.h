// geometry.h - Geometry data definitions
// Separates vertex data from rendering logic for better modularity
#pragma once

#include <cmath>
#include <vector>

/**
 * @brief Base class for geometry data
 *
 * This abstract class defines the interface for geometry data,
 * allowing different geometric shapes to provide their vertex
 * and index data in a uniform way.
 */
struct GeometryData {
    virtual ~GeometryData() = default;

    /**
     * @brief Get vertex data pointer
     * @return Pointer to vertex array (format depends on implementation)
     */
    virtual const float* vertices() const = 0;

    /**
     * @brief Get number of vertices
     * @return Total vertex count
     */
    virtual int vertexCount() const = 0;

    /**
     * @brief Get index data pointer (optional)
     * @return Pointer to index array, or nullptr if not using indexed drawing
     */
    virtual const unsigned int* indices() const { return nullptr; }

    /**
     * @brief Get number of indices (optional)
     * @return Total index count, or 0 if not using indexed drawing
     */
    virtual int indexCount() const { return 0; }
};

/**
 * @brief Cube geometry data with lighting support
 *
 * Provides vertex data for a unit cube centered at origin.
 * Each vertex contains: position (3 floats), normal (3 floats), color (3 floats)
 * Total: 9 floats per vertex
 */
class CubeData : public GeometryData {
public:
    CubeData() {
        // Each vertex contains: position(x,y,z) + normal(nx,ny,nz) + color(r,g,b)
        // 8 vertices for cube, each face has its own normal vector
        // clang-format off
        m_vertices = {
            // Front face (z = 0.5) - Normal (0, 0, 1)
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f,  // Bottom-left
             0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f,  // Bottom-right
             0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,  // Top-right
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 0.0f,  // Top-left

            // Back face (z = -0.5) - Normal (0, 0, -1)
             0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.5f, 0.5f, 0.5f,

            // Top face (y = 0.5) - Normal (0, 1, 0)
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.5f, 0.0f,
             0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  0.5f, 1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.5f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f, 0.5f,

            // Bottom face (y = -0.5) - Normal (0, -1, 0)
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  0.5f, 0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.5f, 0.5f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f, 0.5f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  0.5f, 1.0f, 0.5f,

            // Right face (x = 0.5) - Normal (1, 0, 0)
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.5f, 0.5f,
             0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.5f, 1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 0.5f,
             0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.5f, 0.5f, 1.0f,

            // Left face (x = -0.5) - Normal (-1, 0, 0)
            -0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,  0.8f, 0.8f, 0.8f,
            -0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,  0.6f, 0.6f, 0.6f,
            -0.5f,  0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,  0.9f, 0.9f, 0.9f,
        };

        // Index data - two triangles per face
        m_indices = {
            0,  1,  2,   0,  2,  3,   // Front face
            4,  5,  6,   4,  6,  7,   // Back face
            8,  9,  10,  8,  10, 11,  // Top face
            12, 13, 14,  12, 14, 15,  // Bottom face
            16, 17, 18,  16, 18, 19,  // Right face
            20, 21, 22,  20, 22, 23   // Left face
        };
        // clang-format on
    }

    const float* vertices() const override { return m_vertices.data(); }
    int vertexCount() const override { return 24; } // 6 faces * 4 vertices

    const unsigned int* indices() const override { return m_indices.data(); }
    int indexCount() const override { return 36; } // 6 faces * 2 triangles * 3 vertices

private:
    std::vector<float> m_vertices;
    std::vector<unsigned int> m_indices;
};

/**
 * @brief Cylinder geometry data with lighting support
 *
 * Provides vertex data for a cylinder centered at origin.
 * Each vertex contains: position (3 floats), normal (3 floats), color (3 floats)
 * Total: 9 floats per vertex
 */
class CylinderData : public GeometryData {
public:
    explicit CylinderData(int segments = 32, float radius = 0.5f, float height = 1.0f) {
        // Generate cylinder geometry
        const float half_height = height * 0.5f;
        const float angle_step = 2.0f * 3.14159265f / segments;

        // Reserve space for vertices and indices
        m_vertices.reserve((segments * 2 + 2) * 9); // Side + top + bottom circles
        m_indices.reserve(segments * 12);           // Side faces + top + bottom caps

        // Generate side vertices
        for(int i = 0; i <= segments; ++i) {
            float angle = i * angle_step;
            float x = radius * std::cos(angle);
            float z = radius * std::sin(angle);
            float nx = std::cos(angle);
            float nz = std::sin(angle);

            // Color gradient based on angle
            float r = 0.5f + 0.5f * std::cos(angle);
            float g = 0.5f + 0.5f * std::sin(angle);
            float b = 0.5f + 0.5f * std::cos(angle + 1.0f);

            // Bottom vertex
            m_vertices.insert(m_vertices.end(), {x, -half_height, z, nx, 0.0f, nz, r, g, b});

            // Top vertex
            m_vertices.insert(m_vertices.end(), {x, half_height, z, nx, 0.0f, nz, r, g, b});
        }

        // Generate side face indices
        for(int i = 0; i < segments; ++i) {
            int bottom_left = i * 2;
            int top_left = i * 2 + 1;
            int bottom_right = (i + 1) * 2;
            int top_right = (i + 1) * 2 + 1;

            // First triangle
            m_indices.insert(m_indices.end(), {static_cast<unsigned int>(bottom_left),
                                               static_cast<unsigned int>(bottom_right),
                                               static_cast<unsigned int>(top_left)});

            // Second triangle
            m_indices.insert(m_indices.end(), {static_cast<unsigned int>(top_left),
                                               static_cast<unsigned int>(bottom_right),
                                               static_cast<unsigned int>(top_right)});
        }

        // Add center vertices for caps
        int bottom_center_idx = m_vertices.size() / 9;
        m_vertices.insert(m_vertices.end(),
                          {0.0f, -half_height, 0.0f, 0.0f, -1.0f, 0.0f, 0.8f, 0.8f, 0.8f});

        int top_center_idx = m_vertices.size() / 9;
        m_vertices.insert(m_vertices.end(),
                          {0.0f, half_height, 0.0f, 0.0f, 1.0f, 0.0f, 0.9f, 0.9f, 0.9f});

        // Generate bottom cap vertices and indices
        for(int i = 0; i < segments; ++i) {
            float angle = i * angle_step;
            float x = radius * std::cos(angle);
            float z = radius * std::sin(angle);

            int vertex_idx = m_vertices.size() / 9;
            m_vertices.insert(m_vertices.end(),
                              {x, -half_height, z, 0.0f, -1.0f, 0.0f, 0.7f, 0.7f, 0.8f});

            int next_idx = (i + 1) < segments ? vertex_idx + 1 : bottom_center_idx + 2;
            m_indices.insert(m_indices.end(), {static_cast<unsigned int>(bottom_center_idx),
                                               static_cast<unsigned int>(next_idx),
                                               static_cast<unsigned int>(vertex_idx)});
        }

        // Generate top cap vertices and indices
        for(int i = 0; i < segments; ++i) {
            float angle = i * angle_step;
            float x = radius * std::cos(angle);
            float z = radius * std::sin(angle);

            int vertex_idx = m_vertices.size() / 9;
            m_vertices.insert(m_vertices.end(),
                              {x, half_height, z, 0.0f, 1.0f, 0.0f, 0.8f, 0.7f, 0.7f});

            int next_idx = (i + 1) < segments ? vertex_idx + 1 : top_center_idx + segments + 2;
            m_indices.insert(m_indices.end(), {static_cast<unsigned int>(top_center_idx),
                                               static_cast<unsigned int>(vertex_idx),
                                               static_cast<unsigned int>(next_idx)});
        }
    }

    const float* vertices() const override { return m_vertices.data(); }
    int vertexCount() const override { return m_vertices.size() / 9; }

    const unsigned int* indices() const override { return m_indices.data(); }
    int indexCount() const override { return m_indices.size(); }

private:
    std::vector<float> m_vertices;
    std::vector<unsigned int> m_indices;
};
