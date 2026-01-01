/**
 * @file occ_converter.hpp
 * @brief OpenCASCADE geometry converter utilities.
 *
 * Provides conversion between OpenCASCADE TopoDS shapes and
 * the application's geometry data structures.
 */
#pragma once

#include "geometry/geometry_model.hpp"
#include "geometry/geometry_types.hpp"

#include <TopoDS_Shape.hxx>

#include <memory>
#include <string>

namespace OpenGeoLab {
namespace Geometry {

/**
 * @brief Converts OpenCASCADE shapes to application geometry format.
 *
 * Handles tessellation of BREP shapes for visualization and
 * extracts topology (parts, solids, faces, edges, vertices).
 */
class OccConverter {
public:
    /**
     * @brief Tessellation quality settings.
     */
    struct TessellationParams {
        double linearDeflection;  ///< Linear deflection for tessellation.
        double angularDeflection; ///< Angular deflection in radians.
        bool relative;            ///< Use relative deflection.

        TessellationParams() : linearDeflection(0.1), angularDeflection(0.5), relative(true) {}
    };

    OccConverter() = default;
    ~OccConverter() = default;

    /**
     * @brief Convert OCC shape to GeometryModel.
     * @param shape Input TopoDS_Shape from OCC.
     * @param partName Name for the created part.
     * @param params Tessellation parameters.
     * @return Shared pointer to populated GeometryModel.
     */
    GeometryModelPtr convertShape(const TopoDS_Shape& shape,
                                  const std::string& partName,
                                  const TessellationParams& params = TessellationParams());

    /**
     * @brief Add OCC shape to existing model.
     * @param shape Input TopoDS_Shape from OCC.
     * @param partName Name for the created part.
     * @param model Target model to populate.
     * @param params Tessellation parameters.
     * @return True on success.
     */
    bool addShapeToModel(const TopoDS_Shape& shape,
                         const std::string& partName,
                         GeometryModel& model,
                         const TessellationParams& params = TessellationParams());

private:
    /**
     * @brief Tessellate shape for visualization.
     * @param shape Shape to tessellate.
     * @param params Tessellation parameters.
     * @return True if tessellation succeeded.
     */
    bool tessellateShape(const TopoDS_Shape& shape, const TessellationParams& params);

    /**
     * @brief Extract topology and mesh data from tessellated shape.
     * @param shape Tessellated shape.
     * @param partName Name for the part.
     * @param model Target model to populate.
     */
    void
    extractGeometry(const TopoDS_Shape& shape, const std::string& partName, GeometryModel& model);
};

} // namespace Geometry
} // namespace OpenGeoLab
