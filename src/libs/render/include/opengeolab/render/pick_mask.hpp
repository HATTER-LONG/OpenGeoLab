/**
 * @file pick_mask.hpp
 * @brief PickMode and PickMask enumerations for GPU pick filtering
 */

#pragma once

#include <cstdint>

namespace OpenGeoLab::Render {

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

} // namespace OpenGeoLab::Render
