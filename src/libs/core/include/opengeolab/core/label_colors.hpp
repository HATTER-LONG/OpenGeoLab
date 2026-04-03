/**
 * @file label_colors.hpp
 * @brief Shared label color constants for entity-type-based label coloring.
 *
 * Used by LabelPass (render), describe_labels action (scene), and
 * SelectionService (app) to maintain consistent visual encoding.
 */

#pragma once

#include <opengeolab/core/entity_tag.hpp>

#include <glm/vec4.hpp>

#include <string>
#include <string_view>

namespace OpenGeoLab::Core {

/// Label text color per entity type.
inline constexpr glm::vec4 labelColor(EntityType type) noexcept {
    switch(type) {
    case EntityType::GeoVertex:
        return {0.91F, 0.33F, 0.33F, 1.0F}; // #E85454 Red
    case EntityType::GeoEdge:
        return {0.29F, 0.56F, 0.85F, 1.0F}; // #4A90D9 Blue
    case EntityType::GeoFace:
        return {0.36F, 0.72F, 0.36F, 1.0F}; // #5CB85C Green
    case EntityType::GeoSolid:
        return {0.91F, 0.65F, 0.33F, 1.0F}; // #E8A654 Orange
    case EntityType::MeshNode:
        return {0.95F, 0.61F, 0.07F, 1.0F}; // #F29C12 Amber
    case EntityType::MeshEdge:
        return {0.61F, 0.35F, 0.71F, 1.0F}; // #9C59B6 Purple
    case EntityType::MeshElement:
        return {0.17F, 0.64F, 0.79F, 1.0F}; // #2BA3C9 Cyan
    default:
        return {0.8F, 0.8F, 0.8F, 1.0F}; // Gray fallback
    }
}

/// Hex color string per entity type (for JSON describe output).
inline constexpr std::string_view labelColorHex(EntityType type) noexcept {
    switch(type) {
    case EntityType::GeoVertex:
        return "#E85454";
    case EntityType::GeoEdge:
        return "#4A90D9";
    case EntityType::GeoFace:
        return "#5CB85C";
    case EntityType::GeoSolid:
        return "#E8A654";
    case EntityType::MeshNode:
        return "#F29C12";
    case EntityType::MeshEdge:
        return "#9C59B6";
    case EntityType::MeshElement:
        return "#2BA3C9";
    default:
        return "#CCCCCC";
    }
}

/// Label text prefix per entity type.
inline constexpr std::string_view labelPrefix(EntityType type) noexcept {
    switch(type) {
    case EntityType::GeoVertex:
        return "V";
    case EntityType::GeoEdge:
        return "E";
    case EntityType::GeoWire:
        return "W";
    case EntityType::GeoFace:
        return "F";
    case EntityType::GeoSolid:
        return "S";
    case EntityType::MeshNode:
        return "N";
    case EntityType::MeshEdge:
        return "L";
    case EntityType::MeshElement:
        return "Elem";
    default:
        return "?";
    }
}

/// Build display text for a label: "[shapeId]Prefix:localId".
[[nodiscard]] inline std::string
formatLabelText(uint32_t shape_id, EntityType type, uint32_t local_id) {
    return "[" + std::to_string(shape_id) + "]" + std::string(labelPrefix(type)) + ":" +
           std::to_string(local_id);
}

/// Default label background color (dark semi-transparent).
inline constexpr glm::vec4 K_LABEL_BG_COLOR{0.1F, 0.1F, 0.12F, 0.85F};

/// Alpha multiplier for occluded labels.
inline constexpr float K_LABEL_OCCLUDED_ALPHA = 0.3F;

} // namespace OpenGeoLab::Core
