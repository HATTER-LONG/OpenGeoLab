/**
 * @file pick_entity_type.hpp
 * @brief Unified entity type for GPU picking across Geometry and Mesh domains
 *
 * PickEntityType provides a single enum that spans both the Geometry domain
 * (Vertex, Edge, Face, etc.) and the Mesh domain (MeshNode, MeshElement).
 * This is the type used in the GPU pick buffer encoding, SelectManager,
 * and all render-layer entity identification.
 *
 * Domain-specific entity types (Geometry::EntityType, Mesh::EntityType)
 * are used in their respective domain layers. Conversion functions are
 * provided to map between domain types and PickEntityType.
 */

#pragma once

#include "geometry/geometry_types.hpp"
#include "mesh/mesh_types.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace OpenGeoLab::Render {

/**
 * @brief Unified entity type for GPU picking and selection
 *
 * Numeric values for geometry types match Geometry::EntityType values
 * for backward compatibility with existing pick buffer encoding.
 */
enum class PickEntityType : uint8_t {
    None = 0,

    // Geometry domain (values match Geometry::EntityType)
    Vertex = 1,
    Edge = 2,
    Wire = 3,
    Face = 4,
    Shell = 5,
    Solid = 6,
    CompSolid = 7,
    Compound = 8,
    Part = 9,

    // Mesh domain
    MeshNode = 10,
    MeshElement = 11
};

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
    if(val >= 1 && val <= 9) {
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
    return val >= 1 && val <= 9;
}

/**
 * @brief Check if the type belongs to the Mesh domain
 */
[[nodiscard]] constexpr bool isMeshDomain(PickEntityType t) noexcept {
    return t == PickEntityType::MeshNode || t == PickEntityType::MeshElement;
}

/**
 * @brief Convert PickEntityType to human-readable string
 */
[[nodiscard]] std::string pickEntityTypeToString(PickEntityType t);

/**
 * @brief Convert string to PickEntityType
 * @throws std::invalid_argument if the string is not recognized
 */
[[nodiscard]] PickEntityType pickEntityTypeFromString(std::string_view str);

} // namespace OpenGeoLab::Render
