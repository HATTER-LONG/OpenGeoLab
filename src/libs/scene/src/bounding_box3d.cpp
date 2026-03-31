#include <opengeolab/scene/bounding_box3d.hpp>

#include <glm/geometric.hpp>
#include <glm/vec4.hpp>

namespace OpenGeoLab::Scene {

void BoundingBox3D::expand(const glm::vec3& point) {
    min = glm::min(min, point);
    max = glm::max(max, point);
}

void BoundingBox3D::expand(const BoundingBox3D& other) {
    if(!other.isValid()) {
        return;
    }

    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
}

bool BoundingBox3D::isValid() const { return min.x <= max.x; }

glm::vec3 BoundingBox3D::center() const { return (min + max) * 0.5F; }

glm::vec3 BoundingBox3D::size() const { return max - min; }

float BoundingBox3D::diagonal() const { return glm::length(size()); }

BoundingBox3D BoundingBox3D::transformed(const glm::mat4& matrix) const {
    if(!isValid()) {
        return {};
    }

    BoundingBox3D result;
    for(int x = 0; x < 2; ++x) {
        for(int y = 0; y < 2; ++y) {
            for(int z = 0; z < 2; ++z) {
                const glm::vec3 corner{
                    x == 0 ? min.x : max.x,
                    y == 0 ? min.y : max.y,
                    z == 0 ? min.z : max.z,
                };
                const glm::vec4 transformed_corner = matrix * glm::vec4(corner, 1.0F);
                result.expand(glm::vec3{transformed_corner});
            }
        }
    }

    return result;
}

} // namespace OpenGeoLab::Scene
