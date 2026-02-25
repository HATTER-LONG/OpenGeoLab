/**
 * @file render_scene_impl_internal.hpp
 * @brief Shared internal helpers for RenderSceneImpl split modules
 */

#pragma once

#include "render/render_data.hpp"
#include "render/render_select_manager.hpp"
#include "render/render_types.hpp"

#include <QOpenGLFunctions>

#include <cstdint>
#include <utility>

namespace OpenGeoLab::Render::Detail {

struct VertexPC {
    float m_x;
    float m_y;
    float m_z;
    float m_r;
    float m_g;
    float m_b;
    float m_a;
    uint32_t m_pickLow;
    uint32_t m_pickHighType;
};

[[nodiscard]] constexpr bool isPickableType(RenderEntityType type, RenderEntityTypeMask mask) {
    if(type == RenderEntityType::None) {
        return false;
    }
    return static_cast<uint32_t>(toMask(type) & mask) != 0u;
}

[[nodiscard]] constexpr uint64_t toPackedUid56(uint64_t uid) { return uid & 0x00FFFFFFFFFFFFFFULL; }

[[nodiscard]] constexpr std::pair<uint32_t, uint32_t> packUidType(uint64_t uid,
                                                                  RenderEntityType type) {
    const uint64_t uid56 = toPackedUid56(uid);
    const auto low = static_cast<uint32_t>(uid56 & 0xFFFFFFFFULL);
    const auto high24 = static_cast<uint32_t>((uid56 >> 32) & 0x00FFFFFFULL);
    const auto type8 = static_cast<uint32_t>(static_cast<uint8_t>(type));
    return {low, (type8 << 24) | high24};
}

[[nodiscard]] constexpr PickResult unpackPick(uint32_t low, uint32_t high_type) {
    const uint64_t uid =
        static_cast<uint64_t>(low) | (static_cast<uint64_t>(high_type & 0x00FFFFFFU) << 32);
    const auto type = static_cast<RenderEntityType>((high_type >> 24) & 0xFFU);
    return PickResult{uid, type};
}

[[nodiscard]] inline bool isModeVisible(RenderDisplayMode mode, PrimitiveTopology topology) {
    switch(mode) {
    case RenderDisplayMode::Surface:
        return topology == PrimitiveTopology::Triangles || topology == PrimitiveTopology::Lines ||
               topology == PrimitiveTopology::Points;
    case RenderDisplayMode::Wireframe:
        return topology == PrimitiveTopology::Lines;
    case RenderDisplayMode::Points:
        return topology == PrimitiveTopology::Points;
    default:
        return true;
    }
}

[[nodiscard]] inline GLenum toGlPrimitive(PrimitiveTopology topology) {
    switch(topology) {
    case PrimitiveTopology::Points:
        return GL_POINTS;
    case PrimitiveTopology::Lines:
        return GL_LINES;
    case PrimitiveTopology::Triangles:
        return GL_TRIANGLES;
    default:
        return GL_TRIANGLES;
    }
}

} // namespace OpenGeoLab::Render::Detail
