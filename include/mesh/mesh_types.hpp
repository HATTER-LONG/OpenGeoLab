#pragma once

#include <cstdint>
#include <string>
namespace OpenGeoLab::Mesh {

// =============================================================================
// Mesh Element Type Definitions
// =============================================================================

enum class MeshElementType : uint8_t {
    Invalid = 0,

    // -------- 1D --------
    Node, // mesh node
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

/// Global unique identifier for any mesh element
using MeshElementId = uint64_t;

/// Type-scoped unique identifier within the same mesh element type
using MeshElementUID = uint64_t;

/// Invalid/null MeshElementId constant
constexpr MeshElementId INVALID_MESH_ELEMENT_ID = 0;

/// Invalid/null MeshElementUID constant
constexpr MeshElementUID INVALID_MESH_ELEMENT_UID = 0;

// =============================================================================
// Mesh Node Definition
// =============================================================================

struct MeshNode {
    MeshElementId m_id{INVALID_MESH_ELEMENT_ID};
    MeshElementUID m_uid{INVALID_MESH_ELEMENT_UID};
};
} // namespace OpenGeoLab::Mesh