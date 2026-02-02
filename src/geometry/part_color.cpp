/**
 * @file part_color.cpp
 * @brief Implementation of part color palette management
 */

#include "geometry/part_color.hpp"

#include <iomanip>
#include <sstream>

namespace OpenGeoLab::Geometry {

namespace {
/**
 * @brief Predefined color palette for parts
 *
 * Colors are chosen to be:
 * - Visually distinct and harmonious
 * - Not too saturated (avoiding pure RGB)
 * - Suitable for 3D CAD applications
 */
constexpr std::array<PartColor, 16> K_PART_COLOR_PALETTE = {{
    {0.529f, 0.808f, 0.922f, 1.0f}, // Sky Blue
    {0.941f, 0.502f, 0.502f, 1.0f}, // Light Coral
    {0.565f, 0.933f, 0.565f, 1.0f}, // Light Green
    {0.804f, 0.698f, 0.878f, 1.0f}, // Light Purple
    {0.980f, 0.804f, 0.502f, 1.0f}, // Moccasin (Warm Yellow)
    {0.686f, 0.933f, 0.933f, 1.0f}, // Pale Turquoise
    {0.957f, 0.643f, 0.376f, 1.0f}, // Sandy Brown
    {0.690f, 0.878f, 0.902f, 1.0f}, // Light Cyan
    {0.867f, 0.627f, 0.867f, 1.0f}, // Plum
    {0.596f, 0.984f, 0.596f, 1.0f}, // Pale Green
    {0.933f, 0.510f, 0.933f, 1.0f}, // Violet
    {0.855f, 0.647f, 0.125f, 1.0f}, // Goldenrod
    {0.529f, 0.808f, 0.980f, 1.0f}, // Light Sky Blue
    {0.863f, 0.078f, 0.235f, 0.7f}, // Crimson (semi-transparent)
    {0.000f, 0.502f, 0.502f, 1.0f}, // Teal
    {0.722f, 0.525f, 0.043f, 1.0f}, // Dark Goldenrod
}};
} // namespace

std::string PartColor::toHex() const {
    std::ostringstream oss;
    oss << "#" << std::hex << std::setfill('0') << std::setw(2)
        << static_cast<int>(r * 255.0f + 0.5f) << std::setw(2)
        << static_cast<int>(g * 255.0f + 0.5f) << std::setw(2)
        << static_cast<int>(b * 255.0f + 0.5f);
    return oss.str();
}

PartColor PartColor::fromHex(const std::string& hex) {
    PartColor color;
    if(hex.empty() || hex[0] != '#') {
        return color;
    }

    unsigned int rgb = 0;
    std::istringstream iss(hex.substr(1));
    iss >> std::hex >> rgb;

    if(hex.length() >= 7) {
        color.r = static_cast<float>((rgb >> 16) & 0xFF) / 255.0f;
        color.g = static_cast<float>((rgb >> 8) & 0xFF) / 255.0f;
        color.b = static_cast<float>(rgb & 0xFF) / 255.0f;
    }
    if(hex.length() >= 9) {
        color.a = static_cast<float>(rgb & 0xFF) / 255.0f;
        color.b = static_cast<float>((rgb >> 8) & 0xFF) / 255.0f;
        color.g = static_cast<float>((rgb >> 16) & 0xFF) / 255.0f;
        color.r = static_cast<float>((rgb >> 24) & 0xFF) / 255.0f;
    }

    return color;
}

PartColor PartColorPalette::getColor(size_t index) {
    return K_PART_COLOR_PALETTE[index % K_PART_COLOR_PALETTE.size()];
}

PartColor PartColorPalette::getColorByEntityId(EntityId entity_id) {
    // Use entity ID to consistently select a color
    return getColor(static_cast<size_t>(entity_id - 1));
}

size_t PartColorPalette::paletteSize() { return K_PART_COLOR_PALETTE.size(); }

const std::array<PartColor, 16>& PartColorPalette::palette() { return K_PART_COLOR_PALETTE; }

} // namespace OpenGeoLab::Geometry
