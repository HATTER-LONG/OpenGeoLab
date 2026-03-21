/**
 * @file geometry_registration.hpp
 * @brief Declares the explicit registration entry point for the geometry module.
 */

#pragma once

#include <opengeolab/geometry/geometry_export.hpp>

namespace Kangaroo::Util {
class PluginComponentFactory;
} // namespace Kangaroo::Util

namespace OpenGeoLab::Geometry {

/**
 * @brief Registers geometry module services and actions with the given factory.
 *
 * Registers GeometryModule as a singleton, plus all geometry actions
 * (bounding_box, set_points, get_stored_bbox) as transient entries.
 * Must be called once before dispatching geometry requests.
 *
 * @param factory Component factory receiving the registrations.
 */
OPENGEOLAB_GEOMETRY_EXPORT void
registerGeometryModule(Kangaroo::Util::PluginComponentFactory& factory);

} // namespace OpenGeoLab::Geometry
