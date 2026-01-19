/**
 * @file render_data.cpp
 * @brief Implementation of render data structures
 */

#include "render/render_data.hpp"

#include <cmath>

namespace OpenGeoLab::Render {

void Camera::fitToBoundingBox(const Geometry::BoundingBox& bbox) {
    if(!bbox.isValid()) {
        reset();
        return;
    }

    // Calculate center and size
    Geometry::Point3D center = bbox.center();
    double diagonal = bbox.diagonalLength();

    if(diagonal < 1e-6) {
        diagonal = 100.0;
    }

    // Position camera to view the entire object
    double distance = diagonal * 1.5 / std::tan(fov * 0.5 * 3.14159265 / 180.0);

    target = {static_cast<float>(center.x), static_cast<float>(center.y),
              static_cast<float>(center.z)};

    // Position camera along a 45-degree diagonal view
    float offset = static_cast<float>(distance / std::sqrt(3.0));
    position = {target[0] + offset, target[1] + offset, target[2] + offset};

    // Adjust near/far planes based on object size
    nearPlane = static_cast<float>(diagonal * 0.001);
    farPlane = static_cast<float>(diagonal * 100.0);
}

} // namespace OpenGeoLab::Render
