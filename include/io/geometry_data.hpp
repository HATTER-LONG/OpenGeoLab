/**
 * @file geometry_data.hpp
 * @brief Intermediate geometry data structure for model import
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace OpenGeoLab {
namespace IO {

/**
 * @brief 3D point coordinates
 */
struct Point3D {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Point3D() = default;
    Point3D(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
};

/**
 * @brief Vertex with position and normal for rendering
 */
struct Vertex {
    Point3D position;
    Point3D normal;

    Vertex() = default;
    Vertex(const Point3D& pos, const Point3D& norm) : position(pos), normal(norm) {}
};

/**
 * @brief Geometric vertex entity with unique ID
 */
struct GeometryVertex {
    uint32_t id = 0;  ///< Unique vertex identifier
    Point3D position; ///< 3D position
};

/**
 * @brief Geometric edge entity
 */
struct GeometryEdge {
    uint32_t id = 0;                   ///< Unique edge identifier
    uint32_t start_vertex_id = 0;      ///< Start vertex ID
    uint32_t end_vertex_id = 0;        ///< End vertex ID
    std::vector<Point3D> curve_points; ///< Discretized curve points for visualization
};

/**
 * @brief Geometric face entity
 */
struct GeometryFace {
    uint32_t id = 0;                    ///< Unique face identifier
    std::vector<uint32_t> edge_ids;     ///< Boundary edge IDs (ordered)
    std::vector<Vertex> mesh_vertices;  ///< Tessellated mesh vertices for rendering
    std::vector<uint32_t> mesh_indices; ///< Triangle indices (3 per triangle)
};

/**
 * @brief Geometric solid/volume entity
 */
struct GeometrySolid {
    uint32_t id = 0;                ///< Unique solid identifier
    std::vector<uint32_t> face_ids; ///< Bounding face IDs
};

/**
 * @brief Part/component in the model hierarchy
 */
struct ModelPart {
    uint32_t id = 0;                 ///< Unique part identifier
    std::string name;                ///< Part name from model file
    std::vector<uint32_t> solid_ids; ///< Solids contained in this part
};

/**
 * @brief Complete geometry data structure from model import
 *
 * Contains hierarchical topology (parts -> solids -> faces -> edges -> vertices)
 * and rendering data (tessellated meshes). Suitable for both visualization
 * and downstream mesh generation.
 */
struct GeometryData {
    std::vector<ModelPart> parts;         ///< Top-level parts/assemblies
    std::vector<GeometrySolid> solids;    ///< 3D solid bodies
    std::vector<GeometryFace> faces;      ///< 2D surface faces
    std::vector<GeometryEdge> edges;      ///< 1D curves/edges
    std::vector<GeometryVertex> vertices; ///< 0D points/vertices

    /**
     * @brief Get summary statistics of the geometry
     * @return Human-readable summary string
     */
    std::string getSummary() const;

    /**
     * @brief Check if geometry data is empty
     * @return true if no geometric entities exist
     */
    bool isEmpty() const;
};

using GeometryDataPtr = std::shared_ptr<GeometryData>;

} // namespace IO
} // namespace OpenGeoLab
