/**
 * @file geometry_converter.hpp
 * @brief Converts geometry model to renderable format
 *
 * Provides conversion between GeometryModel and RenderGeometry.
 */

#pragma once

#include "geometry/geometry_model.hpp"
#include "render/render_data.hpp"

#include <memory>

namespace OpenGeoLab {
namespace Render {

/**
 * @brief Converts geometry model to renderable format
 */
class GeometryConverter {
public:
    /**
     * @brief Convert geometry model to render geometry
     * @param model Source geometry model
     * @param defaultColor Default vertex color if not provided
     * @return Shared pointer to render geometry
     */
    static std::shared_ptr<RenderGeometry>
    convert(const Geometry::GeometryModel& model,
            const QVector3D& defaultColor = QVector3D(0.6f, 0.7f, 0.8f));
};

} // namespace Render
} // namespace OpenGeoLab
