/**
 * @file tessellator.hpp
 * @brief Geometry tessellation for OpenGL rendering
 *
 * Tessellator converts OCC TopoDS_Shape geometry into triangulated mesh
 * data suitable for OpenGL rendering.
 */

#pragma once

#include "geometry/geometry_entity.hpp"
#include "geometry/part_entity.hpp"
#include "geometry/render_data.hpp"

#include <TopoDS_Shape.hxx>

namespace OpenGeoLab::Geometry {

/**
 * @brief Tessellation quality settings
 */
struct TessellationParams {
    double m_linearDeflection{0.1};  ///< Linear deflection (chord height)
    double m_angularDeflection{0.5}; ///< Angular deflection in radians
    bool m_relativeTolerance{true};  ///< Use relative tolerance based on shape size

    /// Default quality for display
    [[nodiscard]] static TessellationParams defaultQuality() { return TessellationParams(); }

    /// High quality for detailed visualization
    [[nodiscard]] static TessellationParams highQuality() {
        TessellationParams params;
        params.m_linearDeflection = 0.01;
        params.m_angularDeflection = 0.1;
        return params;
    }

    /// Low quality for fast preview
    [[nodiscard]] static TessellationParams lowQuality() {
        TessellationParams params;
        params.m_linearDeflection = 0.5;
        params.m_angularDeflection = 1.0;
        return params;
    }
};

/**
 * @brief Tessellator for converting OCC shapes to render meshes
 *
 * Uses OCC's BRepMesh algorithm to triangulate faces and discretize edges.
 * Thread-safe for multiple independent tessellation operations.
 */
class Tessellator {
public:
    /**
     * @brief Tessellate an OCC shape into render data
     * @param shape Shape to tessellate
     * @param params Tessellation quality parameters
     * @return RenderData with triangulated mesh and edge data
     */
    [[nodiscard]] static RenderData
    tessellateShape(const TopoDS_Shape& shape,
                    const TessellationParams& params = TessellationParams::defaultQuality());

    /**
     * @brief Tessellate a geometry entity
     * @param entity Entity to tessellate
     * @param params Tessellation quality parameters
     * @return RenderData with entity ID set
     */
    [[nodiscard]] static RenderData
    tessellateEntity(const GeometryEntityPtr& entity,
                     const TessellationParams& params = TessellationParams::defaultQuality());

    /**
     * @brief Tessellate a part and all its faces
     * @param part Part entity to tessellate
     * @param params Tessellation quality parameters
     * @return PartRenderData with per-face and combined mesh data
     */
    [[nodiscard]] static PartRenderData
    tessellatePart(const PartEntityPtr& part,
                   const TessellationParams& params = TessellationParams::defaultQuality());

    /**
     * @brief Extract edge mesh from a shape
     * @param shape Shape to process
     * @param params Tessellation parameters for edge discretization
     * @return EdgeMesh with line segment data
     */
    [[nodiscard]] static EdgeMesh
    extractEdges(const TopoDS_Shape& shape,
                 const TessellationParams& params = TessellationParams::defaultQuality());

    /**
     * @brief Generate a unique color for a part based on its ID
     * @param part_id Part entity ID
     * @return Color4f unique to this part
     *
     * @note Uses a golden ratio hue distribution for visually distinct colors.
     */
    [[nodiscard]] static Color4f generatePartColor(EntityId part_id);

private:
    /**
     * @brief Perform shape meshing with BRepMesh
     * @param shape Shape to mesh
     * @param params Tessellation parameters
     */
    static void meshShape(const TopoDS_Shape& shape, const TessellationParams& params);

    /**
     * @brief Extract triangle data from meshed faces
     * @param shape Meshed shape
     * @return TriangleMesh with vertex and normal data
     */
    [[nodiscard]] static TriangleMesh extractTriangles(const TopoDS_Shape& shape);
};

} // namespace OpenGeoLab::Geometry
