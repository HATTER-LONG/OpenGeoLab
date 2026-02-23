/**
 * @file mesh_types.hpp
 * @brief Fundamental mesh type definitions and identifier types
 */

#pragma once

#include "util/core_identity.hpp"

#include <cstdint>
#include <string>
#include <unordered_set>
namespace OpenGeoLab::Mesh {

/**
 * @brief Enumerates supported finite-element topologies.
 *
 * @note The enumerator values are grouped by spatial dimension (1D, 2D, 3D).
 *       The node count implied by each type must agree with
 *       MeshElement::nodeCountFromType.
 */
enum class MeshElementType : uint8_t {
    Invalid = 0,

    // -------- 1D --------
    Line, // 2-node line

    // -------- 2D --------
    Triangle, // 3-node triangle
    Quad4,    // 4-node quadrilateral

    // -------- 3D --------
    Tetra4,   // 4-node tetrahedron
    Hexa8,    // 8-node brick
    Prism6,   // 6-node prism
    Pyramid5, // 5-node pyramid
};

/**
 * @brief Convert a MeshElementType enumerator to its human-readable string form.
 * @param type The element type to convert.
 * @return A non-empty string such as "Triangle" or "Hexa8".
 *         Returns "Invalid" for MeshElementType::Invalid.
 */
[[nodiscard]] std::string meshElementTypeToString(MeshElementType type);

/**
 * @brief Parse a string into the corresponding MeshElementType.
 * @param str Case-sensitive name (e.g. "Triangle", "Hexa8").
 * @return The matching enumerator, or MeshElementType::Invalid if @p str
 *         does not match any known type name.
 */
[[nodiscard]] MeshElementType meshElementTypeFromString(std::string_view str);

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

/**
 * @brief Produce the next globally-unique MeshElementId.
 * @return A monotonically increasing element identifier (never zero).
 * @note Thread-safety depends on the implementation; see the .cpp file.
 */
[[nodiscard]] MeshElementId generateMeshElementId();

/**
 * @brief Produce the next type-scoped MeshElementUID for @p type.
 * @param type Element topology whose UID counter is advanced.
 * @return A UID unique among elements of the same @p type (never zero).
 * @warning Passing MeshElementType::Invalid is undefined behaviour.
 */
[[nodiscard]] MeshElementUID generateMeshElementUID(MeshElementType type);

/**
 * @brief Reset the global MeshElementId counter to its initial state.
 * @warning Existing elements will still hold their old IDs;
 *          future calls to generateMeshElementId may produce duplicates.
 */
void resetMeshElementIdGenerator();

/**
 * @brief Reset the type-scoped UID counter for @p type.
 * @param type The element type whose counter is reset.
 * @warning Same caveat as resetMeshElementIdGenerator regarding duplicates.
 */
void resetMeshElementUIDGenerator(MeshElementType type);

/**
 * @brief Reset every per-type UID counter at once.
 * @warning Same caveat as resetMeshElementIdGenerator regarding duplicates.
 */
void resetAllMeshElementUIDGenerators();

/**
 * @brief Return the current value of the global MeshElementId counter.
 * @return The ID that was most recently issued, or 0 if none have been generated.
 */
uint64_t getCurrentMeshElementIdCounter();

/**
 * @brief Return the current value of the UID counter for @p type.
 * @param type The element type to query.
 * @return The UID that was most recently issued for @p type, or 0 if none.
 */
uint64_t getCurrentMeshElementUIDCounter(MeshElementType type);

/**
 * @brief Produce the next globally-unique MeshNodeId.
 * @return A monotonically increasing node identifier (never zero).
 */
[[nodiscard]] MeshNodeId generateMeshNodeId();

/**
 * @brief Reset the global MeshNodeId counter to its initial state.
 * @warning Existing nodes will still hold their old IDs;
 *          future calls to generateMeshNodeId may produce duplicates.
 */
void resetMeshNodeIdGenerator();

/**
 * @brief Return the current value of the global MeshNodeId counter.
 * @return The ID that was most recently issued, or 0 if none have been generated.
 */
uint64_t getCurrentMeshNodeIdCounter();

// =============================================================================
// MeshElementKey (id+uid+type)
// =============================================================================

/**
 * @brief Full identity key for a mesh element, combining global ID,
 *        type-scoped UID, and element type.
 *
 * @note Implicitly convertible to MeshElementRef (drops the global ID).
 */
using MeshElementKey = Util::CoreIdentity<MeshElementId,
                                          MeshElementUID,
                                          MeshElementType,
                                          INVALID_MESH_ELEMENT_ID,
                                          INVALID_MESH_ELEMENT_UID,
                                          MeshElementType::Invalid>;

/// @brief Hash functor for MeshElementKey, suitable for unordered containers.
using MeshElementKeyHash = Util::CoreIdentityHash<MeshElementKey>;
/// @brief Unordered set of MeshElementKey values.
using MeshElementKeySet = std::unordered_set<MeshElementKey, MeshElementKeyHash>;
/// @brief Unordered map keyed by MeshElementKey.
template <typename T>
using MeshElementKeyMap = std::unordered_map<MeshElementKey, T, MeshElementKeyHash>;

// =============================================================================
// MeshElementRef (uid+type only)
// =============================================================================

/**
 * @brief Lightweight reference to a mesh element using only UID and type
 *        (no global ID).
 *
 * @note Useful when the caller only needs to identify an element within its
 *       type-scoped namespace rather than globally.
 */
using MeshElementRef = Util::CoreUidIdentity<MeshElementUID,
                                             MeshElementType,
                                             INVALID_MESH_ELEMENT_UID,
                                             MeshElementType::Invalid>;
/// @brief Hash functor for MeshElementRef, suitable for unordered containers.
using MeshElementRefHash = Util::CoreUidIdentityHash<MeshElementRef>;
/// @brief Unordered set of MeshElementRef values.
using MeshElementRefSet = std::unordered_set<MeshElementRef, MeshElementRefHash>;
/// @brief Unordered map keyed by MeshElementRef.
template <typename T>
using MeshElementRefMap = std::unordered_map<MeshElementRef, T, MeshElementRefHash>;

} // namespace OpenGeoLab::Mesh