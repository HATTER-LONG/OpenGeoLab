/**
 * @file label_anchor.hpp
 * @brief Free functions for computing label anchor positions from vertex data.
 */

#pragma once

#include <opengeolab/render/render_export.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace OpenGeoLab::Render {

/// Compute the centroid of a set of vertex positions.
/// Returns origin if positions is empty.
[[nodiscard]] OPENGEOLAB_RENDER_EXPORT glm::vec3
computeAnchorFromVertices(std::span<const glm::vec3> positions);

/// Assign stack indices for overlapping labels.
/// Labels whose screen positions are within @p tolerance pixels of each other
/// are grouped and assigned sequential stackIndex values (0, 1, 2, ...).
/// @return Vector of stack indices, one per input position.
[[nodiscard]] OPENGEOLAB_RENDER_EXPORT std::vector<uint32_t>
computeStackIndices(std::span<const glm::vec2> screen_positions, float tolerance);

} // namespace OpenGeoLab::Render
