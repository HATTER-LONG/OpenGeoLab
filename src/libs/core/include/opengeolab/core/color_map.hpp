/**
 * @file color_map.hpp
 * @brief 15-color cyclic palette for shape rendering, matching the OGL reference.
 */

#pragma once

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

} // namespace OpenGeoLab::Core
