/**
 * @file geometry_types.cpp
 * @brief Implementation of geometry type utilities
 */

#include "geometry/geometry_types.hpp"

#include <atomic>
#include <cmath>
#include <limits>

namespace OpenGeoLab::Geometry {

double Vector3D::length() const { return std::sqrt(x * x + y * y + z * z); }

Vector3D Vector3D::normalized() const {
    const double len = length();
    if(len < std::numeric_limits<double>::epsilon()) {
        return Vector3D(0.0, 0.0, 0.0);
    }
    return Vector3D(x / len, y / len, z / len);
}

double BoundingBox::diagonalLength() const {
    const double dx = max.x - min.x;
    const double dy = max.y - min.y;
    const double dz = max.z - min.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void BoundingBox::expand(const Point3D& point) {
    min.x = std::min(min.x, point.x);
    min.y = std::min(min.y, point.y);
    min.z = std::min(min.z, point.z);
    max.x = std::max(max.x, point.x);
    max.y = std::max(max.y, point.y);
    max.z = std::max(max.z, point.z);
}

void BoundingBox::expand(const BoundingBox& other) {
    if(!other.isValid()) {
        return;
    }
    expand(other.min);
    expand(other.max);
}

EntityId generateEntityId() {
    static std::atomic<EntityId> nextId{1};
    return nextId.fetch_add(1, std::memory_order_relaxed);
}

} // namespace OpenGeoLab::Geometry
