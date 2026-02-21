#pragma once

#include "util/core_identity.hpp"

#include <cstdint>
#include <string>
#include <unordered_set>
namespace OpenGeoLab::Mesh {
// =============================================================================
// Mesh Entity Type (domain-level: Node vs Element)
// =============================================================================

/**
 * @brief Mesh entity type for domain-level identification
 *
 * Used in the picking and selection system to distinguish mesh nodes from
 * mesh elements. Separate from Geometry::EntityType to maintain clean
 * domain separation.
 */
enum class EntityType : uint8_t {
    Invalid = 0,
    Node = 1,   ///< Mesh node (point in FEM mesh)
    Element = 2 ///< Mesh element (triangle, quad, etc.)
};

[[nodiscard]] std::string meshEntityTypeToString(EntityType type);
[[nodiscard]] EntityType meshEntityTypeFromString(std::string_view str);

// =============================================================================
// Mesh Element Type Definitions
// =============================================================================
enum class MeshElementType : uint8_t {
    Invalid = 0,

    // -------- 1D --------
    Line, // 2-node line

    // -------- 2D --------
    Triangle, // 3-node triangle
    Quad4,    // 4-node quadrilateral

    // -------- 3D --------
    Tetra4, // 4-node tetrahedron
    Hexa8,  // 8-node brick
    Prism6, // 6-node prism
};

[[nodiscard]] std::string meshElementTypeToString(MeshElementType type);
[[nodiscard]] MeshElementType meshElementTypeFromString(std::string_view str);

// =============================================================================
// ID System
// =============================================================================
// Global identifier for any mesh node
using MeshNodeId = uint64_t;

/// Global unique identifier for any mesh element
using MeshElementId = uint64_t;

/// Type-scoped unique identifier within the same mesh element type
using MeshElementUID = uint64_t;

/// Invalid/null MeshNodeId constant
constexpr MeshNodeId INVALID_MESH_NODE_ID = 0;

/// Invalid/null MeshElementId constant
constexpr MeshElementId INVALID_MESH_ELEMENT_ID = 0;

/// Invalid/null MeshElementUID constant
constexpr MeshElementUID INVALID_MESH_ELEMENT_UID = 0;

[[nodiscard]] MeshElementId generateMeshElementId();

[[nodiscard]] MeshElementUID generateMeshElementUID(MeshElementType type);

void resetMeshElementIdGenerator();

void resetMeshElementUIDGenerator(MeshElementType type);

void resetAllMeshElementUIDGenerators();

uint64_t getCurrentMeshElementIdCounter();

uint64_t getCurrentMeshElementUIDCounter(MeshElementType type);

[[nodiscard]] MeshNodeId generateMeshNodeId();

void resetMeshNodeIdGenerator();

uint64_t getCurrentMeshNodeIdCounter();

// =============================================================================
// MeshElementKey (id+uid+type)
// =============================================================================

using MeshElementKey = Util::CoreIdentity<MeshElementId,
                                          MeshElementUID,
                                          MeshElementType,
                                          INVALID_MESH_ELEMENT_ID,
                                          INVALID_MESH_ELEMENT_UID,
                                          MeshElementType::Invalid>;

using MeshElementKeyHash = Util::CoreIdentityHash<MeshElementKey>;
using MeshElementKeySet = std::unordered_set<MeshElementKey, MeshElementKeyHash>;
template <typename T>
using MeshElementKeyMap = std::unordered_map<MeshElementKey, T, MeshElementKeyHash>;

// =============================================================================
// MeshElementRef (uid+type only)
// =============================================================================

using MeshElementRef = Util::CoreUidIdentity<MeshElementUID,
                                             MeshElementType,
                                             INVALID_MESH_ELEMENT_UID,
                                             MeshElementType::Invalid>;
using MeshElementRefHash = Util::CoreUidIdentityHash<MeshElementRef>;
using MeshElementRefSet = std::unordered_set<MeshElementRef, MeshElementRefHash>;
template <typename T>
using MeshElementRefMap = std::unordered_map<MeshElementRef, T, MeshElementRefHash>;

} // namespace OpenGeoLab::Mesh