/**
 * @file render_types.hpp
 * @brief Render-layer entity type system and unified identifier
 *
 * Defines RenderEntityType (8-bit) and RenderUID (32-bit packed identifier)
 * as the canonical identity types for the render pipeline.
 *
 * Encoding: bits [31..8] = uid (24 bits), bits [7..0] = type (8 bits)
 *
 * Geometry types occupy range 1-15, mesh types occupy 16+.
 * Conversion functions bridge between domain types (Geometry::EntityType)
 * and render types.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace OpenGeoLab::Render {

// =============================================================================
// RenderEntityType
// =============================================================================

/**
 * @brief Entity type enumeration for the render pipeline
 *
 * Geometry types (1-15) mirror Geometry::EntityType values for direct casting.
 * Mesh types (16+) are in a separate range to avoid mixing concerns.
 */
enum class RenderEntityType : uint8_t {
    None = 0,
    // Geometry domain (1-15)
    Vertex = 1,
    Edge = 2,
    Wire = 3,
    Face = 4,
    Shell = 5,
    Solid = 6,
    CompSolid = 7,
    Compound = 8,
    Part = 9,
    // Reserved 10-15 for future geometry types
    // Mesh domain (16+)
    MeshNode = 16,
    MeshElement = 17,
};

/**
 * @brief Convert RenderEntityType to string for debug/logging
 */
[[nodiscard]] inline const char* renderEntityTypeToString(RenderEntityType type) {
    switch(type) {
    case RenderEntityType::None:
        return "None";
    case RenderEntityType::Vertex:
        return "Vertex";
    case RenderEntityType::Edge:
        return "Edge";
    case RenderEntityType::Wire:
        return "Wire";
    case RenderEntityType::Face:
        return "Face";
    case RenderEntityType::Shell:
        return "Shell";
    case RenderEntityType::Solid:
        return "Solid";
    case RenderEntityType::CompSolid:
        return "CompSolid";
    case RenderEntityType::Compound:
        return "Compound";
    case RenderEntityType::Part:
        return "Part";
    case RenderEntityType::MeshNode:
        return "MeshNode";
    case RenderEntityType::MeshElement:
        return "MeshElement";
    default:
        return "Unknown";
    }
}

/**
 * @brief Convert string to RenderEntityType (inverse of renderEntityTypeToString)
 * @param s String representation (e.g. "Face", "Edge", "MeshNode")
 * @return Corresponding RenderEntityType, or RenderEntityType::None if unrecognized
 */
[[nodiscard]] inline RenderEntityType renderEntityTypeFromString(std::string_view s) {
    if(s == "Vertex")
        return RenderEntityType::Vertex;
    if(s == "Edge")
        return RenderEntityType::Edge;
    if(s == "Wire")
        return RenderEntityType::Wire;
    if(s == "Face")
        return RenderEntityType::Face;
    if(s == "Shell")
        return RenderEntityType::Shell;
    if(s == "Solid")
        return RenderEntityType::Solid;
    if(s == "CompSolid")
        return RenderEntityType::CompSolid;
    if(s == "Compound")
        return RenderEntityType::Compound;
    if(s == "Part")
        return RenderEntityType::Part;
    if(s == "MeshNode")
        return RenderEntityType::MeshNode;
    if(s == "MeshElement")
        return RenderEntityType::MeshElement;
    return RenderEntityType::None;
}

/**
 * @brief Check if a render entity type is in the geometry range (1-15)
 */
[[nodiscard]] constexpr bool isGeometryType(RenderEntityType type) {
    const auto v = static_cast<uint8_t>(type);
    return v >= 1 && v <= 15;
}

/**
 * @brief Check if a render entity type is in the mesh range (16+)
 */
[[nodiscard]] constexpr bool isMeshType(RenderEntityType type) {
    return type == RenderEntityType::MeshNode || type == RenderEntityType::MeshElement;
}

// =============================================================================
// RenderUID
// =============================================================================

/**
 * @brief 64-bit packed identifier: 56-bit uid + 8-bit type
 *
 * Layout: bits [63..8] = uid (56 bits, max 72,057,594,037,927,935)
 *         bits [7..0]  = type (8 bits, RenderEntityType)
 *
 * Used throughout the render pipeline for picking, selection, and entity
 * identification. Uses RG32UI FBO for GPU-side 64-bit picking.
 */
struct RenderUID {
    uint64_t m_packed{0};

    constexpr RenderUID() = default;
    constexpr explicit RenderUID(uint64_t packed) : m_packed(packed) {}

    /**
     * @brief Encode a type and uid into a packed RenderUID
     * @param type Entity type (stored in low 8 bits)
     * @param uid56 56-bit unique identifier (stored in high 56 bits)
     */
    [[nodiscard]] static constexpr RenderUID encode(RenderEntityType type, uint64_t uid56) {
        return RenderUID{((uid56 & 0x00FFFFFFFFFFFFFFu) << 8) |
                         (static_cast<uint64_t>(type) & 0xFFu)};
    }

    /**
     * @brief Extract the entity type from the packed value
     */
    [[nodiscard]] constexpr RenderEntityType type() const {
        return static_cast<RenderEntityType>(m_packed & 0xFFu);
    }

    /**
     * @brief Extract the 56-bit uid from the packed value
     */
    [[nodiscard]] constexpr uint64_t uid56() const { return (m_packed >> 8) & 0x00FFFFFFFFFFFFFFu; }

    /**
     * @brief Check if this UID is valid (non-zero)
     */
    [[nodiscard]] constexpr bool isValid() const { return m_packed != 0; }

    constexpr bool operator==(RenderUID other) const { return m_packed == other.m_packed; }
    constexpr bool operator!=(RenderUID other) const { return m_packed != other.m_packed; }
    constexpr bool operator<(RenderUID other) const { return m_packed < other.m_packed; }

    struct Hash {
        std::size_t operator()(RenderUID uid) const noexcept {
            return std::hash<uint64_t>{}(uid.m_packed);
        }
    };
};

/// Map keyed by RenderUID
template <typename T> using RenderUIDMap = std::unordered_map<uint64_t, T>;

/// Set of RenderUIDs
using RenderUIDSet = std::unordered_set<uint64_t>;

// =============================================================================
// RenderEntityInfo
// =============================================================================

/**
 * @brief Per-entity metadata for selection, highlighting, and sub-draw
 *
 * Stores the index/vertex range of an entity within a batched mesh,
 * ownership hierarchy, and visual properties for hover/selection.
 */
struct RenderEntityInfo {
    RenderUID m_uid; ///< Packed entity identifier

    uint64_t m_owningPartUid56{0};            ///< 56-bit owning part uid (0 = none)
    uint64_t m_owningSolidUid56{0};           ///< 56-bit owning solid uid (0 = none)
    std::vector<uint64_t> m_owningWireUid56s; ///< 56-bit owning wire uids

    uint32_t m_indexOffset{0};  ///< Start position in batched index array
    uint32_t m_indexCount{0};   ///< Number of indices for this entity
    uint32_t m_vertexOffset{0}; ///< Start position in batched vertex array
    uint32_t m_vertexCount{0};  ///< Number of vertices for this entity

    float m_hoverColor[4]{1.0f, 1.0f, 0.0f, 1.0f};    ///< RGBA hover highlight color
    float m_selectedColor[4]{1.0f, 0.5f, 0.0f, 1.0f}; ///< RGBA selected highlight color

    float m_centroid[3]{0.0f, 0.0f, 0.0f}; ///< Geometric centroid for outline scaling

    [[nodiscard]] bool isValid() const { return m_uid.isValid(); }
};

/// Entity info map keyed by RenderUID::m_packed
using RenderEntityInfoMap = std::unordered_map<uint64_t, RenderEntityInfo>;

// =============================================================================
// Conversions
// =============================================================================

/**
 * @brief Convert a Geometry::EntityType (uint8_t) to RenderEntityType.
 *
 * Geometry types 0-9 map directly by design (None through Part).
 * Mesh types (MeshNode=16, MeshElement=17) exist only in RenderEntityType.
 */
[[nodiscard]] constexpr RenderEntityType toRenderEntityType(uint8_t geometry_entity_type) {
    return static_cast<RenderEntityType>(geometry_entity_type);
}

/**
 * @brief Convert RenderEntityType back to Geometry-layer uint8_t.
 *
 * Only valid for geometry types (0-9). Returns the underlying value.
 */
[[nodiscard]] constexpr uint8_t toGeometryEntityTypeValue(RenderEntityType type) {
    return static_cast<uint8_t>(type);
}

} // namespace OpenGeoLab::Render
