/**
 * @file mesh_types.hpp
 * @brief Core mesh types and ID system for OpenGeoLab
 *
 * Defines fundamental mesh data types (MeshNode, MeshElement) and the
 * dual ID system (ElementId / ElementUID + MeshEntityType) used
 * throughout the mesh layer.
 */

#pragma once

#include "geometry/geometry_types.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace OpenGeoLab::Mesh {

// =============================================================================
// Mesh Entity Type Definitions
// =============================================================================

/**
 * @brief Enumeration of mesh entity types
 */
enum class MeshEntityType : uint8_t {
    None = 0,     ///< Invalid / unspecified
    Node = 1,     ///< Mesh node (point)
    Edge = 2,     ///< Mesh edge element (2-node line)
    Triangle = 3, ///< Triangle element (3-node)
    Quad = 4,     ///< Quadrilateral element (4-node)
};

/**
 * @brief Convert MeshEntityType to human-readable string
 * @param type Mesh entity type
 * @return String representation
 */
[[nodiscard]] inline std::string meshEntityTypeToString(MeshEntityType type) {
    switch(type) {
    case MeshEntityType::Node:
        return "MeshNode";
    case MeshEntityType::Edge:
        return "MeshEdge";
    case MeshEntityType::Triangle:
        return "MeshTriangle";
    case MeshEntityType::Quad:
        return "MeshQuad";
    default:
        return "MeshNone";
    }
}

/**
 * @brief Convert string to MeshEntityType
 * @param value String representation
 * @return Corresponding MeshEntityType
 */
[[nodiscard]] inline MeshEntityType meshEntityTypeFromString(std::string_view value) {
    if(value == "MeshNode")
        return MeshEntityType::Node;
    if(value == "MeshEdge")
        return MeshEntityType::Edge;
    if(value == "MeshTriangle")
        return MeshEntityType::Triangle;
    if(value == "MeshQuad")
        return MeshEntityType::Quad;
    return MeshEntityType::None;
}

// =============================================================================
// ID System
// =============================================================================

/// Global unique identifier for any mesh entity
using MeshElementId = uint64_t;

/// Type-scoped unique identifier within the same mesh entity type
using MeshElementUID = uint64_t;

/// Invalid/null MeshElementId constant
constexpr MeshElementId INVALID_MESH_ELEMENT_ID = 0;

/// Invalid/null MeshElementUID constant
constexpr MeshElementUID INVALID_MESH_ELEMENT_UID = 0;

// =============================================================================
// MeshElementKey
// =============================================================================

/**
 * @brief Comparable, hashable handle for a mesh entity
 *
 * Combines MeshElementId, MeshElementUID, and MeshEntityType into a
 * single key for indexing and lookups.
 */
struct MeshElementKey {
    MeshElementId m_id{INVALID_MESH_ELEMENT_ID};
    MeshElementUID m_uid{INVALID_MESH_ELEMENT_UID};
    MeshEntityType m_type{MeshEntityType::None};

    MeshElementKey() = default;
    MeshElementKey(MeshElementId id, MeshElementUID uid, MeshEntityType type)
        : m_id(id), m_uid(uid), m_type(type) {}

    [[nodiscard]] bool isValid() const noexcept {
        return m_id != INVALID_MESH_ELEMENT_ID && m_uid != INVALID_MESH_ELEMENT_UID &&
               m_type != MeshEntityType::None;
    }

    [[nodiscard]] bool operator==(const MeshElementKey& other) const noexcept {
        return m_id == other.m_id && m_uid == other.m_uid && m_type == other.m_type;
    }

    [[nodiscard]] bool operator!=(const MeshElementKey& other) const noexcept {
        return !(*this == other);
    }

    bool operator<(const MeshElementKey& other) const noexcept {
        if(m_id != other.m_id)
            return m_id < other.m_id;
        if(m_type != other.m_type)
            return static_cast<uint8_t>(m_type) < static_cast<uint8_t>(other.m_type);
        return m_uid < other.m_uid;
    }
};

/**
 * @brief Hash functor for MeshElementKey
 */
struct MeshElementKeyHash {
    static void hashCombine(size_t& seed, size_t value) {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }

    size_t operator()(const MeshElementKey& key) const noexcept {
        size_t seed = 0;
        hashCombine(seed, std::hash<MeshElementId>{}(key.m_id));
        hashCombine(seed, std::hash<MeshElementUID>{}(key.m_uid));
        using U = std::underlying_type_t<MeshEntityType>;
        hashCombine(seed, std::hash<U>{}(static_cast<U>(key.m_type)));
        return seed;
    }
};

using MeshElementKeySet = std::unordered_set<MeshElementKey, MeshElementKeyHash>;

template <typename T>
using MeshElementKeyMap = std::unordered_map<MeshElementKey, T, MeshElementKeyHash>;

// =============================================================================
// MeshNode
// =============================================================================

/**
 * @brief A single mesh node (vertex) with 3D coordinates
 */
struct MeshNode {
    MeshElementId m_id{INVALID_MESH_ELEMENT_ID};    ///< Global unique ID
    MeshElementUID m_uid{INVALID_MESH_ELEMENT_UID}; ///< Type-scoped UID
    Geometry::Point3D m_position;                   ///< Node coordinates

    /// Geometry entity this node is associated with (may be invalid)
    Geometry::EntityKey m_geometryEntity;

    MeshNode() = default;

    MeshNode(MeshElementId id, MeshElementUID uid, const Geometry::Point3D& pos)
        : m_id(id), m_uid(uid), m_position(pos) {}

    [[nodiscard]] bool isValid() const noexcept {
        return m_id != INVALID_MESH_ELEMENT_ID && m_uid != INVALID_MESH_ELEMENT_UID;
    }

    [[nodiscard]] MeshElementKey key() const {
        return MeshElementKey(m_id, m_uid, MeshEntityType::Node);
    }
};

// =============================================================================
// MeshElement
// =============================================================================

/**
 * @brief A single mesh element (edge, triangle, quad)
 *
 * Stores connectivity (node UIDs) and a reference to the source geometry entity.
 */
struct MeshElement {
    MeshElementId m_id{INVALID_MESH_ELEMENT_ID};    ///< Global unique ID
    MeshElementUID m_uid{INVALID_MESH_ELEMENT_UID}; ///< Type-scoped UID
    MeshEntityType m_type{MeshEntityType::None};    ///< Element type

    std::vector<MeshElementUID> m_nodeUids; ///< Connectivity: node UIDs

    /// Geometry entity this element is associated with (e.g. the Face)
    Geometry::EntityKey m_geometryEntity;

    MeshElement() = default;

    MeshElement(MeshElementId id,
                MeshElementUID uid,
                MeshEntityType type,
                std::vector<MeshElementUID> node_uids)
        : m_id(id), m_uid(uid), m_type(type), m_nodeUids(std::move(node_uids)) {}

    [[nodiscard]] bool isValid() const noexcept {
        return m_id != INVALID_MESH_ELEMENT_ID && m_uid != INVALID_MESH_ELEMENT_UID &&
               m_type != MeshEntityType::None && !m_nodeUids.empty();
    }

    [[nodiscard]] MeshElementKey key() const { return MeshElementKey(m_id, m_uid, m_type); }

    [[nodiscard]] size_t nodeCount() const { return m_nodeUids.size(); }
};

// =============================================================================
// MeshEntityRef
// =============================================================================

/**
 * @brief Lightweight reference to a mesh entity (uid + type)
 */
struct MeshEntityRef {
    MeshElementUID m_uid{INVALID_MESH_ELEMENT_UID};
    MeshEntityType m_type{MeshEntityType::None};

    constexpr MeshEntityRef() = default;
    constexpr MeshEntityRef(MeshElementUID uid, MeshEntityType type) : m_uid(uid), m_type(type) {}

    [[nodiscard]] constexpr bool isValid() const noexcept {
        return m_uid != INVALID_MESH_ELEMENT_UID && m_type != MeshEntityType::None;
    }

    [[nodiscard]] constexpr bool operator==(const MeshEntityRef& other) const noexcept {
        return m_uid == other.m_uid && m_type == other.m_type;
    }
    [[nodiscard]] constexpr bool operator!=(const MeshEntityRef& other) const noexcept {
        return !(*this == other);
    }
};

struct MeshEntityRefHash {
    size_t operator()(const MeshEntityRef& ref) const noexcept {
        size_t seed = 0;
        auto combine = [](size_t& s, size_t v) {
            s ^= v + 0x9e3779b97f4a7c15ULL + (s << 6) + (s >> 2);
        };
        combine(seed, std::hash<MeshElementUID>{}(ref.m_uid));
        using U = std::underlying_type_t<MeshEntityType>;
        combine(seed, std::hash<U>{}(static_cast<U>(ref.m_type)));
        return seed;
    }
};

using MeshEntityRefSet = std::unordered_set<MeshEntityRef, MeshEntityRefHash>;

} // namespace OpenGeoLab::Mesh
