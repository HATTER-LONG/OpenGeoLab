/**
 * @file bounding_box.hpp
 * @brief Declares axis-aligned bounding box helpers for scene content.
 */
#pragma once

#include <glm/common.hpp>
#include <glm/vec3.hpp>

#include <limits>

namespace OpenGeoLab::Scene {

/**
 * @brief Axis-aligned bounding box defined by minimum and maximum corners.
 */
struct BoundingBox {
    glm::vec3 min{std::numeric_limits<float>::max()};    /**< Minimum corner. */
    glm::vec3 max{std::numeric_limits<float>::lowest()}; /**< Maximum corner. */

    /**
     * @brief Returns whether the bounding box contains at least one point.
     */
    [[nodiscard]] bool isValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    /**
     * @brief Returns the center point, or the origin if the box is invalid.
     */
    [[nodiscard]] glm::vec3 center() const {
        return isValid() ? (min + max) * 0.5F : glm::vec3{0.0F};
    }

    /**
     * @brief Returns the size vector, or zero if the box is invalid.
     */
    [[nodiscard]] glm::vec3 size() const { return isValid() ? max - min : glm::vec3{0.0F}; }

    /**
     * @brief Expands the box to include @p point.
     * @param point Point to include in the box.
     */
    void expand(const glm::vec3& point) {
        if(!isValid()) {
            min = point;
            max = point;
            return;
        }

        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    /**
     * @brief Expands the box to include another valid box.
     * @param other Bounds to merge into this box.
     */
    void merge(const BoundingBox& other) {
        if(!other.isValid()) {
            return;
        }

        expand(other.min);
        expand(other.max);
    }
};

} // namespace OpenGeoLab::Scene
