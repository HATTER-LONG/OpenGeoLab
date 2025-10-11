// geometry.h - Geometry data definitions
// Separates vertex data from rendering logic for better modularity
#pragma once

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
