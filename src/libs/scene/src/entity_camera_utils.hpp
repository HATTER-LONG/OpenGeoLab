/**
 * @file entity_camera_utils.hpp
 * @brief Internal helpers for computing camera targets from topology entities
 */

#pragma once

#include <opengeolab/scene/bounding_box3d.hpp>

#include <glm/glm.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace OpenGeoLab::Geometry {
class ShapeStore;
} // namespace OpenGeoLab::Geometry

namespace OpenGeoLab::Scene {

/**
 * @brief Result of computing a camera target from a topology entity.
 *
 * Contains the point to look at, the ideal viewing direction (from
 * entity toward camera), and the entity's local bounding box.
 */
struct EntityCameraTarget {
    glm::vec3 center{0};          ///< Point to look at (face center / edge midpoint / vertex)
    glm::vec3 direction{0, 0, 1}; ///< Viewing direction (outward from entity, normalized)
    BoundingBox3D entityBounds;   ///< Entity local bounding box
};

/**
 * @brief Compute camera target for a topology entity.
 *
 * @param store ShapeStore to look up the entity.
 * @param shape_id Shape identifier.
 * @param entity_type "face", "edge", or "vertex" (LLM-friendly short names).
 * @param local_id 1-based index within the shape.
 * @param out_error Optional output for error message.
 * @return Target info, or nullopt with error string.
 *
 * Direction logic (per design spec):
 * - Face: outward face normal
 * - Edge: average normal of adjacent faces (fallback: world +Z)
 * - Vertex: average normal of adjacent faces via adjacent edges
 */
std::optional<EntityCameraTarget> computeEntityCameraTarget(const Geometry::ShapeStore& store,
                                                            uint32_t shape_id,
                                                            std::string_view entity_type,
                                                            uint32_t local_id,
                                                            std::string* out_error = nullptr);

/**
 * @brief Choose the best world-axis up vector for a viewing direction.
 *
 * Returns world Y (0,1,0) or Z (0,0,1), whichever is more orthogonal
 * to the viewing direction.
 */
glm::vec3 chooseUpVector(const glm::vec3& direction);

} // namespace OpenGeoLab::Scene
