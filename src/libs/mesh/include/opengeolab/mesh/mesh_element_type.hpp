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
    Tri6 = 6,     ///< 6-node quadratic triangle (2D)
    Quad8 = 7,    ///< 8-node serendipity quadrilateral (2D)
    Quad9 = 8,    ///< 9-node quadratic quadrilateral (2D)
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
    case MeshElementType::Tri6:
        return 6;
    case MeshElementType::Quad8:
        return 8;
    case MeshElementType::Quad9:
        return 9;
    }
    return 0;
}

/// Maximum node count across all element types.
inline constexpr uint8_t K_MAX_ELEMENT_NODES = 9;

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
    case MeshElementType::Tri6:
        return "Tri6";
    case MeshElementType::Quad8:
        return "Q8";
    case MeshElementType::Quad9:
        return "Q9";
    }
    return "?";
}

/// Dimension of the element (2 for surface, 3 for volume).
[[nodiscard]] constexpr uint8_t elementDimension(MeshElementType type) noexcept {
    switch(type) {
    case MeshElementType::Triangle:
    case MeshElementType::Quad:
    case MeshElementType::Tri6:
    case MeshElementType::Quad8:
    case MeshElementType::Quad9:
        return 2;
    default:
        return 3;
    }
}

/// Number of corner (vertex) nodes — excludes mid-edge/mid-face nodes.
[[nodiscard]] constexpr uint8_t cornerCount(MeshElementType type) noexcept {
    switch(type) {
    case MeshElementType::Triangle:
    case MeshElementType::Tri6:
        return 3;
    case MeshElementType::Quad:
    case MeshElementType::Quad8:
    case MeshElementType::Quad9:
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

/// Map a second-order type to its first-order equivalent for rendering.
[[nodiscard]] constexpr MeshElementType linearEquivalent(MeshElementType type) noexcept {
    switch(type) {
    case MeshElementType::Tri6:
        return MeshElementType::Triangle;
    case MeshElementType::Quad8:
    case MeshElementType::Quad9:
        return MeshElementType::Quad;
    default:
        return type;
    }
}

} // namespace OpenGeoLab::Mesh
