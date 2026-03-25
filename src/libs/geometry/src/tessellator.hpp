/**
 * @file tessellator.hpp
 * @brief Converts OCC shapes to render-ready mesh data.
 */

#pragma once

#include <opengeolab/scene/bounding_box.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>

#include <TopoDS_Shape.hxx>

namespace OpenGeoLab::Geometry {

/**
 * @brief Tessellation quality options.
 */
struct TessellationOptions {
    double linearDeflection = 0.1;  /**< Max chord height from surface. */
    double angularDeflection = 0.5; /**< Max angular deflection in radians. */
    bool relative = true;           /**< Deflection relative to edge length. */
};

/**
 * @brief Converts OCC shapes to render-ready mesh data.
 *
 * Stateless utility class with static methods.
 * Uses BRepMesh_IncrementalMesh internally.
 */
class Tessellator {
public:
    /**
     * @brief Triangulate all faces of an OCC shape.
     * @param shape OCC B-Rep shape to tessellate.
     * @param options Quality parameters.
     * @return RenderMeshData with PrimitiveType::Triangles.
     */
    [[nodiscard]] static Scene::RenderMeshData tessellate(const TopoDS_Shape& shape,
                                                          const TessellationOptions& options = {});

    /**
     * @brief Extract topological edge curves as line segments.
     * @param shape OCC B-Rep shape.
     * @return RenderMeshData with PrimitiveType::Lines.
     */
    [[nodiscard]] static Scene::RenderMeshData extractEdges(const TopoDS_Shape& shape);

    /**
     * @brief Compute axis-aligned bounding box from OCC shape.
     * @param shape OCC B-Rep shape.
     * @return Scene bounding box.
     */
    [[nodiscard]] static Scene::BoundingBox computeBounds(const TopoDS_Shape& shape);
};

} // namespace OpenGeoLab::Geometry
