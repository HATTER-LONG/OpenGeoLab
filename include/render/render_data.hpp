/**
 * @file render_data.hpp
 * @brief Render data structures for OpenGL visualization
 *
 * Defines the data structures used to transfer geometry information
 * from the Geometry layer to the Render layer for OpenGL rendering.
 */

#pragma once

#include "geometry/geometry_types.hpp"

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Vertex data for rendering
 *
 * Contains position, normal, and optional color information for a vertex.
 */
struct RenderVertex {
    float m_position[3]{0.0f, 0.0f, 0.0f};    ///< Vertex position (x, y, z)
    float m_normal[3]{0.0f, 0.0f, 1.0f};      ///< Vertex normal (nx, ny, nz)
    float m_color[4]{0.8f, 0.8f, 0.8f, 1.0f}; ///< Vertex color (r, g, b, a)

    RenderVertex() = default;

    /**
     * @brief Construct a vertex with position
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     */
    RenderVertex(float x, float y, float z) {
        m_position[0] = x;
        m_position[1] = y;
        m_position[2] = z;
    }

    /**
     * @brief Construct a vertex with position and normal
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param nx Normal X component
     * @param ny Normal Y component
     * @param nz Normal Z component
     */
    RenderVertex(float x, float y, float z, float nx, float ny, float nz) {
        m_position[0] = x;
        m_position[1] = y;
        m_position[2] = z;
        m_normal[0] = nx;
        m_normal[1] = ny;
        m_normal[2] = nz;
    }

    /**
     * @brief Set vertex color
     * @param r Red component [0, 1]
     * @param g Green component [0, 1]
     * @param b Blue component [0, 1]
     * @param a Alpha component [0, 1]
     */
    void setColor(float r, float g, float b, float a = 1.0f) {
        m_color[0] = r;
        m_color[1] = g;
        m_color[2] = b;
        m_color[3] = a;
    }
};

/**
 * @brief Render primitive type
 */
enum class RenderPrimitiveType : uint8_t {
    Points = 0,   ///< Render as points
    Lines = 1,    ///< Render as lines
    Triangles = 2 ///< Render as triangles
};

/**
 * @brief Mesh data for rendering a single geometry entity
 *
 * Contains vertices and indices for OpenGL rendering.
 */
struct RenderMesh {
    std::vector<RenderVertex> m_vertices; ///< Vertex data
    std::vector<uint32_t> m_indices;      ///< Index data for indexed drawing
    RenderPrimitiveType m_primitiveType{RenderPrimitiveType::Triangles}; ///< Primitive type
    Geometry::EntityId m_entityId{Geometry::INVALID_ENTITY_ID};          ///< Associated entity ID

    /**
     * @brief Check if mesh has valid data
     * @return true if mesh contains vertices
     */
    [[nodiscard]] bool isValid() const { return !m_vertices.empty(); }

    /**
     * @brief Clear all mesh data
     */
    void clear() {
        m_vertices.clear();
        m_indices.clear();
    }

    /**
     * @brief Get the bounding box of the mesh
     * @return Computed bounding box
     */
    [[nodiscard]] Geometry::BoundingBox3D boundingBox() const {
        Geometry::BoundingBox3D bbox;
        for(const auto& vertex : m_vertices) {
            bbox.expand(Geometry::Point3D(static_cast<double>(vertex.m_position[0]),
                                          static_cast<double>(vertex.m_position[1]),
                                          static_cast<double>(vertex.m_position[2])));
        }
        return bbox;
    }
};

/**
 * @brief Complete render data for a scene
 *
 * Contains all meshes and scene-level information for rendering.
 */
struct RenderScene {
    std::vector<RenderMesh> m_meshes;      ///< All meshes in the scene
    Geometry::BoundingBox3D m_boundingBox; ///< Scene bounding box

    /**
     * @brief Check if scene has any render data
     * @return true if scene contains meshes
     */
    [[nodiscard]] bool isEmpty() const { return m_meshes.empty(); }

    /**
     * @brief Clear all scene data
     */
    void clear() {
        m_meshes.clear();
        m_boundingBox = Geometry::BoundingBox3D();
    }

    /**
     * @brief Recompute scene bounding box from all meshes
     */
    void updateBoundingBox() {
        m_boundingBox = Geometry::BoundingBox3D();
        for(const auto& mesh : m_meshes) {
            m_boundingBox.expand(mesh.boundingBox());
        }
    }
};

} // namespace OpenGeoLab::Render
