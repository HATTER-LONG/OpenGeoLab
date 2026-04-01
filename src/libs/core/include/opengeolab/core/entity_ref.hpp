/**
 * @file entity_ref.hpp
 * @brief EntityRef — scene-wide absolute entity address
 *
 * Combines a top-level shapeId with an EntityTag to form a globally
 * unique entity address usable across all layers.
 */

#pragma once

#include <opengeolab/core/entity_tag.hpp>

#include <compare>
#include <cstdint>
#include <functional>

namespace OpenGeoLab::Core {

/**
 * @brief Scene-wide absolute entity address.
 *
 * Extends EntityTag (shape-scoped) with a shapeId to create a globally
 * unique identifier. Zero-cost conversion to/from PickId encoding.
 */
struct EntityRef {
    uint32_t shapeId{};      ///< Owning shape/part ID
    EntityType entityType{}; ///< Entity classification
    uint32_t localId{};      ///< Type-scoped local ID within the shape

    /// True when this ref points to a real entity.
    [[nodiscard]] constexpr bool isValid() const noexcept { return shapeId != 0 && localId != 0; }

    /// True for geometry entities (GeoVertex..GeoSolid).
    [[nodiscard]] constexpr bool isGeometry() const noexcept {
        return entityType >= EntityType::GeoVertex && entityType <= EntityType::GeoSolid;
    }

    /// True for mesh entities (MeshNode..MeshElement).
    [[nodiscard]] constexpr bool isMesh() const noexcept {
        return entityType >= EntityType::MeshNode && entityType <= EntityType::MeshElement;
    }

    /// Extract the shape-scoped EntityTag.
    [[nodiscard]] constexpr EntityTag tag() const noexcept { return {entityType, localId}; }

    bool operator==(const EntityRef&) const = default;
    auto operator<=>(const EntityRef&) const = default;
};

} // namespace OpenGeoLab::Core

/// Hash specialization for use in unordered containers.
template <> struct std::hash<OpenGeoLab::Core::EntityRef> {
    std::size_t operator()(const OpenGeoLab::Core::EntityRef& ref) const noexcept {
        auto h1 = std::hash<uint32_t>{}(ref.shapeId);
        auto h2 = std::hash<uint8_t>{}(static_cast<uint8_t>(ref.entityType));
        auto h3 = std::hash<uint32_t>{}(ref.localId);
        h1 ^= h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
        h1 ^= h3 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
        return h1;
    }
};
