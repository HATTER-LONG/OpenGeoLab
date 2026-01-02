/**
 * @file render_data.cpp
 * @brief Implementation of render data structures
 */

#include "render/render_data.hpp"

#include <algorithm>
#include <limits>

namespace OpenGeoLab {
namespace Render {

void RenderGeometry::clear() {
    vertices.clear();
    indices.clear();
}

QVector3D RenderGeometry::boundingBoxMin() const {
    if(vertices.empty()) {
        return QVector3D(0, 0, 0);
    }

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();

    for(const auto& v : vertices) {
        minX = std::min(minX, v.position.x());
        minY = std::min(minY, v.position.y());
        minZ = std::min(minZ, v.position.z());
    }

    return QVector3D(minX, minY, minZ);
}

QVector3D RenderGeometry::boundingBoxMax() const {
    if(vertices.empty()) {
        return QVector3D(0, 0, 0);
    }

    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    for(const auto& v : vertices) {
        maxX = std::max(maxX, v.position.x());
        maxY = std::max(maxY, v.position.y());
        maxZ = std::max(maxZ, v.position.z());
    }

    return QVector3D(maxX, maxY, maxZ);
}

QVector3D RenderGeometry::center() const { return (boundingBoxMin() + boundingBoxMax()) * 0.5f; }

} // namespace Render
} // namespace OpenGeoLab
