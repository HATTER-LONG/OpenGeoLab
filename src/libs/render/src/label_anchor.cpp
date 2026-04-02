/**
 * @file label_anchor.cpp
 * @brief Label anchor computation and stacking logic
 */

#include <opengeolab/render/label_anchor.hpp>

#include <glm/geometric.hpp>

#include <algorithm>
#include <numeric>

namespace OpenGeoLab::Render {

glm::vec3 computeAnchorFromVertices(std::span<const glm::vec3> positions) {
    if(positions.empty()) {
        return glm::vec3{0.0F};
    }
    auto sum =
        std::accumulate(positions.begin(), positions.end(), glm::vec3{0.0F});
    return sum / static_cast<float>(positions.size());
}

std::vector<uint32_t>
computeStackIndices(std::span<const glm::vec2> screen_positions, float tolerance) {
    const auto count = screen_positions.size();
    std::vector<uint32_t> indices(count, 0);
    if(count <= 1) {
        return indices;
    }

    // For each label, count how many earlier labels overlap with it.
    const float tol2 = tolerance * tolerance;
    for(std::size_t i = 1; i < count; ++i) {
        uint32_t stack = 0;
        for(std::size_t j = 0; j < i; ++j) {
            auto diff = screen_positions[i] - screen_positions[j];
            if(glm::dot(diff, diff) <= tol2) {
                stack = std::max(stack, indices[j] + 1);
            }
        }
        indices[i] = stack;
    }
    return indices;
}

} // namespace OpenGeoLab::Render
