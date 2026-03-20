#include <opengeolab/geometry/point_store.hpp>

namespace OpenGeoLab::Geometry {

void PointStore::setPoints(std::vector<Point3D> points) {
    std::lock_guard lock(mutex_);
    points_ = std::move(points);
}

auto PointStore::points() const -> std::vector<Point3D> {
    std::lock_guard lock(mutex_);
    return points_;
}

auto PointStore::size() const -> std::size_t {
    std::lock_guard lock(mutex_);
    return points_.size();
}

auto PointStore::empty() const -> bool {
    std::lock_guard lock(mutex_);
    return points_.empty();
}

} // namespace OpenGeoLab::Geometry
