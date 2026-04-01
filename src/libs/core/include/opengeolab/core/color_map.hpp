/**
 * @file color_map.hpp
 * @brief 15-color cyclic palette for shape rendering, matching the OGL reference.
 */

#pragma once

#include <opengeolab/core/core_export.hpp>

#include <array>
#include <cstdint>
#include <string>

namespace OpenGeoLab::Core {

/** @brief RGBA float color. */
struct RenderColor {
    float r{};
    float g{};
    float b{};
    float a{1.0F};
};

/** @brief Number of distinct shape face colors. */
inline constexpr std::size_t K_PART_PALETTE_SIZE = 15;

/** @brief 15-color cyclic palette for shape faces (from OGL reference). */
inline constexpr std::array<RenderColor, K_PART_PALETTE_SIZE> K_PART_COLOR_PALETTE{{
    {0.671F, 0.929F, 0.851F, 1.0F}, // #abedd8
    {0.275F, 0.804F, 0.812F, 1.0F}, // #46cdcf
    {0.722F, 0.537F, 0.325F, 1.0F}, // #b88953
    {0.698F, 0.875F, 0.541F, 1.0F}, // #b2df8a
    {0.200F, 0.627F, 0.173F, 1.0F}, // #33a02c
    {0.122F, 0.471F, 0.706F, 1.0F}, // #1f78b4
    {0.651F, 0.808F, 0.890F, 1.0F}, // #a6cee3
    {0.984F, 0.604F, 0.600F, 1.0F}, // #fb9a99
    {0.800F, 0.659F, 0.914F, 1.0F}, // #cca8e9
    {0.596F, 0.306F, 0.639F, 1.0F}, // #984ea3
    {1.000F, 1.000F, 0.204F, 1.0F}, // #ffff33
    {0.216F, 0.494F, 0.722F, 1.0F}, // #377eb8
    {0.302F, 0.686F, 0.290F, 1.0F}, // #4daf4a
    {0.969F, 0.506F, 0.749F, 1.0F}, // #f781bf
    {0.455F, 0.263F, 0.263F, 1.0F}, // #744343
}};

/** @brief Golden-yellow edge color (#ffd460). */
inline constexpr RenderColor K_EDGE_COLOR{1.0F, 0.831F, 0.376F, 1.0F};

/** @brief Cornflower-blue vertex color (#3490de). */
inline constexpr RenderColor K_VERTEX_COLOR{0.204F, 0.565F, 0.871F, 1.0F};

/**
 * @brief Get the palette color for a given shape id.
 * @param shapeId Shape identifier (cycled modulo palette size).
 */
inline const RenderColor& colorForShapeId(uint32_t shape_id) {
    return K_PART_COLOR_PALETTE[shape_id % K_PART_PALETTE_SIZE];
}

/**
 * @brief Convert a RenderColor to a hex string (#rrggbb).
 */
inline std::string colorToHex(const RenderColor& c) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", static_cast<int>(c.r * 255.0F + 0.5F),
                  static_cast<int>(c.g * 255.0F + 0.5F), static_cast<int>(c.b * 255.0F + 0.5F));
    return buf;
}

/// Visual style for a highlighted entity category.
struct HighlightStyle {
    RenderColor color;
    float lineWidth{1.5F};  ///< Edge rendering width (pixels).
    float pointScale{1.0F}; ///< Vertex point-size multiplier relative to defaultPointSize.
};

/// Complete color/style configuration for hover and selection states.
struct ColorMapConfig {
    HighlightStyle hoverEdgeVertex;     ///< Edge & Vertex hover.
    HighlightStyle hoverFace;           ///< Face hover.
    HighlightStyle selectionEdgeVertex; ///< Edge & Vertex selection.
    HighlightStyle selectionFace;       ///< Face selection.
    RenderColor defaultEdge;            ///< Default edge color.
    RenderColor defaultVertex;          ///< Default vertex color.
    float defaultEdgeWidth{1.5F};       ///< Default edge line width.
    float defaultPointSize{6.0F};       ///< Default vertex point size.
};

/// Access and override the active color map configuration.
namespace ColorMap {

/// Compile-time default configuration matching OGL reference palette.
inline constexpr ColorMapConfig kDefault{
    // hoverEdgeVertex: orange #ff7f00
    {.color = {1.F, 0.498F, 0.F, 1.F}, .lineWidth = 2.5F, .pointScale = 1.5F},
    // hoverFace: blue #4b55e9, alpha 0.6
    {.color = {0.294F, 0.333F, 0.914F, 0.6F}, .lineWidth = 1.5F, .pointScale = 1.F},
    // selectionEdgeVertex: red-pink #ff165d
    {.color = {1.F, 0.086F, 0.365F, 1.F}, .lineWidth = 2.0F, .pointScale = 1.2F},
    // selectionFace: deep blue #4116ff, alpha 0.6
    {.color = {0.255F, 0.086F, 1.F, 0.6F}, .lineWidth = 1.5F, .pointScale = 1.F},
    // defaultEdge: #ffd460
    {1.F, 0.831F, 0.376F, 1.F},
    // defaultVertex: #3490de
    {0.204F, 0.565F, 0.871F, 1.F},
    // defaultEdgeWidth, defaultPointSize
    1.5F,
    6.0F,
};

/// Returns the currently active configuration (thread-safe read).
OPENGEOLAB_CORE_EXPORT const ColorMapConfig& active();

/// Overrides the active configuration at runtime.
/// Pass @c kDefault to reset to defaults.
OPENGEOLAB_CORE_EXPORT void setOverride(const ColorMapConfig& config);

} // namespace ColorMap
} // namespace OpenGeoLab::Core
