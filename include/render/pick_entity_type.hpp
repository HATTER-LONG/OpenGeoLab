#pragma once

#include "geometry/geometry_types.hpp"
#include "mesh/mesh_types.hpp"

#include <cstdint>
#include <optional>

namespace OpenGeoLab::Render {

enum class PickEntityType : uint8_t {
    // Geometry domain (values match Geometry::EntityType)
    Vertex = 0,
    Edge = 1,
    Wire = 2,
    Face = 3,
    Shell = 4,
    Solid = 5,
    CompSolid = 6,
    Compound = 7,
    Part = 8,

    // Mesh domain
    MeshNode = 9,
    MeshLine = 10,
    MeshElement = 11,

    // Default/invalid type
    None = 12,
};

enum class PickMask : uint32_t {
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
    MeshElement = 1 << 11,

    None = 1 << 12,
};

constexpr PickMask toMask(PickEntityType type) {
    return static_cast<PickMask>(1 << static_cast<uint8_t>(type));
}

constexpr PickEntityType toPickEntityType(Geometry::EntityType t) {
    if(t == Geometry::EntityType::None) {
        return PickEntityType::None;
    }
    return static_cast<PickEntityType>(static_cast<uint8_t>(t));
}

constexpr PickEntityType toPickEntityType(Mesh::EntityType t) {
    switch(t) {
    case Mesh::EntityType::Node:
        return PickEntityType::MeshNode;
    case Mesh::EntityType::Element:
        return PickEntityType::MeshElement;
    default:
        break;
    }
    return PickEntityType::None;
}

constexpr PickMask operator|(PickMask a, PickMask b) {
    return static_cast<PickMask>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr PickMask operator&(PickMask a, PickMask b) {
    return static_cast<PickMask>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// =============================================================================
// Domain conversion
// =============================================================================

/**
 * @brief Convert Geometry::EntityType to PickEntityType
 * @note Values are identical by design, so this is a simple cast.
 */
[[nodiscard]] constexpr PickEntityType toPickType(Geometry::EntityType t) noexcept {
    return static_cast<PickEntityType>(t);
}

/**
 * @brief Convert Mesh::EntityType to PickEntityType
 */
[[nodiscard]] constexpr PickEntityType toPickType(Mesh::EntityType t) noexcept {
    switch(t) {
    case Mesh::EntityType::Node:
        return PickEntityType::MeshNode;
    case Mesh::EntityType::Element:
        return PickEntityType::MeshElement;
    default:
        return PickEntityType::None;
    }
}

/**
 * @brief Try to convert PickEntityType to Geometry::EntityType
 * @return The geometry type, or std::nullopt if it's a mesh type
 */
[[nodiscard]] constexpr std::optional<Geometry::EntityType>
toGeometryType(PickEntityType t) noexcept {
    const auto val = static_cast<uint8_t>(t);
    if(val >= 0 && val <= 8) {
        return static_cast<Geometry::EntityType>(val);
    }
    return std::nullopt;
}

/**
 * @brief Try to convert PickEntityType to Mesh::EntityType
 * @return The mesh type, or std::nullopt if it's a geometry type
 */
[[nodiscard]] constexpr std::optional<Mesh::EntityType> toMeshType(PickEntityType t) noexcept {
    switch(t) {
    case PickEntityType::MeshNode:
        return Mesh::EntityType::Node;
    case PickEntityType::MeshLine:
    case PickEntityType::MeshElement:
        return Mesh::EntityType::Element;
    default:
        return std::nullopt;
    }
}

/**
 * @brief Check if the type belongs to the Geometry domain
 */
[[nodiscard]] constexpr bool isGeometryDomain(PickEntityType t) noexcept {
    const auto val = static_cast<uint8_t>(t);
    return val >= 0 && val <= 8;
}

/**
 * @brief Check if the type belongs to the Mesh domain
 */
[[nodiscard]] constexpr bool isMeshDomain(PickEntityType t) noexcept {
    return t == PickEntityType::MeshNode || t == PickEntityType::MeshLine ||
           t == PickEntityType::MeshElement;
}

} // namespace OpenGeoLab::Render
