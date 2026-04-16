/**
 * @file split_mode.hpp
 * @brief SplitMode — mesh element split pattern classification
 */

#pragma once

#include <cstdint>
#include <string_view>

namespace OpenGeoLab::Mesh {

/**
 * @brief Split pattern applied to a mesh element.
 *
 * Values match reference imMeshSpliter::EdgeType / NodeType for cross-referencing.
 * Bit-flag style values allow future combination if needed.
 */
enum class SplitMode : uint8_t {
    Auto = 0,             ///< Auto-detect split pattern from selection
    TriaOneQuadThree = 1, ///< 3 quads + 1 triangle (quad 3-edge, center P)
    TriaOneQuadTwo = 2,   ///< 2 quads + 1 triangle (quad 3-edge, Mr connects)
    TriaThreeQuadTwo = 4, ///< 2 quads + 3 triangles (quad 3-edge, center P variant)
    TriaFour = 8,         ///< 4 triangles (triangle 3-edge uniform)
    QuadThree = 16,       ///< 3 quads (triangle 3-edge centroid)
    TriaThree = 32,       ///< 3 triangles (triangle 3-node centroid)
};

/// Convert mode enum to JSON-friendly string identifier.
[[nodiscard]] constexpr std::string_view splitModeToString(SplitMode mode) noexcept {
    switch(mode) {
    case SplitMode::Auto:
        return "auto";
    case SplitMode::TriaOneQuadThree:
        return "tria_one_quad_three";
    case SplitMode::TriaOneQuadTwo:
        return "tria_one_quad_two";
    case SplitMode::TriaThreeQuadTwo:
        return "tria_three_quad_two";
    case SplitMode::TriaFour:
        return "tria_four";
    case SplitMode::QuadThree:
        return "quad_three";
    case SplitMode::TriaThree:
        return "tria_three";
    }
    return "unknown";
}

} // namespace OpenGeoLab::Mesh
