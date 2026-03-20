/**
 * @file bounding_box_calculator.hpp
 * @brief Declares utilities for generating point clouds and computing bounding boxes.
 */

#pragma once

#include <opengeolab/geometry/bounding_box.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief Computes axis-aligned bounding boxes for point sets.
 */
class OPENGEOLAB_GEOMETRY_EXPORT BoundingBoxCalculator {
public:
    /**
     * @brief Computes the axis-aligned bounding box of the provided points.
     * @param points Points to inspect. All coordinates must be finite (non-NaN, non-Inf);
     *        NaN values cause silently incorrect results due to IEEE 754 comparison semantics.
     * @return Bounding box containing all input points.
     * @throws std::invalid_argument Thrown when @p points is empty.
     */
    [[nodiscard]] static BoundingBox compute(std::span<const Point3D> points);

    /**
     * @brief Generates pseudo-random points in the range [-1000, 1000] for each axis.
     * @param count Number of points to generate.
     * @param seed Seed used to initialize the pseudo-random engine.
     * @return Generated points.
     */
    [[nodiscard]] static std::vector<Point3D> generateRandomPoints(std::size_t count,
                                                                   unsigned int seed = 42U);
};

} // namespace OpenGeoLab::Geometry
