/**
 * @file geometry_module.hpp
 * @brief Geometry module facade for processing geometry requests.
 */

#pragma once

#include <opengeolab/geometry/geometry_export.hpp>

#include <string>

namespace OpenGeoLab::Geometry {

/**
 * @brief Facade for the geometry subsystem.
 *
 * Processes JSON-encoded geometry requests and returns JSON responses.
 */
class OPENGEOLAB_GEOMETRY_EXPORT GeometryModule {
public:
    GeometryModule() = default;

    /**
     * @brief Process a JSON request and return a JSON response.
     * @param request JSON-encoded request string.
     * @return JSON-encoded response string.
     */
    std::string process(const std::string& request);
};

}  // namespace OpenGeoLab::Geometry
