/**
 * @file tessellation_service.hpp
 * @brief Service for tessellating OCC geometry to render-ready mesh data
 *
 * TessellationService extracts triangulated mesh data from OpenCASCADE geometry
 * entities. It generates vertex positions, normals, and indices suitable for
 * direct consumption by OpenGL rendering pipelines.
 */

#pragma once

#include "geometry/edge_entity.hpp"
#include "geometry/face_entity.hpp"
#include "geometry/geometry_document.hpp"
#include "geometry/part_entity.hpp"
#include "render/render_data.hpp"


namespace OpenGeoLab::Render {

/**
 * @brief Service for converting OCC geometry to renderable mesh data
 *
 * Extracts discretized geometry from GeometryDocument and its entities,
 * producing PartRenderData and DocumentRenderData suitable for OpenGL.
 * Each Part gets a distinct color for visual differentiation.
 */
class TessellationService {
public:
    TessellationService() = default;
    ~TessellationService() = default;

    /**
     * @brief Tessellate all parts in a document
     * @param document Source geometry document
     * @param params Tessellation quality parameters
     * @return Complete render data for all parts
     */
    [[nodiscard]] DocumentRenderDataPtr
    tessellateDocument(const Geometry::GeometryDocumentPtr& document,
                       const TessellationParams& params = TessellationParams::mediumQuality());

    /**
     * @brief Tessellate a single part entity
     * @param part_entity Source part entity
     * @param part_index Index for color generation
     * @param params Tessellation quality parameters
     * @return Render data for the part
     */
    [[nodiscard]] PartRenderDataPtr
    tessellatePart(const Geometry::PartEntityPtr& part_entity,
                   size_t part_index,
                   const TessellationParams& params = TessellationParams::mediumQuality());

    /**
     * @brief Tessellate a single face
     * @param face_entity Source face entity
     * @param color Color for the face vertices
     * @param params Tessellation quality parameters
     * @return Face render data
     */
    [[nodiscard]] RenderFace
    tessellateFace(const Geometry::FaceEntityPtr& face_entity,
                   const RenderColor& color,
                   const TessellationParams& params = TessellationParams::mediumQuality());

    /**
     * @brief Discretize an edge into polyline points
     * @param edge_entity Source edge entity
     * @param params Tessellation parameters for discretization
     * @return Edge render data with polyline points
     */
    [[nodiscard]] RenderEdge
    discretizeEdge(const Geometry::EdgeEntityPtr& edge_entity,
                   const TessellationParams& params = TessellationParams::mediumQuality());

private:
    /**
     * @brief Ensure shape has up-to-date triangulation
     * @param shape Shape to triangulate
     * @param params Tessellation parameters
     */
    void ensureTriangulation(const TopoDS_Shape& shape, const TessellationParams& params);

    /**
     * @brief Extract mesh data from a triangulated face
     * @param face OCC face with existing triangulation
     * @param color Vertex color
     * @return Face render data
     */
    [[nodiscard]] RenderFace extractFaceTriangulation(const TopoDS_Face& face,
                                                      const RenderColor& color);

    /**
     * @brief Discretize an edge to polyline
     * @param edge OCC edge
     * @param deflection Chord height tolerance
     * @return Vector of polyline points
     */
    [[nodiscard]] std::vector<Geometry::Point3D> discretizeEdgeCurve(const TopoDS_Edge& edge,
                                                                     double deflection);
};

} // namespace OpenGeoLab::Render
