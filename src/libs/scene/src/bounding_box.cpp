#include <opengeolab/scene/bounding_box.hpp>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <cmath>
#include <cstring>
#include <limits>

namespace OpenGeoLab::Scene {

void BoundingBox::expand(const glm::vec3& point) {
    m_min = glm::min(m_min, point);
    m_max = glm::max(m_max, point);
}

void BoundingBox::expand(const BoundingBox& other) {
    if(!other.isValid()) {
        return;
    }
    m_min = glm::min(m_min, other.m_min);
    m_max = glm::max(m_max, other.m_max);
}

glm::vec3 BoundingBox::center() const { return (m_min + m_max) * 0.5f; }

float BoundingBox::radius() const { return glm::length(m_max - m_min) * 0.5f; }

bool BoundingBox::isValid() const {
    return m_min.x <= m_max.x && m_min.y <= m_max.y && m_min.z <= m_max.z;
}

void BoundingBox::reset() {
    m_min = glm::vec3{std::numeric_limits<float>::max()};
    m_max = glm::vec3{std::numeric_limits<float>::lowest()};
}

const glm::vec3& BoundingBox::min() const { return m_min; }

const glm::vec3& BoundingBox::max() const { return m_max; }

BoundingBox BoundingBox::fromPositions(const float* data, std::size_t count, std::size_t stride) {
    BoundingBox box;
    if(data == nullptr || count == 0) {
        return box;
    }

    const auto* raw = reinterpret_cast<const unsigned char*>(data);
    for(std::size_t i = 0; i < count; ++i) {
        glm::vec3 pos;
        std::memcpy(&pos, raw + i * stride, sizeof(glm::vec3));
        box.expand(pos);
    }
    return box;
}

} // namespace OpenGeoLab::Scene
