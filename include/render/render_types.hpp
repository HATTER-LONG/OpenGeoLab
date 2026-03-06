/**
 * @file render_types.hpp
 * @brief Shared render-layer enumerations and type-mapping utilities.
 */

#pragma once

#include "geometry/geometry_types.hpp"
#include "mesh/mesh_types.hpp"

namespace OpenGeoLab::Render {

/**
 * @brief Identifies the kind of renderable entity (geometry or mesh domain).
 */
enum class RenderEntityType : uint8_t {
    Vertex = 0,    ///< BRep vertex (point)
    Edge = 1,      ///< BRep edge (curve segment)
    Wire = 2,      ///< BRep wire (closed loop of edges)
    Face = 3,      ///< BRep face (surface patch)
    Shell = 4,     ///< BRep shell (connected set of faces)
    Solid = 5,     ///< BRep solid (enclosed volume)
    CompSolid = 6, ///< BRep composite solid
    Compound = 7,  ///< BRep compound shape
    Part = 8,      ///< Top-level part (root geometry entity)

    MeshNode = 9,      ///< FEM mesh node (point)
    MeshLine = 10,     ///< FEM mesh line (edge between two nodes)
    MeshTriangle = 11, ///< FEM triangle element (3-node)
    MeshQuad4 = 12,    ///< FEM quadrilateral element (4-node)
    MeshTetra4 = 13,   ///< FEM tetrahedron element (4-node)
    MeshHexa8 = 14,    ///< FEM hexahedron element (8-node)
    MeshPrism6 = 15,   ///< FEM prism element (6-node)
    MeshPyramid5 = 16, ///< FEM pyramid element (5-node)

    None = 17, ///< Sentinel / invalid type
};
/**
 * @brief Bitmask for filtering sets of RenderEntityType values.
 */
enum class RenderEntityTypeMask : uint32_t {
    None = 0,

    Vertex = 1 << 0,
    Edge = 1 << 1,
    Wire = 1 << 2,
    Face = 1 << 3,
    Shell = 1 << 4,
    Solid = 1 << 5,
    CompSolid = 1 << 6,
    Compound = 1 << 7,
    Part = 1 << 8,

    MeshNode = 1 << 9,
    MeshLine = 1 << 10,
    MeshTriangle = 1 << 11,
    MeshQuad4 = 1 << 12,
    MeshTetra4 = 1 << 13,
    MeshHexa8 = 1 << 14,
    MeshPrism6 = 1 << 15,
    MeshPyramid5 = 1 << 16,
};

constexpr RenderEntityTypeMask operator|(RenderEntityTypeMask a, RenderEntityTypeMask b) {
    return static_cast<RenderEntityTypeMask>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr RenderEntityTypeMask operator&(RenderEntityTypeMask a, RenderEntityTypeMask b) {
    return static_cast<RenderEntityTypeMask>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/// @brief Combined mask for all 2D/3D mesh element types (excludes MeshNode and MeshLine).
constexpr auto RENDER_MESH_ELEMENTS =
    RenderEntityTypeMask::MeshTriangle | RenderEntityTypeMask::MeshQuad4 |
    RenderEntityTypeMask::MeshTetra4 | RenderEntityTypeMask::MeshHexa8 |
    RenderEntityTypeMask::MeshPrism6 | RenderEntityTypeMask::MeshPyramid5;

/** @brief Convert a single RenderEntityType to its corresponding bitmask. */
constexpr RenderEntityTypeMask toMask(RenderEntityType type) {
    return static_cast<RenderEntityTypeMask>(1 << static_cast<uint8_t>(type));
}

/** @brief Map a Geometry::EntityType to its RenderEntityType equivalent. */
constexpr RenderEntityType toRenderEntityType(Geometry::EntityType t) {
    if(t == Geometry::EntityType::None) {
        return RenderEntityType::None;
    }
    return static_cast<RenderEntityType>(static_cast<uint8_t>(t));
}

/** @brief Map a Mesh::MeshElementType to its RenderEntityType equivalent. */
constexpr RenderEntityType toRenderEntityType(Mesh::MeshElementType t) {
    if(t == Mesh::MeshElementType::None) {
        return RenderEntityType::None;
    }
    return static_cast<RenderEntityType>(static_cast<uint8_t>(t) + 9);
}

/** @brief True if the type belongs to the CAD geometry domain (Vertex..Part). */
constexpr bool isGeometryDomain(RenderEntityType t) noexcept {
    const auto val = static_cast<uint8_t>(t);
    return val <= 8;
}

/** @brief True if the type belongs to the mesh domain (MeshNode..MeshPyramid5). */
constexpr bool isMeshDomain(RenderEntityType t) noexcept {
    const auto val = static_cast<uint8_t>(t);
    return val >= 9 && val <= 16;
}

/** @brief Convert a geometry-domain RenderEntityType back to Geometry::EntityType. */
constexpr Geometry::EntityType toGeometryType(RenderEntityType t) {
    if(isGeometryDomain(t)) {
        return static_cast<Geometry::EntityType>(static_cast<uint8_t>(t));
    }
    return Geometry::EntityType::None;
}

/** @brief Convert a mesh-domain RenderEntityType back to Mesh::MeshElementType. */
constexpr Mesh::MeshElementType toMeshElementType(RenderEntityType t) {
    if(isMeshDomain(t)) {
        return static_cast<Mesh::MeshElementType>(static_cast<uint8_t>(t) - 9);
    }
    return Mesh::MeshElementType::None;
}

/**
 * @brief Action to perform on a pick event.
 */
enum class PickAction : uint8_t {
    None = 0,   ///< No action
    Add = 1,    ///< Add entity to selection
    Remove = 2, ///< Remove entity from selection
};
} // namespace OpenGeoLab::Render