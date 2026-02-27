/**
 * @file mesh_types.hpp
 * @brief Mesh type definitions, element type enumeration, and ID generation utilities.
 */

#pragma once

#include "util/core_identity.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
namespace OpenGeoLab::Mesh {

/// @brief FEM element topology type.
enum class MeshElementType : uint8_t {
    Node = 0, ///< Mesh node (0D)

    // -------- 1D --------
    Line = 1, ///< 2-node line

    // -------- 2D --------
    Triangle = 2, ///< 3-node triangle
    Quad4 = 3,    ///< 4-node quadrilateral

    // -------- 3D --------
    Tetra4 = 4,   ///< 4-node tetrahedron
    Hexa8 = 5,    ///< 8-node brick
    Prism6 = 6,   ///< 6-node prism
    Pyramid5 = 7, ///< 5-node pyramid

    None = 255 ///< Invalid/unset
};

[[nodiscard]] std::optional<std::string> meshElementTypeToString(MeshElementType type) noexcept;
[[nodiscard]] std::optional<MeshElementType>
meshElementTypeFromString(std::string_view str) noexcept;

// =============================================================================
// ID System
// =============================================================================

/// Global identifier for any mesh node
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

// =============================================================================
// Mesh Node id unordered set
// =============================================================================
using MeshNodeIdSet = std::unordered_set<MeshNodeId>;

// =============================================================================
// MeshElementKey (id+uid+type)
// =============================================================================

using MeshElementKey = Util::CoreIdentity<MeshElementId,
                                          MeshElementUID,
                                          MeshElementType,
                                          INVALID_MESH_ELEMENT_ID,
                                          INVALID_MESH_ELEMENT_UID,
                                          MeshElementType::None>;

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
                                             MeshElementType::None>;
using MeshElementRefHash = Util::CoreUidIdentityHash<MeshElementRef>;
using MeshElementRefSet = std::unordered_set<MeshElementRef, MeshElementRefHash>;
template <typename T>
using MeshElementRefMap = std::unordered_map<MeshElementRef, T, MeshElementRefHash>;

} // namespace OpenGeoLab::Mesh