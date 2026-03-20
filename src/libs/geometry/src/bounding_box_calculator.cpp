#include <opengeolab/geometry/bounding_box_calculator.hpp>

#include <random>
#include <stdexcept>

namespace OpenGeoLab::Geometry {

BoundingBox BoundingBoxCalculator::compute(std::span<const Point3D> points) {
    if(points.empty()) {
        throw std::invalid_argument("BoundingBoxCalculator::compute requires at least one point");
    }

    BoundingBox bounding_box{.min = points.front(), .max = points.front()};
    for(const Point3D& point : points.subspan(1)) {
        if(point.x < bounding_box.min.x) {
            bounding_box.min.x = point.x;
        }
        if(point.y < bounding_box.min.y) {
            bounding_box.min.y = point.y;
        }
        if(point.z < bounding_box.min.z) {
            bounding_box.min.z = point.z;
        }
        if(point.x > bounding_box.max.x) {
            bounding_box.max.x = point.x;
        }
        if(point.y > bounding_box.max.y) {
            bounding_box.max.y = point.y;
        }
        if(point.z > bounding_box.max.z) {
            bounding_box.max.z = point.z;
        }
    }

    return bounding_box;
}

std::vector<Point3D> BoundingBoxCalculator::generateRandomPoints(std::size_t count,
                                                                 unsigned int seed) {
    std::mt19937 engine(seed);
    std::uniform_real_distribution<double> distribution(-1000.0, 1000.0);

    std::vector<Point3D> points;
    points.reserve(count);
    for(std::size_t index = 0; index < count; ++index) {
        points.push_back(Point3D{
            .x = distribution(engine), .y = distribution(engine), .z = distribution(engine)});
    }

    return points;
}

} // namespace OpenGeoLab::Geometry
