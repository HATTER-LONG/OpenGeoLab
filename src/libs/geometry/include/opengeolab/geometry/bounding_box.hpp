/**
 * @file bounding_box.hpp
 * @brief Declares basic point and bounding-box value types for geometry modules.
 */

#pragma once

namespace OpenGeoLab::Geometry {

/**
 * @brief Represents a point in three-dimensional Cartesian space.
 */
struct Point3D {
    double x{};
    double y{};
    double z{};
};

/**
 * @brief Represents the minimum and maximum corners of an axis-aligned bounding box.
 */
struct BoundingBox {
    Point3D min;
    Point3D max;
};

} // namespace OpenGeoLab::Geometry
