/**
 * @file occ_primitives.hpp
 * @brief Factory functions for creating OCC primitive shapes.
 */

#pragma once

#include <TopoDS_Shape.hxx>

#include <array>

namespace OpenGeoLab::Geometry {

/**
 * @brief Create an OCC box centered at the given position.
 * @param center Box center coordinates.
 * @param size Box dimensions along the X, Y, and Z axes.
 * @return OCC solid shape.
 */
[[nodiscard]] TopoDS_Shape makeBox(std::array<double, 3> center, std::array<double, 3> size);

/**
 * @brief Create an OCC cylinder at the given position.
 * @param center Base center coordinates.
 * @param radius Cylinder radius.
 * @param height Cylinder height along the positive Z axis.
 * @return OCC solid shape.
 */
[[nodiscard]] TopoDS_Shape makeCylinder(std::array<double, 3> center, double radius, double height);

/**
 * @brief Create an OCC sphere at the given position.
 * @param center Sphere center coordinates.
 * @param radius Sphere radius.
 * @return OCC solid shape.
 */
[[nodiscard]] TopoDS_Shape makeSphere(std::array<double, 3> center, double radius);

/**
 * @brief Create an OCC torus at the given position.
 * @param center Torus center coordinates.
 * @param major_radius Distance from center to tube center.
 * @param minor_radius Tube radius.
 * @return OCC solid shape.
 */
[[nodiscard]] TopoDS_Shape
makeTorus(std::array<double, 3> center, double major_radius, double minor_radius);

} // namespace OpenGeoLab::Geometry
