/**
 * @file color_map.cpp
 * @brief ColorMap implementation with a single color palette
 */

#include "util/color_map.hpp"

#include <array>

namespace OpenGeoLab::Util {
namespace {

using RenderColor = Render::RenderColor;

/// @brief Construct an RGBA color from float components.
constexpr RenderColor rgba(float r, float g, float b, float a = 1.0f) {
    return RenderColor{r, g, b, a};
}

/// @brief Convert a hex (0-255) component to float [0, 1].
constexpr float hex(int v) { return static_cast<float>(v) / 255.0F; }

/// Part color palette (15 distinctive colours for cyclic assignment).
/// #abedd8 #46cdcf #fad2a3 #b2df8a #33a02c #1f78b4 #a6cee3 #fb9a99
/// #cca8e9 #984ea3 #ffff33 #377eb8 #4daf4a #f781bf #999999
constexpr size_t K_PART_PALETTE_SIZE = 15;

constexpr std::array<RenderColor, K_PART_PALETTE_SIZE> K_PART_COLOR_PALETTE = {{
    rgba(hex(0xAB), hex(0xED), hex(0xD8)), // #abedd8
    rgba(hex(0x46), hex(0xCD), hex(0xCF)), // #46cdcf
    rgba(hex(0xFA), hex(0xD2), hex(0xA3)), // #fad2a3
    rgba(hex(0xB2), hex(0xDF), hex(0x8A)), // #b2df8a
    rgba(hex(0x33), hex(0xA0), hex(0x2C)), // #33a02c
    rgba(hex(0x1F), hex(0x78), hex(0xB4)), // #1f78b4
    rgba(hex(0xA6), hex(0xCE), hex(0xE3)), // #a6cee3
    rgba(hex(0xFB), hex(0x9A), hex(0x99)), // #fb9a99
    rgba(hex(0xCC), hex(0xA8), hex(0xE9)), // #cca8e9
    rgba(hex(0x98), hex(0x4E), hex(0xA3)), // #984ea3
    rgba(hex(0xFF), hex(0xFF), hex(0x33)), // #ffff33
    rgba(hex(0x37), hex(0x7E), hex(0xB8)), // #377eb8
    rgba(hex(0x4D), hex(0xAF), hex(0x4A)), // #4daf4a
    rgba(hex(0xF7), hex(0x81), hex(0xBF)), // #f781bf
    rgba(hex(0x99), hex(0x99), hex(0x99)), // #999999
}};

/// Mesh element palette (reuses the part colour palette).
constexpr auto& K_MESH_COLOR_PALETTE = K_PART_COLOR_PALETTE;

/// Edge + vertex hover: #ff7f00, selection: #ff165d
/// Face hover: #4b55e9, selection: #4116ff
constexpr RenderColor K_EDGE_VERTEX_HOVER = rgba(hex(0xFF), hex(0x7F), hex(0x00));
constexpr RenderColor K_EDGE_VERTEX_SELECTION = rgba(hex(0xFF), hex(0x16), hex(0x5D));
constexpr RenderColor K_FACE_HOVER = rgba(hex(0x4B), hex(0x55), hex(0xE9));
constexpr RenderColor K_FACE_SELECTION = rgba(hex(0x41), hex(0x16), hex(0xFF));
constexpr RenderColor K_EDGE_COLOR = rgba(hex(0xFF), hex(0xD4), hex(0x60));
constexpr RenderColor K_VERTEX_COLOR = rgba(hex(0x34), hex(0x90), hex(0xDE));
constexpr RenderColor K_MESH_NODE_COLOR = rgba(0.114F, 0.647F, 0.839F, 1.0F);
constexpr RenderColor K_MESH_LINE_COLOR = rgba(0.039F, 0.235F, 0.412F, 1.0F);
constexpr RenderColor K_BACKGROUND_COLOR = rgba(0.15F, 0.15F, 0.17F, 1.0F);

} // namespace

const ColorMap& ColorMap::instance() {
    static const ColorMap instance;
    return instance;
}

const RenderColor& ColorMap::getColorForPartId(Geometry::EntityUID part_uid) const noexcept {
    const auto index = static_cast<size_t>(part_uid % K_PART_PALETTE_SIZE);
    return K_PART_COLOR_PALETTE[index];
}

RenderColor ColorMap::darkenColor(const RenderColor& color, float factor) noexcept {
    return {color.m_r * factor, color.m_g * factor, color.m_b * factor, color.m_a};
}

const RenderColor&
ColorMap::getColorForMeshElementId(Mesh::MeshElementUID element_uid) const noexcept {
    const auto index = static_cast<size_t>(element_uid % K_PART_PALETTE_SIZE);
    return K_MESH_COLOR_PALETTE[index];
}

const RenderColor& ColorMap::getEdgeVertexHoverColor() const noexcept {
    return K_EDGE_VERTEX_HOVER;
}

const RenderColor& ColorMap::getEdgeVertexSelectionColor() const noexcept {
    return K_EDGE_VERTEX_SELECTION;
}

const RenderColor& ColorMap::getFaceHoverColor() const noexcept { return K_FACE_HOVER; }

const RenderColor& ColorMap::getFaceSelectionColor() const noexcept { return K_FACE_SELECTION; }

const RenderColor& ColorMap::getEdgeColor() const noexcept { return K_EDGE_COLOR; }

const RenderColor& ColorMap::getVertexColor() const noexcept { return K_VERTEX_COLOR; }

const RenderColor& ColorMap::getMeshNodeColor() const noexcept { return K_MESH_NODE_COLOR; }

const RenderColor& ColorMap::getMeshLineColor() const noexcept { return K_MESH_LINE_COLOR; }

const RenderColor& ColorMap::getBackgroundColor() const noexcept { return K_BACKGROUND_COLOR; }

} // namespace OpenGeoLab::Util
