#include "util/color_map.hpp"

#include <array>

namespace OpenGeoLab::Util {
namespace {

using RenderColor = Render::RenderColor;

constexpr RenderColor rgba(float r, float g, float b, float a = 1.0f) {
    return RenderColor{r, g, b, a};
}

constexpr float clamp01(float value) {
    if(value < 0.0F) {
        return 0.0F;
    }
    if(value > 1.0F) {
        return 1.0F;
    }
    return value;
}

constexpr RenderColor shiftLuma(const RenderColor& c, float delta) {
    return rgba(clamp01(c.m_r + delta), clamp01(c.m_g + delta), clamp01(c.m_b + delta), c.m_a);
}

constexpr RenderColor tuneSaturation(const RenderColor& c, float factor) {
    const float luma = c.m_r * 0.299F + c.m_g * 0.587F + c.m_b * 0.114F;
    return rgba(clamp01(luma + (c.m_r - luma) * factor), clamp01(luma + (c.m_g - luma) * factor),
                clamp01(luma + (c.m_b - luma) * factor), c.m_a);
}

constexpr RenderColor emphasizeMesh(const RenderColor& base, bool dark_theme) {
    const auto luma_shifted = dark_theme ? shiftLuma(base, 0.14F) : shiftLuma(base, -0.08F);
    return tuneSaturation(luma_shifted, dark_theme ? 1.14F : 1.10F);
}

constexpr std::array<RenderColor, 32> K_PART_COLOR_PALETTE_LIGHT = {{
    rgba(0.219F, 0.470F, 0.898F), rgba(0.950F, 0.432F, 0.314F), rgba(0.000F, 0.655F, 0.529F),
    rgba(0.560F, 0.389F, 0.953F), rgba(0.961F, 0.673F, 0.208F), rgba(0.000F, 0.737F, 0.831F),
    rgba(0.886F, 0.314F, 0.553F), rgba(0.376F, 0.647F, 0.980F), rgba(0.996F, 0.537F, 0.196F),
    rgba(0.204F, 0.780F, 0.349F), rgba(0.427F, 0.376F, 0.941F), rgba(0.859F, 0.318F, 0.318F),
    rgba(0.078F, 0.690F, 0.659F), rgba(0.694F, 0.451F, 0.902F), rgba(0.271F, 0.596F, 0.898F),
    rgba(0.949F, 0.420F, 0.698F), rgba(0.150F, 0.741F, 0.611F), rgba(0.976F, 0.576F, 0.263F),
    rgba(0.459F, 0.521F, 0.965F), rgba(0.925F, 0.361F, 0.388F), rgba(0.082F, 0.718F, 0.847F),
    rgba(0.612F, 0.504F, 0.941F), rgba(0.992F, 0.635F, 0.251F), rgba(0.204F, 0.667F, 0.820F),
    rgba(0.922F, 0.478F, 0.282F), rgba(0.337F, 0.698F, 0.522F), rgba(0.518F, 0.467F, 0.918F),
    rgba(0.984F, 0.478F, 0.510F), rgba(0.173F, 0.655F, 0.741F), rgba(0.780F, 0.416F, 0.878F),
    rgba(0.412F, 0.592F, 0.933F), rgba(0.941F, 0.565F, 0.337F),
}};

constexpr std::array<RenderColor, 32> makeDarkPartPalette() {
    std::array<RenderColor, 32> palette{};
    for(size_t i = 0; i < palette.size(); ++i) {
        palette[i] = tuneSaturation(shiftLuma(K_PART_COLOR_PALETTE_LIGHT[i], 0.10F), 0.92F);
    }
    return palette;
}

constexpr std::array<RenderColor, 32> K_PART_COLOR_PALETTE_DARK = makeDarkPartPalette();

constexpr std::array<RenderColor, 32>
makeMeshPalette(const std::array<RenderColor, 32>& part_palette, bool dark_theme) {
    std::array<RenderColor, 32> palette{};
    for(size_t i = 0; i < palette.size(); ++i) {
        palette[i] = emphasizeMesh(part_palette[i], dark_theme);
    }
    return palette;
}

constexpr std::array<RenderColor, 32> K_MESH_COLOR_PALETTE_LIGHT =
    makeMeshPalette(K_PART_COLOR_PALETTE_LIGHT, false);
constexpr std::array<RenderColor, 32> K_MESH_COLOR_PALETTE_DARK =
    makeMeshPalette(K_PART_COLOR_PALETTE_DARK, true);

struct ThemePalette {
    const std::array<RenderColor, 32>& m_part;
    const std::array<RenderColor, 32>& m_meshElement;
    RenderColor m_hover;
    RenderColor m_selection;
    RenderColor m_edge;
    RenderColor m_vertex;
    RenderColor m_meshNode;
    RenderColor m_meshLine;
};

constexpr ThemePalette K_THEME_PALETTES[2] = {
    ThemePalette{K_PART_COLOR_PALETTE_LIGHT, K_MESH_COLOR_PALETTE_LIGHT,
                 rgba(0.122F, 0.553F, 0.898F, 1.0F), rgba(0.988F, 0.576F, 0.090F, 1.0F),
                 rgba(0.100F, 0.125F, 0.178F, 1.0F), rgba(0.074F, 0.368F, 0.690F, 1.0F),
                 rgba(0.114F, 0.647F, 0.839F, 1.0F), rgba(0.039F, 0.235F, 0.412F, 1.0F)},
    ThemePalette{K_PART_COLOR_PALETTE_DARK, K_MESH_COLOR_PALETTE_DARK,
                 rgba(0.365F, 0.741F, 0.996F, 1.0F), rgba(1.000F, 0.741F, 0.243F, 1.0F),
                 rgba(0.804F, 0.847F, 0.922F, 1.0F), rgba(0.639F, 0.784F, 0.996F, 1.0F),
                 rgba(0.604F, 0.894F, 1.000F, 1.0F), rgba(0.749F, 0.859F, 0.961F, 1.0F)}};

constexpr const ThemePalette& paletteForIndex(uint8_t index) {
    return K_THEME_PALETTES[index == static_cast<uint8_t>(ColorTheme::Light) ? 0 : 1];
}

constexpr size_t K_PART_PALETTE_SIZE = K_PART_COLOR_PALETTE_LIGHT.size();

} // namespace

const ColorMap& ColorMap::instance() {
    static const ColorMap instance;
    return instance;
}

ColorMap& ColorMap::mutableInstance() {
    static ColorMap instance;
    return instance;
}

void ColorMap::setTheme(ColorTheme theme) noexcept {
    m_themeIndex.store(static_cast<uint8_t>(theme), std::memory_order_release);
}

void ColorMap::setThemeMode(int mode) noexcept {
    setTheme(mode == 0 ? ColorTheme::Light : ColorTheme::Dark);
}

ColorTheme ColorMap::theme() const noexcept {
    return m_themeIndex.load(std::memory_order_acquire) == static_cast<uint8_t>(ColorTheme::Light)
               ? ColorTheme::Light
               : ColorTheme::Dark;
}

const RenderColor& ColorMap::getColorForPartId(Geometry::EntityUID part_uid) const noexcept {
    const auto& palette = paletteForIndex(m_themeIndex.load(std::memory_order_relaxed)).m_part;
    const auto index = static_cast<size_t>(part_uid % K_PART_PALETTE_SIZE);
    return palette[index];
}

const RenderColor&
ColorMap::getColorForMeshElementId(Mesh::MeshElementUID element_uid) const noexcept {
    const auto& palette =
        paletteForIndex(m_themeIndex.load(std::memory_order_relaxed)).m_meshElement;
    const auto index = static_cast<size_t>(element_uid % K_PART_PALETTE_SIZE);
    return palette[index];
}

const RenderColor& ColorMap::getHoverColor() const noexcept {
    return paletteForIndex(m_themeIndex.load(std::memory_order_relaxed)).m_hover;
}

const RenderColor& ColorMap::getSelectionColor() const noexcept {
    return paletteForIndex(m_themeIndex.load(std::memory_order_relaxed)).m_selection;
}

const RenderColor& ColorMap::getEdgeColor() const noexcept {
    return paletteForIndex(m_themeIndex.load(std::memory_order_relaxed)).m_edge;
}

const RenderColor& ColorMap::getVertexColor() const noexcept {
    return paletteForIndex(m_themeIndex.load(std::memory_order_relaxed)).m_vertex;
}

const RenderColor& ColorMap::getMeshNodeColor() const noexcept {
    return paletteForIndex(m_themeIndex.load(std::memory_order_relaxed)).m_meshNode;
}

const RenderColor& ColorMap::getMeshLineColor() const noexcept {
    return paletteForIndex(m_themeIndex.load(std::memory_order_relaxed)).m_meshLine;
}

} // namespace OpenGeoLab::Util
