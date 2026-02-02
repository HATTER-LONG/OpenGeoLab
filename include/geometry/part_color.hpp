/**
 * @file part_color.hpp
 * @brief Part color management for visual differentiation
 *
 * Provides a predefined palette of visually distinct colors for parts.
 * Colors are designed to be aesthetically pleasing and easily distinguishable.
 */

#pragma once

#include "geometry/geometry_types.hpp"

#include <array>
#include <string>

namespace OpenGeoLab::Geometry {

/**
 * @brief RGBA color representation with float components
 */
struct PartColor {
    float r{0.7f}; ///< Red component [0.0, 1.0] NOLINT
    float g{0.7f}; ///< Green component [0.0, 1.0] NOLINT
    float b{0.7f}; ///< Blue component [0.0, 1.0] NOLINT
    float a{1.0f}; ///< Alpha component [0.0, 1.0] NOLINT

    PartColor() = default;
    constexpr PartColor(float red, float green, float blue, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}

    /**
     * @brief Convert to hex string format "#RRGGBB"
     * @return Hex color string
     */
    [[nodiscard]] std::string toHex() const;

    /**
     * @brief Create from hex string "#RRGGBB" or "#RRGGBBAA"
     * @param hex Hex color string
     * @return PartColor instance
     */
    [[nodiscard]] static PartColor fromHex(const std::string& hex);
};

/**
 * @brief Color palette manager for part visualization
 *
 * Provides a carefully curated set of colors that are:
 * - Visually distinct from each other
 * - Aesthetically pleasing (not pure primary colors)
 * - Suitable for 3D CAD visualization
 */
class PartColorPalette {
public:
    /**
     * @brief Get color for a part by index
     * @param index Part index (cycles through palette)
     * @return Color for the part
     */
    [[nodiscard]] static PartColor getColor(size_t index);

    /**
     * @brief Get color for a part by entity ID
     * @param entity_id Entity ID to derive color from
     * @return Consistent color for the entity
     */
    [[nodiscard]] static PartColor getColorByEntityId(EntityId entity_id);

    /**
     * @brief Get the number of colors in the palette
     * @return Palette size
     */
    [[nodiscard]] static size_t paletteSize();

    /**
     * @brief Get all colors in the palette
     * @return Array of all palette colors
     */
    [[nodiscard]] static const std::array<PartColor, 16>& palette();
};

} // namespace OpenGeoLab::Geometry
