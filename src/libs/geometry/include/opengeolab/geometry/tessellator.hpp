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

#include <nlohmann/json.hpp>

#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief Parameters controlling BRepMesh tessellation quality.
 *
 * Centralises all deflection-related knobs that were previously scattered
 * as magic literals across actions.  Use the default-constructed instance
 * for a sensible starting point.
 *
 * When @c linearDeflection is zero (the default), the tessellator will
 * automatically compute an appropriate value from the shape's bounding box
 * via @ref calculateDeflection.
 */
struct OPENGEOLAB_GEOMETRY_EXPORT TessellationParams {
    double linearDeflection = 0.0;   /**< Chord deviation; 0 = auto-calculate from shape */
    double angularDeflection = 0.25; /**< Maximum angular deviation (radians) */
    double tessRatio = 1.0;         /**< Quality multiplier applied to auto-calculated deflection */
    bool keepTriangulation = false; /**< Preserve existing Poly_Triangulation on faces */

    /**
     * @brief Build TessellationParams from a JSON object, falling back to defaults.
     *
     * Recognised keys: `"linearDeflection"`, `"angularDeflection"`,
     * `"tessRatio"`, `"keepTriangulation"`.
     *
     * If `"linearDeflection"` is absent, the value stays at 0 and the
     * tessellator will auto-calculate from the shape bounding box.
     */
    static TessellationParams fromJson(const nlohmann::json& j) {
        return {j.value("linearDeflection", 0.0), j.value("angularDeflection", 0.25),
                j.value("tessRatio", 1.0), j.value("keepTriangulation", false)};
    }
};

/**
 * @brief Result of tessellating a ShapeEntry.
 */
struct OPENGEOLAB_GEOMETRY_EXPORT TessellationResult {
    Core::VisualData visualData;
    std::vector<Core::EntityTag> triangleTags; /**< One per triangle (across all faces) */
    std::vector<Core::EntityTag> edgeTags;     /**< One per edge line-segment */
    std::vector<Core::EntityTag> vertexTags;   /**< One per topological vertex */
};

/**
 * @brief Compute a suitable linear deflection for the given shape.
 *
 * The deflection is proportional to the largest bounding-box dimension,
 * scaled by a base accuracy (0.0001) and the caller-supplied @p tess_ratio.
 * Wire-like shapes receive finer resolution automatically.
 * The result is clamped to a safe range above Precision::Confusion().
 *
 * @param shape     OCC topological shape
 * @param tess_ratio Quality multiplier (default 1.0; smaller = finer mesh)
 * @return Positive linear deflection suitable for BRepMesh_IncrementalMesh
 */
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT double calculateDeflection(const TopoDS_Shape& shape,
                                                                    double tess_ratio = 1.0);

/**
 * @brief Tessellate a ShapeEntry into render-ready data.
 *
 * If @c params.linearDeflection is zero (the default), the function
 * calls @ref calculateDeflection internally to derive an appropriate
 * value from the shape's bounding box and @c params.tess_ratio.
 *
 * @param entry  ShapeEntry with valid shape and sub-shape maps
 * @param params Tessellation quality parameters
 * @return Populated TessellationResult
 */
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT TessellationResult
tessellate(const ShapeEntry& entry, const TessellationParams& params = {});

} // namespace OpenGeoLab::Geometry
