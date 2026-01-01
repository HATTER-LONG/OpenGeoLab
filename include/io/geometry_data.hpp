/**
 * @file geometry_data.hpp
 * @brief Intermediate geometry data structure for model import.
 *
 * This file provides compatibility types for IO readers.
 * New code should use the geometry module types directly.
 */
#pragma once

#include "geometry/geometry_types.hpp"

#include <memory>
#include <string>
#include <vector>

namespace OpenGeoLab {
// Forward declarations
namespace Geometry {
class GeometryStore;
} // namespace Geometry

namespace IO {

// Import geometry types for backward compatibility
using Point3D = Geometry::Point3D;
using Vertex = Geometry::RenderVertex;
using GeometryVertex = Geometry::Vertex;
using GeometryEdge = Geometry::Edge;
using GeometryFace = Geometry::Face;
using GeometrySolid = Geometry::Solid;
using ModelPart = Geometry::Part;

/**
 * @brief Complete geometry data structure from model import.
 *
 * Contains hierarchical topology (parts -> solids -> faces -> edges -> vertices)
 * and rendering data (tessellated meshes). Suitable for both visualization
 * and downstream mesh generation.
 */
struct GeometryData {
    std::vector<ModelPart> m_parts;         ///< Top-level parts/assemblies.
    std::vector<GeometrySolid> m_solids;    ///< 3D solid bodies.
    std::vector<GeometryFace> m_faces;      ///< 2D surface faces.
    std::vector<GeometryEdge> m_edges;      ///< 1D curves/edges.
    std::vector<GeometryVertex> m_vertices; ///< 0D points/vertices.

    /**
     * @brief Get summary statistics of the geometry.
     */
    std::string getSummary() const;

    /**
     * @brief Check if geometry data is empty.
     */
    bool isEmpty() const;

    /**
     * @brief Convert to GeometryModel and store in GeometryStore.
     */
    void storeToGeometryStore() const;
};

using GeometryDataPtr = std::shared_ptr<GeometryData>;

} // namespace IO
} // namespace OpenGeoLab
