#include <opengeolab/geometry/point_store.hpp>

namespace OpenGeoLab::Geometry {

void PointStore::setPoints(std::vector<Point3D> points) {
    const std::lock_guard lock(mutex_);
    points_ = std::move(points);
}

auto PointStore::points() const -> std::vector<Point3D> {
    const std::lock_guard lock(mutex_);
    return points_;
}

auto PointStore::size() const -> std::size_t {
    const std::lock_guard lock(mutex_);
    return points_.size();
}

auto PointStore::empty() const -> bool {
    const std::lock_guard lock(mutex_);
    return points_.empty();
}

} // namespace OpenGeoLab::Geometry
