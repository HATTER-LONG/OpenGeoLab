/**
 * @file GeometryService.hpp
 * @brief Provides lightweight geometry operations for the process protocol skeleton.
 */

#pragma once

#include <opengeolab/geometry/GeometryExport.hpp>

#include <nlohmann/json.hpp>

namespace OpenGeoLab::Geometry
{

/**
 * @brief Provides minimal geometry computations for protocol-driven backend requests.
 */
class OPENGEOLAB_GEOMETRY_EXPORT GeometryService
{
public:
    /**
     * @brief Describes a box primitive from JSON dimensions.
     * @param payload JSON payload with width, height, and depth members.
     * @return JSON description of the requested box primitive.
     * @note The payload dimensions must be positive numbers.
     */
    [[nodiscard]] static nlohmann::json describeBox(const nlohmann::json& payload);
};

}  // namespace OpenGeoLab::Geometry
