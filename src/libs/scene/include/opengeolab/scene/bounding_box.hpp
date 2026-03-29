/**
 * @file bounding_box.hpp
 * @brief Axis-aligned bounding box (AABB) for scene objects.
 */

#pragma once

#include <opengeolab/scene/scene_export.hpp>

#include <glm/vec3.hpp>

#include <cstddef>

namespace OpenGeoLab::Scene {

/// Axis-aligned bounding box (AABB).
class OPENGEOLAB_SCENE_EXPORT BoundingBox {
public:
    /// Expand the box to include a single point.
    void expand(const glm::vec3& point);

    /// Expand the box to include another bounding box.
    void expand(const BoundingBox& other);

    /// Geometric center of the box.
    [[nodiscard]] glm::vec3 center() const;

    /// Half-diagonal length (bounding sphere radius).
    [[nodiscard]] float radius() const;

    /// True if at least one point has been added.
    [[nodiscard]] bool isValid() const;

    /// Reset to invalid (empty) state.
    void reset();

    [[nodiscard]] const glm::vec3& min() const;
    [[nodiscard]] const glm::vec3& max() const;

    /**
     * @brief Build a bounding box from a packed float array.
     * @param data     Pointer to the first float of the first position.
     * @param count    Number of vertices.
     * @param stride   Byte stride between consecutive positions (default 12 = 3 floats).
     */
    [[nodiscard]] static BoundingBox
    fromPositions(const float* data, std::size_t count, std::size_t stride = 12);

private:
    glm::vec3 m_min{std::numeric_limits<float>::max()};
    glm::vec3 m_max{std::numeric_limits<float>::lowest()};
};

} // namespace OpenGeoLab::Scene
