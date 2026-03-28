/**
 * @file tessellator.hpp
 * @brief Tessellator — converts OCC BRep geometry to render-ready VisualData
 *
 * Uses BRepMesh_IncrementalMesh for face triangulation, BRepAdaptor_Curve
 * for edge polylines, and BRep_Tool::Pnt for vertex extraction.  Each
 * primitive is tagged with an EntityTag linking back to the originating
 * sub-shape local index for pick support.
 */

#pragma once

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/core/visual_data.hpp>
#include <opengeolab/geometry/geometry_export.hpp>
#include <opengeolab/geometry/shape_entry.hpp>

#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief Result of tessellating a ShapeEntry.
 */
struct OPENGEOLAB_GEOMETRY_EXPORT TessellationResult {
    Core::VisualData visualData;
    std::vector<Core::EntityTag> triangleTags; ///< One per triangle (across all faces)
    std::vector<Core::EntityTag> edgeTags;     ///< One per edge line-segment
    std::vector<Core::EntityTag> vertexTags;   ///< One per topological vertex
};

/**
 * @brief Tessellate a ShapeEntry into render-ready data.
 *
 * @param entry             ShapeEntry with valid shape and sub-shape maps
 * @param linearDeflection  Chord deviation for BRepMesh
 * @param angularDeflection Angular deviation in radians for BRepMesh
 * @return Populated TessellationResult
 */
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT TessellationResult tessellate(const ShapeEntry& entry,
                                                                       double linearDeflection,
                                                                       double angularDeflection);

} // namespace OpenGeoLab::Geometry
