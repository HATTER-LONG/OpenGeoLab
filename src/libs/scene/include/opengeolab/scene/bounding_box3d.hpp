/**
 * @file bounding_box3d.hpp
 * @brief Axis-aligned bounding box for scene objects
 */

#pragma once

#include <opengeolab/scene/scene_export.hpp>

#include <glm/glm.hpp>

#include <limits>

namespace OpenGeoLab::Scene {

/**
 * @brief Axis-aligned bounding box (AABB).
 *
 * Default-constructed box is invalid (min > max).
 * Call expand() to grow it around points or other boxes.
 */
struct OPENGEOLAB_SCENE_EXPORT BoundingBox3D {
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{std::numeric_limits<float>::lowest()};

    /** @brief Expand to include a point. */
    void expand(const glm::vec3& point);

    /** @brief Expand to include another bounding box. */
    void expand(const BoundingBox3D& other);

    /** @brief True when at least one point has been added. */
    [[nodiscard]] bool isValid() const;

    /** @brief Center of the box. Undefined if !isValid(). */
    [[nodiscard]] glm::vec3 center() const;

    /** @brief Size along each axis. */
    [[nodiscard]] glm::vec3 size() const;

    /** @brief Length of the box diagonal. */
    [[nodiscard]] float diagonal() const;

    /**
     * @brief Return a new AABB that encloses this box after a matrix transform.
     *
     * Transforms all 8 corners and rebuilds the AABB from them.
     */
    [[nodiscard]] BoundingBox3D transformed(const glm::mat4& matrix) const;
};

} // namespace OpenGeoLab::Scene
