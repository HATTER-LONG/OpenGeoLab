/// @file mesh_types.hpp
/// @brief Core mesh data types: ElementType, MeshNodeArray, ElementBlock, GeoEntityRef.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace OpenGeoLab::Mesh {

/// @brief Mesh element type, values correspond to Gmsh element type IDs.
enum class ElementType : uint8_t {
    Line2 = 1,      ///< 2-node line segment
    Triangle3 = 2,  ///< 3-node triangle
    Quad4 = 3,      ///< 4-node quadrilateral
    Tetra4 = 4,     ///< 4-node tetrahedron
    Hexa8 = 5,      ///< 8-node hexahedron
    Prism6 = 6,     ///< 6-node prism (wedge)
    Pyramid5 = 7,   ///< 5-node pyramid
    Line3 = 8,      ///< 3-node quadratic line
    Triangle6 = 9,  ///< 6-node quadratic triangle
    Quad9 = 10,     ///< 9-node quadratic quadrilateral
    Tetra10 = 11,   ///< 10-node quadratic tetrahedron
    Hexa27 = 12,    ///< 27-node quadratic hexahedron
    Prism18 = 13,   ///< 18-node quadratic prism
    Pyramid14 = 14, ///< 14-node quadratic pyramid
};

/// @brief Returns the number of nodes per element for the given type.
[[nodiscard]] constexpr uint32_t nodesPerElement(ElementType type) {
    switch(type) {
    case ElementType::Line2:
        return 2;
    case ElementType::Triangle3:
        return 3;
    case ElementType::Quad4:
        return 4;
    case ElementType::Tetra4:
        return 4;
    case ElementType::Hexa8:
        return 8;
    case ElementType::Prism6:
        return 6;
    case ElementType::Pyramid5:
        return 5;
    case ElementType::Line3:
        return 3;
    case ElementType::Triangle6:
        return 6;
    case ElementType::Quad9:
        return 9;
    case ElementType::Tetra10:
        return 10;
    case ElementType::Hexa27:
        return 27;
    case ElementType::Prism18:
        return 18;
    case ElementType::Pyramid14:
        return 14;
    }
    return 0;
}

/// @brief Returns the topological dimension (1=line, 2=surface, 3=volume).
[[nodiscard]] constexpr int elementDimension(ElementType type) {
    switch(type) {
    case ElementType::Line2:
    case ElementType::Line3:
        return 1;
    case ElementType::Triangle3:
    case ElementType::Quad4:
    case ElementType::Triangle6:
    case ElementType::Quad9:
        return 2;
    case ElementType::Tetra4:
    case ElementType::Hexa8:
    case ElementType::Prism6:
    case ElementType::Pyramid5:
    case ElementType::Tetra10:
    case ElementType::Hexa27:
    case ElementType::Prism18:
    case ElementType::Pyramid14:
        return 3;
    }
    return 0;
}

/// @brief Compact node coordinate array (double precision, interleaved xyz).
struct MeshNodeArray {
    std::vector<double> coords; ///< [x0,y0,z0, x1,y1,z1, ...] — 3*N doubles

    /// @brief Returns the number of nodes.
    [[nodiscard]] size_t count() const { return coords.size() / 3; }

    /// @brief Returns the xyz coordinates for a 1-based node ID.
    [[nodiscard]] std::array<double, 3> position(uint32_t node_id) const {
        const auto idx = static_cast<size_t>(node_id - 1) * 3;
        return {coords[idx], coords[idx + 1], coords[idx + 2]};
    }
};

/// @brief Reference to the source geometric entity (face/solid) in the original shape.
struct GeoEntityRef {
    int dimension = 0;          ///< Geometric dimension (0=vertex, 1=curve, 2=surface, 3=volume)
    int gmshTag = 0;            ///< Gmsh geometric entity tag
    uint32_t sourceLocalId = 0; ///< 1-based index into ShapeEntry's faceMap/solidMap
};

/// @brief A contiguous block of same-type elements.
struct ElementBlock {
    ElementType type{};                 ///< Element type for this block
    std::vector<uint32_t> connectivity; ///< Flat node IDs (1-based): [n0,n1,..., n3,n4,...]
    GeoEntityRef geoEntity;             ///< Source geometric entity reference

    /// @brief Returns the number of nodes per element.
    [[nodiscard]] uint32_t nodesPerElem() const { return nodesPerElement(type); }

    /// @brief Returns the number of elements in this block.
    [[nodiscard]] size_t elementCount() const { return connectivity.size() / nodesPerElem(); }
};

} // namespace OpenGeoLab::Mesh
