/**
 * @file render_color_manager.cpp
 * @brief Implementation of global render color manager
 */

#include "render/render_color_manager.hpp"

#include <algorithm>
#include <array>

namespace OpenGeoLab::Render {
namespace {
constexpr std::array<RenderColor, 16> K_BASE_PART_PALETTE = {{
    {0.529f, 0.808f, 0.922f, 1.0f},
    {0.941f, 0.502f, 0.502f, 1.0f},
    {0.565f, 0.933f, 0.565f, 1.0f},
    {0.804f, 0.698f, 0.878f, 1.0f},
    {0.980f, 0.804f, 0.502f, 1.0f},
    {0.686f, 0.933f, 0.933f, 1.0f},
    {0.957f, 0.643f, 0.376f, 1.0f},
    {0.690f, 0.878f, 0.902f, 1.0f},
    {0.867f, 0.627f, 0.867f, 1.0f},
    {0.596f, 0.984f, 0.596f, 1.0f},
    {0.933f, 0.510f, 0.933f, 1.0f},
    {0.855f, 0.647f, 0.125f, 1.0f},
    {0.529f, 0.808f, 0.980f, 1.0f},
    {0.863f, 0.278f, 0.435f, 1.0f},
    {0.000f, 0.702f, 0.702f, 1.0f},
    {0.722f, 0.525f, 0.243f, 1.0f},
}};

[[nodiscard]] constexpr float clamp01(float v) {
    return (v < 0.0f) ? 0.0f : ((v > 1.0f) ? 1.0f : v);
}

[[nodiscard]] RenderColor mix(const RenderColor& a, const RenderColor& b, float t) {
    const float s = clamp01(t);
    return RenderColor{a.m_r + (b.m_r - a.m_r) * s, a.m_g + (b.m_g - a.m_g) * s,
                       a.m_b + (b.m_b - a.m_b) * s, a.m_a + (b.m_a - a.m_a) * s};
}

[[nodiscard]] uint64_t safePartKey(uint64_t part_id, uint64_t fallback_uid) {
    if(part_id != 0) {
        return part_id;
    }
    return (fallback_uid == 0) ? 1 : fallback_uid;
}

[[nodiscard]] RenderColorRole roleFromEntityType(RenderEntityType entity_type) {
    switch(entity_type) {
    case RenderEntityType::Edge:
    case RenderEntityType::Wire:
        return RenderColorRole::Edge;
    case RenderEntityType::Vertex:
        return RenderColorRole::Vertex;
    case RenderEntityType::MeshNode:
        return RenderColorRole::MeshNode;
    case RenderEntityType::MeshLine:
        return RenderColorRole::MeshLine;
    case RenderEntityType::MeshTriangle:
    case RenderEntityType::MeshQuad4:
    case RenderEntityType::MeshTetra4:
    case RenderEntityType::MeshHexa8:
    case RenderEntityType::MeshPrism6:
    case RenderEntityType::MeshPyramid5:
        return RenderColorRole::MeshElement;
    default:
        return RenderColorRole::Face;
    }
}

[[nodiscard]] size_t roleOffset(RenderColorRole role) {
    switch(role) {
    case RenderColorRole::Face:
        return 0;
    case RenderColorRole::Edge:
        return 5;
    case RenderColorRole::Vertex:
        return 9;
    case RenderColorRole::MeshNode:
        return 3;
    case RenderColorRole::MeshLine:
        return 7;
    case RenderColorRole::MeshElement:
        return 11;
    default:
        return 0;
    }
}

[[nodiscard]] RenderColor roleAdjust(RenderColorRole role, const RenderColor& base) {
    switch(role) {
    case RenderColorRole::Face:
        return base;
    case RenderColorRole::Edge: {
        auto edge = mix(base, RenderColor{1.0f, 1.0f, 0.78f, 1.0f}, 0.55f);
        edge.m_r = std::max(edge.m_r, 0.72f);
        edge.m_g = std::max(edge.m_g, 0.72f);
        edge.m_b = std::max(edge.m_b, 0.72f);
        return edge;
    }
    case RenderColorRole::Vertex:
        return mix(base, RenderColor{1.0f, 0.55f, 0.12f, 1.0f}, 0.6f);
    case RenderColorRole::MeshNode:
        return mix(base, RenderColor{0.95f, 0.85f, 0.20f, 1.0f}, 0.55f);
    case RenderColorRole::MeshLine:
        return mix(base, RenderColor{0.36f, 0.86f, 1.0f, 1.0f}, 0.5f);
    case RenderColorRole::MeshElement:
        return mix(base, RenderColor{0.25f, 0.66f, 1.0f, 1.0f}, 0.4f);
    default:
        return base;
    }
}

[[nodiscard]] RenderColor applyState(RenderColor color, RenderColorState state) {
    switch(state) {
    case RenderColorState::Normal:
        return color;
    case RenderColorState::Hover:
        return mix(color, RenderColor{1.0f, 1.0f, 1.0f, color.m_a}, 0.25f);
    case RenderColorState::Selected:
        return mix(color, RenderColor{1.0f, 0.92f, 0.40f, color.m_a}, 0.45f);
    default:
        return color;
    }
}
} // namespace

RenderColor
RenderColorManager::colorForPart(uint64_t part_id, RenderColorRole role, RenderColorState state) {
    const size_t palette_size = K_BASE_PART_PALETTE.size();
    const size_t base_index = static_cast<size_t>((part_id == 0 ? 1 : part_id) % palette_size);
    const size_t index = (base_index + roleOffset(role)) % palette_size;
    const RenderColor role_color = roleAdjust(role, K_BASE_PART_PALETTE[index]);
    return applyState(role_color, state);
}

RenderColor RenderColorManager::colorForEntity(RenderEntityType entity_type,
                                               uint64_t entity_uid,
                                               uint64_t part_id,
                                               RenderColorState state) {
    const auto role = roleFromEntityType(entity_type);
    const auto key = safePartKey(part_id, entity_uid);
    return colorForPart(key, role, state);
}

} // namespace OpenGeoLab::Render
