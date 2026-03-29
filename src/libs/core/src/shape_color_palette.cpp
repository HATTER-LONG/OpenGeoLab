/**
 * @file shape_color_palette.cpp
 * @brief shapeColorHex implementation
 */

#include <opengeolab/core/shape_color_palette.hpp>

#include <cmath>
#include <cstdio>

namespace OpenGeoLab::Core {

std::string shapeColorHex(uint32_t shape_id) {
    const auto& c = kShapeColorPalette[shape_id % kShapeColorPaletteSize];
    auto to_byte = [](float value) -> int { return static_cast<int>(std::lround(value * 255.f)); };

    char buffer[8]; // "#RRGGBB\0"
    std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", to_byte(c[0]), to_byte(c[1]),
                  to_byte(c[2]));
    return buffer;
}

} // namespace OpenGeoLab::Core
