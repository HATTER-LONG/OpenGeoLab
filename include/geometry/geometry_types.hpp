/**
 * @file geometry_types.hpp
 * @brief Core geometry data types for CAD model representation.
 *
 * Defines hierarchical topology structures (parts -> solids -> faces -> edges -> vertices)
 * and rendering data (tessellated meshes). Suitable for visualization and mesh generation.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace OpenGeoLab {
namespace Geometry {

/**
 * @brief 3D point coordinates.
 */
struct Point3D {
    double m_x = 0.0;
    double m_y = 0.0;
    double m_z = 0.0;

    Point3D() = default;
    Point3D(double x, double y, double z) : m_x(x), m_y(y), m_z(z) {}
};

/**
 * @brief Vertex with position and normal for rendering.
 */
struct RenderVertex {
    Point3D m_position;
    Point3D m_normal;

    RenderVertex() = default;
    RenderVertex(const Point3D& pos, const Point3D& norm) : m_position(pos), m_normal(norm) {}
};

/**
 * @brief Geometric vertex entity with unique ID.
 */
struct Vertex {
    uint32_t m_id = 0;  ///< Unique vertex identifier.
    Point3D m_position; ///< 3D position.
};

/**
 * @brief Geometric edge entity.
 */
struct Edge {
    uint32_t m_id = 0;                  ///< Unique edge identifier.
    uint32_t m_startVertexId = 0;       ///< Start vertex ID.
    uint32_t m_endVertexId = 0;         ///< End vertex ID.
    std::vector<Point3D> m_curvePoints; ///< Discretized curve points for visualization.
};

/**
 * @brief Geometric face entity.
 */
struct Face {
    uint32_t m_id = 0;                        ///< Unique face identifier.
    std::vector<uint32_t> m_edgeIds;          ///< Boundary edge IDs (ordered).
    std::vector<RenderVertex> m_meshVertices; ///< Tessellated mesh vertices for rendering.
    std::vector<uint32_t> m_meshIndices;      ///< Triangle indices (3 per triangle).
};

/**
 * @brief Geometric solid/volume entity.
 */
struct Solid {
    uint32_t m_id = 0;               ///< Unique solid identifier.
    std::vector<uint32_t> m_faceIds; ///< Bounding face IDs.
};

/**
 * @brief Part/component in the model hierarchy.
 */
struct Part {
    uint32_t m_id = 0;                ///< Unique part identifier.
    std::string m_name;               ///< Part name from model file.
    std::vector<uint32_t> m_solidIds; ///< Solids contained in this part.
};

/**
 * @brief Bounding box for geometry.
 */
struct BoundingBox {
    Point3D m_min; ///< Minimum corner.
    Point3D m_max; ///< Maximum corner.

    /**
     * @brief Check if bounding box is valid (non-empty).
     */
    bool isValid() const {
        return m_min.m_x <= m_max.m_x && m_min.m_y <= m_max.m_y && m_min.m_z <= m_max.m_z;
    }

    /**
     * @brief Get center point of bounding box.
     */
    Point3D center() const {
        return Point3D((m_min.m_x + m_max.m_x) / 2, (m_min.m_y + m_max.m_y) / 2,
                       (m_min.m_z + m_max.m_z) / 2);
    }
};

} // namespace Geometry
} // namespace OpenGeoLab
