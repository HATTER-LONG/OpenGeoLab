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
 */
struct OPENGEOLAB_GEOMETRY_EXPORT TessellationParams {
    double linearDeflection = 0.1;  /**< Maximum chord deviation from the true surface */
    double angularDeflection = 0.5; /**< Maximum angular deviation (radians) */

    /**
     * @brief Build TessellationParams from a JSON object, falling back to defaults.
     *
     * Recognised keys: `"linearDeflection"`, `"angularDeflection"`.
     */
    static TessellationParams fromJson(const nlohmann::json& j) {
        return {j.value("linearDeflection", 0.1), j.value("angularDeflection", 0.5)};
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
 * @brief Tessellate a ShapeEntry into render-ready data.
 *
 * @param entry  ShapeEntry with valid shape and sub-shape maps
 * @param params Tessellation quality parameters
 * @return Populated TessellationResult
 */
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT TessellationResult
tessellate(const ShapeEntry& entry, const TessellationParams& params = {});

} // namespace OpenGeoLab::Geometry
