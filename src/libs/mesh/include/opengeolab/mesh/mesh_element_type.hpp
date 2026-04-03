/**
 * @file mesh_element_type.hpp
 * @brief MeshElementType — finite element topology classification
 */

#pragma once

#include <cstdint>
#include <string_view>

namespace OpenGeoLab::Mesh {

/**
 * @brief Finite element topology types.
 *
 * Each value identifies the element shape and its node count.
 */
enum class MeshElementType : uint8_t {
    Triangle = 0, ///< 3-node triangle (2D)
    Quad = 1,     ///< 4-node quadrilateral (2D)
    Tetra = 2,    ///< 4-node tetrahedron (3D)
    Hexa = 3,     ///< 8-node hexahedron (3D)
    Prism = 4,    ///< 6-node prism/wedge (3D)
    Pyramid = 5,  ///< 5-node pyramid (3D)
};

/// Number of nodes for the given element type.
[[nodiscard]] constexpr uint8_t nodeCount(MeshElementType type) noexcept {
    switch(type) {
    case MeshElementType::Triangle:
        return 3;
    case MeshElementType::Quad:
        return 4;
    case MeshElementType::Tetra:
        return 4;
    case MeshElementType::Hexa:
        return 8;
    case MeshElementType::Prism:
        return 6;
    case MeshElementType::Pyramid:
        return 5;
    }
    return 0;
}

/// Maximum node count across all element types.
inline constexpr uint8_t K_MAX_ELEMENT_NODES = 8;

/// Short label prefix for display (e.g. "Tri", "Tet").
[[nodiscard]] constexpr std::string_view elementTypePrefix(MeshElementType type) noexcept {
    switch(type) {
    case MeshElementType::Triangle:
        return "Tri";
    case MeshElementType::Quad:
        return "Q";
    case MeshElementType::Tetra:
        return "Tet";
    case MeshElementType::Hexa:
        return "Hex";
    case MeshElementType::Prism:
        return "Pri";
    case MeshElementType::Pyramid:
        return "Pyr";
    }
    return "?";
}

/// Dimension of the element (2 for surface, 3 for volume).
[[nodiscard]] constexpr uint8_t elementDimension(MeshElementType type) noexcept {
    switch(type) {
    case MeshElementType::Triangle:
    case MeshElementType::Quad:
        return 2;
    default:
        return 3;
    }
}

} // namespace OpenGeoLab::Mesh
