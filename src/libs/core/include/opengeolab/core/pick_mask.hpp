/**
 * @file pick_mask.hpp
 * @brief PickMode and PickMask enumerations for GPU pick filtering
 */

#pragma once

#include <opengeolab/core/entity_tag.hpp>

#include <cstdint>

namespace OpenGeoLab::Core {

enum class PickMode : uint8_t {
    VEF,   /**< Vertex > Edge > Face priority */
    Wire,  /**< Edge → resolve to Wire */
    Solid, /**< Face → resolve to Solid */
    Part,  /**< Any → resolve to Part (shapeId) */
};

enum class PickMask : uint32_t {
    None = 0,
    Vertex = 1 << 0,
    Edge = 1 << 1,
    Wire = 1 << 2,
    Face = 1 << 3,
    Solid = 1 << 4,
    Part = 1 << 5,
    All = 0xFFFFFFFF,
};

constexpr PickMask operator|(PickMask a, PickMask b) {
    return static_cast<PickMask>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr PickMask operator&(PickMask a, PickMask b) {
    return static_cast<PickMask>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/// Map an EntityType to the corresponding single PickMask bit.
constexpr PickMask maskForEntityType(EntityType type) noexcept {
    switch(type) {
    case EntityType::GeoVertex:
    case EntityType::MeshNode:
        return PickMask::Vertex;
    case EntityType::GeoEdge:
    case EntityType::MeshEdge:
        return PickMask::Edge;
    case EntityType::GeoWire:
        return PickMask::Wire;
    case EntityType::GeoFace:
    case EntityType::MeshElement:
        return PickMask::Face;
    case EntityType::GeoSolid:
        return PickMask::Solid;
    default:
        return PickMask::None;
    }
}

} // namespace OpenGeoLab::Core
