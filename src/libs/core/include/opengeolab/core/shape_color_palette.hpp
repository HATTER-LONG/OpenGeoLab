/**
 * @file shape_color_palette.hpp
 * @brief Shared 15-colour palette for shape face colouring
 */

#pragma once

#include <opengeolab/core/core_export.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace OpenGeoLab::Core {

/// 15-colour face palette — index by `shape_id % kShapeColorPaletteSize`.
constexpr std::array<std::array<float, 4>, 15> kShapeColorPalette = {{
    {0.671f, 0.929f, 0.847f, 1.f}, // #ABEDD8 Mint
    {0.275f, 0.804f, 0.812f, 1.f}, // #46CDCF Cyan
    {0.722f, 0.537f, 0.325f, 1.f}, // #B88953 Brown
    {0.698f, 0.875f, 0.541f, 1.f}, // #B2DF8A Light Green
    {0.200f, 0.627f, 0.173f, 1.f}, // #33A02C Dark Green
    {0.122f, 0.471f, 0.706f, 1.f}, // #1F78B4 Blue
    {0.651f, 0.808f, 0.890f, 1.f}, // #A6CEE3 Light Blue
    {0.984f, 0.604f, 0.600f, 1.f}, // #FB9A99 Pink
    {0.800f, 0.659f, 0.914f, 1.f}, // #CCA8E9 Lavender
    {0.596f, 0.306f, 0.639f, 1.f}, // #984EA3 Purple
    {1.000f, 1.000f, 0.200f, 1.f}, // #FFFF33 Yellow
    {0.216f, 0.494f, 0.722f, 1.f}, // #377EB8 Steel Blue
    {0.302f, 0.686f, 0.290f, 1.f}, // #4DAF4A Forest Green
    {0.969f, 0.506f, 0.749f, 1.f}, // #F781BF Hot Pink
    {0.455f, 0.263f, 0.263f, 1.f}, // #744343 Dark Brown
}};

/// Total number of palette entries.
constexpr std::size_t kShapeColorPaletteSize = kShapeColorPalette.size();

/// Return hex colour string for a shape ID (e.g. "#ABEDD8").
/// Wraps `kShapeColorPalette[shape_id % kShapeColorPaletteSize]`.
[[nodiscard]] OPENGEOLAB_CORE_EXPORT std::string shapeColorHex(uint32_t shape_id);

} // namespace OpenGeoLab::Core
