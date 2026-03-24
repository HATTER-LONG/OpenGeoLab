/// @file box_data.hpp
/// @brief Value type representing a generated box.
#pragma once

#include <opengeolab/geometry/geometry_export.hpp>

#include <array>
#include <string>

namespace OpenGeoLab::Geometry {

/// @brief Immutable description of a generated box.
struct OPENGEOLAB_GEOMETRY_EXPORT BoxData {
    std::array<double, 3> center{0.0, 0.0, 0.0}; ///< Box center coordinates.
    std::array<double, 3> size{1.0, 1.0, 1.0};   ///< Box dimensions (width, height, depth).
    int vertexCount = 8;                         ///< Number of generated vertices.
    std::string label;                           ///< Optional human-readable label.
};

} // namespace OpenGeoLab::Geometry
