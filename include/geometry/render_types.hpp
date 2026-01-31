/**
 * @file render_types.hpp
 * @brief Render data structures for OpenGL visualization
 *
 * This file defines the data structures used to transfer geometry data
 * to the rendering layer. These structures contain triangulated meshes,
 * discretized edges, and vertex positions suitable for OpenGL rendering.
 */

#pragma once

#include "geometry_types.hpp"

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Geometry {

// =============================================================================
// Render Data Structures
// =============================================================================

/**
 * @brief Triangulated mesh data for rendering
 *
 * Contains vertex positions, normals, and triangle indices for
 * OpenGL rendering. Each face in the geometry generates a separate mesh.
 */
struct RenderMesh {
    std::vector<float> m_vertices;          ///< Vertex positions (x, y, z triplets)
    std::vector<float> m_normals;           ///< Vertex normals (x, y, z triplets)
    std::vector<uint32_t> m_indices;        ///< Triangle indices
    EntityId m_entityId{INVALID_ENTITY_ID}; ///< Associated geometry entity

    /// Check if mesh has valid data
    [[nodiscard]] bool isValid() const { return !m_vertices.empty() && !m_indices.empty(); }

    /// Get vertex count
    [[nodiscard]] size_t vertexCount() const { return m_vertices.size() / 3; }

    /// Get triangle count
    [[nodiscard]] size_t triangleCount() const { return m_indices.size() / 3; }
};

/**
 * @brief Edge data for wireframe rendering
 *
 * Contains discretized edge points for line rendering.
 */
struct RenderEdge {
    std::vector<float> m_points;            ///< Edge points (x, y, z triplets)
    EntityId m_entityId{INVALID_ENTITY_ID}; ///< Associated edge entity

    /// Check if edge has valid data
    [[nodiscard]] bool isValid() const { return m_points.size() >= 6; }

    /// Get point count
    [[nodiscard]] size_t pointCount() const { return m_points.size() / 3; }
};

/**
 * @brief Vertex data for point rendering
 */
struct RenderVertex {
    float m_x{0.0f};                        ///< X coordinate
    float m_y{0.0f};                        ///< Y coordinate
    float m_z{0.0f};                        ///< Z coordinate
    EntityId m_entityId{INVALID_ENTITY_ID}; ///< Associated vertex entity
};

/**
 * @brief Complete render context for a geometry document
 *
 * Contains all triangulated meshes, edges, and vertices needed to
 * render the geometry in OpenGL or similar graphics API.
 */
struct RenderContext {
    std::vector<RenderMesh> m_meshes;     ///< Face meshes for shaded rendering
    std::vector<RenderEdge> m_edges;      ///< Edges for wireframe rendering
    std::vector<RenderVertex> m_vertices; ///< Vertices for point rendering
    BoundingBox3D m_boundingBox;          ///< Overall bounding box

    /// Check if context has any renderable data
    [[nodiscard]] bool isEmpty() const {
        return m_meshes.empty() && m_edges.empty() && m_vertices.empty();
    }

    /// Clear all render data
    void clear() {
        m_meshes.clear();
        m_edges.clear();
        m_vertices.clear();
        m_boundingBox = BoundingBox3D();
    }
};

} // namespace OpenGeoLab::Geometry
