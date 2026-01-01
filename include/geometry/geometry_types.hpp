/**
 * @file geometry_types.hpp
 * @brief Core geometry data types for CAD model representation.
 *
 * Defines hierarchical topology structures (parts -> solids -> faces -> edges -> vertices)
 * and rendering data (tessellated meshes). Suitable for visualization and mesh generation.
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace OpenGeoLab {
namespace Geometry {

/**
 * @brief 3D point coordinates.
 */
struct Point3D {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Point3D() = default;
    Point3D(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
};

/**
 * @brief Vertex with position and normal for rendering.
 */
struct RenderVertex {
    Point3D position;
    Point3D normal;

    RenderVertex() = default;
    RenderVertex(const Point3D& pos, const Point3D& norm) : position(pos), normal(norm) {}
};

/**
 * @brief Geometric vertex entity with unique ID.
 */
struct Vertex {
    uint32_t id = 0;  ///< Unique vertex identifier.
    Point3D position; ///< 3D position.
};

/**
 * @brief Geometric edge entity.
 */
struct Edge {
    uint32_t id = 0;                   ///< Unique edge identifier.
    uint32_t start_vertex_id = 0;      ///< Start vertex ID.
    uint32_t end_vertex_id = 0;        ///< End vertex ID.
    std::vector<Point3D> curve_points; ///< Discretized curve points for visualization.
};

/**
 * @brief Geometric face entity.
 */
struct Face {
    uint32_t id = 0;                         ///< Unique face identifier.
    std::vector<uint32_t> edge_ids;          ///< Boundary edge IDs (ordered).
    std::vector<RenderVertex> mesh_vertices; ///< Tessellated mesh vertices for rendering.
    std::vector<uint32_t> mesh_indices;      ///< Triangle indices (3 per triangle).
};

/**
 * @brief Geometric solid/volume entity.
 */
struct Solid {
    uint32_t id = 0;                ///< Unique solid identifier.
    std::vector<uint32_t> face_ids; ///< Bounding face IDs.
};

/**
 * @brief Part/component in the model hierarchy.
 */
struct Part {
    uint32_t id = 0;                 ///< Unique part identifier.
    std::string name;                ///< Part name from model file.
    std::vector<uint32_t> solid_ids; ///< Solids contained in this part.
};

/**
 * @brief Bounding box for geometry.
 */
struct BoundingBox {
    Point3D min; ///< Minimum corner.
    Point3D max; ///< Maximum corner.

    /**
     * @brief Check if bounding box is valid (non-empty).
     */
    bool isValid() const { return min.x <= max.x && min.y <= max.y && min.z <= max.z; }

    /**
     * @brief Get center point of bounding box.
     */
    Point3D center() const {
        return Point3D((min.x + max.x) / 2, (min.y + max.y) / 2, (min.z + max.z) / 2);
    }
};

} // namespace Geometry
} // namespace OpenGeoLab
