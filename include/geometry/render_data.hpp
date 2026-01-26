/**
 * @file render_data.hpp
 * @brief Render data structures for OpenGL visualization
 *
 * This file defines the data structures used to transfer geometry
 * from the OCC layer to the OpenGL rendering layer.
 */

#pragma once

#include "geometry/geometry_types.hpp"

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief RGBA color representation for rendering
 */
struct Color4f {
    float m_r{0.8f}; ///< Red component [0, 1]
    float m_g{0.8f}; ///< Green component [0, 1]
    float m_b{0.8f}; ///< Blue component [0, 1]
    float m_a{1.0f}; ///< Alpha component [0, 1]

    /// Default gray color
    Color4f() = default;

    /**
     * @brief Construct color from RGBA values
     * @param r Red [0, 1]
     * @param g Green [0, 1]
     * @param b Blue [0, 1]
     * @param a Alpha [0, 1]
     */
    Color4f(float r, float g, float b, float a = 1.0f) : m_r(r), m_g(g), m_b(b), m_a(a) {}

    /// Predefined colors
    [[nodiscard]] static Color4f red() { return Color4f(1.0f, 0.0f, 0.0f, 1.0f); }
    [[nodiscard]] static Color4f green() { return Color4f(0.0f, 1.0f, 0.0f, 1.0f); }
    [[nodiscard]] static Color4f blue() { return Color4f(0.0f, 0.0f, 1.0f, 1.0f); }
    [[nodiscard]] static Color4f white() { return Color4f(1.0f, 1.0f, 1.0f, 1.0f); }
    [[nodiscard]] static Color4f black() { return Color4f(0.0f, 0.0f, 0.0f, 1.0f); }
    [[nodiscard]] static Color4f gray() { return Color4f(0.5f, 0.5f, 0.5f, 1.0f); }
    [[nodiscard]] static Color4f yellow() { return Color4f(1.0f, 1.0f, 0.0f, 1.0f); }
    [[nodiscard]] static Color4f cyan() { return Color4f(0.0f, 1.0f, 1.0f, 1.0f); }
    [[nodiscard]] static Color4f magenta() { return Color4f(1.0f, 0.0f, 1.0f, 1.0f); }
    [[nodiscard]] static Color4f orange() { return Color4f(1.0f, 0.5f, 0.0f, 1.0f); }
};

/**
 * @brief Triangulated mesh data for OpenGL rendering
 *
 * Contains vertex positions, normals, and triangle indices suitable
 * for rendering with GL_TRIANGLES. Also includes edge data for
 * wireframe rendering.
 */
struct TriangleMesh {
    std::vector<float> m_vertices;   ///< Vertex positions (x, y, z) interleaved
    std::vector<float> m_normals;    ///< Vertex normals (nx, ny, nz) interleaved
    std::vector<uint32_t> m_indices; ///< Triangle indices (3 per triangle)

    /// Clear all mesh data
    void clear() {
        m_vertices.clear();
        m_normals.clear();
        m_indices.clear();
    }

    /// Check if mesh is empty
    [[nodiscard]] bool isEmpty() const { return m_vertices.empty(); }

    /// Get vertex count
    [[nodiscard]] size_t vertexCount() const { return m_vertices.size() / 3; }

    /// Get triangle count
    [[nodiscard]] size_t triangleCount() const { return m_indices.size() / 3; }

    /// Merge another mesh into this one
    void merge(const TriangleMesh& other) {
        if(other.isEmpty()) {
            return;
        }

        uint32_t vertex_offset = static_cast<uint32_t>(vertexCount());

        m_vertices.insert(m_vertices.end(), other.m_vertices.begin(), other.m_vertices.end());
        m_normals.insert(m_normals.end(), other.m_normals.begin(), other.m_normals.end());

        for(uint32_t idx : other.m_indices) {
            m_indices.push_back(idx + vertex_offset);
        }
    }
};

/**
 * @brief Edge/wireframe data for OpenGL rendering
 *
 * Contains line segment data suitable for rendering with GL_LINES.
 */
struct EdgeMesh {
    std::vector<float> m_vertices;   ///< Vertex positions (x, y, z) interleaved
    std::vector<uint32_t> m_indices; ///< Line indices (2 per segment)

    /// Clear all edge data
    void clear() {
        m_vertices.clear();
        m_indices.clear();
    }

    /// Check if mesh is empty
    [[nodiscard]] bool isEmpty() const { return m_vertices.empty(); }

    /// Get vertex count
    [[nodiscard]] size_t vertexCount() const { return m_vertices.size() / 3; }

    /// Get line segment count
    [[nodiscard]] size_t lineCount() const { return m_indices.size() / 2; }

    /// Merge another edge mesh into this one
    void merge(const EdgeMesh& other) {
        if(other.isEmpty()) {
            return;
        }

        uint32_t vertex_offset = static_cast<uint32_t>(vertexCount());

        m_vertices.insert(m_vertices.end(), other.m_vertices.begin(), other.m_vertices.end());

        for(uint32_t idx : other.m_indices) {
            m_indices.push_back(idx + vertex_offset);
        }
    }
};

/**
 * @brief Complete render data for a geometry entity
 *
 * Contains both surface mesh (triangles) and edge mesh (wireframe),
 * plus color information for rendering.
 */
struct RenderData {
    EntityId m_entityId{INVALID_ENTITY_ID}; ///< Source entity ID
    TriangleMesh m_triangleMesh;            ///< Surface mesh data
    EdgeMesh m_edgeMesh;                    ///< Edge/wireframe data
    Color4f m_faceColor;                    ///< Face/surface color
    Color4f m_edgeColor{0.0f, 0.0f, 0.0f};  ///< Edge color (default black)
    bool m_visible{true};                   ///< Visibility flag

    /// Clear all render data
    void clear() {
        m_entityId = INVALID_ENTITY_ID;
        m_triangleMesh.clear();
        m_edgeMesh.clear();
        m_faceColor = Color4f();
        m_edgeColor = Color4f(0.0f, 0.0f, 0.0f);
        m_visible = true;
    }

    /// Check if render data is empty
    [[nodiscard]] bool isEmpty() const { return m_triangleMesh.isEmpty() && m_edgeMesh.isEmpty(); }
};

/**
 * @brief Collection of render data for a part
 *
 * Contains render data for all faces in a part, with a unique color
 * assigned to the part.
 */
struct PartRenderData {
    EntityId m_partId{INVALID_ENTITY_ID}; ///< Part entity ID
    std::string m_partName;               ///< Part display name
    Color4f m_partColor;                  ///< Part color for rendering
    std::vector<RenderData> m_faceData;   ///< Render data per face
    RenderData m_combinedData;            ///< Combined mesh for entire part
    BoundingBox3D m_boundingBox;          ///< Part bounding box

    /// Clear all part render data
    void clear() {
        m_partId = INVALID_ENTITY_ID;
        m_partName.clear();
        m_partColor = Color4f();
        m_faceData.clear();
        m_combinedData.clear();
        m_boundingBox = BoundingBox3D();
    }
};

} // namespace OpenGeoLab::Geometry
