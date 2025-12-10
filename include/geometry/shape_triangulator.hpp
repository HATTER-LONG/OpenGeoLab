/**
 * @file shape_triangulator.hpp
 * @brief Utility for converting OCC TopoDS_Shape to renderable geometry data
 *
 * Provides functionality to triangulate Open CASCADE shapes and convert them
 * to the internal GeometryData format suitable for OpenGL rendering.
 */

#pragma once

#include <geometry/geometry.hpp>

#include <TopoDS_Shape.hxx>

#include <memory>
#include <string>

namespace OpenGeoLab {
namespace Geometry {

/**
 * @brief Parameters for mesh triangulation
 */
struct TriangulationParams {
    double linearDeflection = 0.1;  ///< Maximum distance between mesh and surface
    double angularDeflection = 0.5; ///< Maximum angle between facet normals (radians)
    bool relative = false;          ///< If true, deflection is relative to model size
    float colorR = 0.7f;            ///< Default color R component
    float colorG = 0.7f;            ///< Default color G component
    float colorB = 0.7f;            ///< Default color B component
};

/**
 * @brief Triangulates OCC shapes and converts to GeometryData
 *
 * This class provides methods to convert TopoDS_Shape objects to triangulated
 * mesh data that can be rendered using OpenGL. It handles mesh generation,
 * normal calculation, and vertex deduplication.
 */
class ShapeTriangulator {
public:
    ShapeTriangulator() = default;

    /**
     * @brief Triangulate a shape and convert to GeometryData
     * @param shape The TopoDS_Shape to triangulate
     * @param params Triangulation parameters
     * @return Shared pointer to geometry data, or nullptr on failure
     */
    std::shared_ptr<GeometryData> triangulate(const TopoDS_Shape& shape,
                                              const TriangulationParams& params = {});

    /**
     * @brief Get the last error message
     * @return Error message string, empty if no error
     */
    std::string getLastError() const { return m_lastError; }

private:
    /**
     * @brief Perform mesh triangulation on the shape
     * @param shape The shape to triangulate
     * @param params Triangulation parameters
     * @return true on success, false on failure
     */
    bool performTriangulation(TopoDS_Shape& shape, const TriangulationParams& params);

    /**
     * @brief Extract triangle data from a triangulated shape
     * @param shape The triangulated shape
     * @param params Triangulation parameters (for color)
     * @param vertex_data Output buffer for vertex data
     * @param index_data Output buffer for triangle indices
     * @return true on success, false on failure
     */
    bool extractTriangleData(const TopoDS_Shape& shape,
                             const TriangulationParams& params,
                             std::vector<float>& vertex_data,
                             std::vector<unsigned int>& index_data);

    std::string m_lastError;
};

} // namespace Geometry
} // namespace OpenGeoLab
