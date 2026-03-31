/**
 * @file display_mode.hpp
 * @brief Display mode and display mode mask enumerations
 */

#pragma once

#include <cstdint>

namespace OpenGeoLab::Scene {

/** @brief How a scene node should be rendered. */
enum class DisplayMode : uint8_t {
    Solid = 0x01,          /**< Filled triangles only */
    Wireframe = 0x02,      /**< Edges only */
    SolidWithEdges = 0x03, /**< Filled triangles with edge overlay */
    Points = 0x04,         /**< Point cloud */
};

/** @brief Bitmask for selecting which render passes draw a node. */
enum class DisplayModeMask : uint8_t {
    None = 0,
    Surface = 1 << 0,   /**< Draw filled surfaces */
    Wireframe = 1 << 1, /**< Draw wireframe edges */
    Points = 1 << 2,    /**< Draw point markers */
};

/** @brief Bitwise OR for DisplayModeMask. */
constexpr DisplayModeMask operator|(DisplayModeMask a, DisplayModeMask b) {
    return static_cast<DisplayModeMask>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

/** @brief Bitwise AND for DisplayModeMask. */
constexpr DisplayModeMask operator&(DisplayModeMask a, DisplayModeMask b) {
    return static_cast<DisplayModeMask>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

/** @brief Bitwise NOT for DisplayModeMask. */
constexpr DisplayModeMask operator~(DisplayModeMask a) {
    return static_cast<DisplayModeMask>(~static_cast<uint8_t>(a));
}

} // namespace OpenGeoLab::Scene
