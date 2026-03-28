/**
 * @file visual_data.hpp
 * @brief Render-ready mesh data produced by tessellation
 *
 * VisualData is the geometry→render contract: it holds triangle meshes,
 * edge polylines, and point sets in plain float/uint32 arrays that the
 * render module can upload to GPU buffers without touching OCC types.
 */

#pragma once

#include <opengeolab/core/core_export.hpp>

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Core {

/**
 * @brief Triangle mesh for one surface (typically one OCC face).
 *
 * Positions and normals are interleaved per-vertex as flat float arrays
 * (x, y, z per vertex).  Indices are 0-based triangle indices.
 */
struct SurfaceMesh {
    std::vector<float> positions;                  ///< xyz per vertex
    std::vector<float> normals;                    ///< xyz per vertex
    std::vector<uint32_t> indices;                 ///< triangle indices (3 per face)
    std::vector<float> colors;                     ///< rgba per vertex (optional)
    float defaultColor[4]{0.7f, 0.7f, 0.7f, 1.0f}; ///< fallback RGBA
};

/**
 * @brief Polyline for one edge.
 *
 * Positions are flat xyz.  Indices define line segments (2 per segment).
 */
struct EdgeMesh {
    std::vector<float> positions;           ///< xyz per vertex
    std::vector<uint32_t> indices;          ///< line-segment indices (2 per seg)
    float color[4]{0.0f, 0.0f, 0.0f, 1.0f}; ///< RGBA line color
};

/**
 * @brief Point cloud for topological vertices.
 */
struct PointSet {
    std::vector<float> positions;           ///< xyz per point
    float pointSize{5.0f};                  ///< render point size
    float color[4]{1.0f, 0.0f, 0.0f, 1.0f}; ///< RGBA point color
};

/**
 * @brief Visual presentation style.
 */
enum class RenderStyle : uint8_t { Solid, Wireframe, SolidWithEdges, Transparent };

/**
 * @brief Complete render-ready representation of a shape.
 */
struct VisualData {
    std::vector<SurfaceMesh> surfaces; ///< one per face
    std::vector<EdgeMesh> edges;       ///< one per edge
    std::vector<PointSet> points;      ///< one set of vertices
    RenderStyle style{RenderStyle::SolidWithEdges};
};

} // namespace OpenGeoLab::Core
