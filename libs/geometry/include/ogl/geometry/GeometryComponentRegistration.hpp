/**
 * @file GeometryComponentRegistration.hpp
 * @brief Registers placeholder geometry services into Kangaroo ComponentFactory.
 */

#pragma once

#include <ogl/geometry/export.hpp>

namespace ogl::geometry {

/**
 * @brief Register geometry services exactly once for the current process.
 */
OGL_GEOMETRY_EXPORT void registerGeometryComponents();

} // namespace ogl::geometry