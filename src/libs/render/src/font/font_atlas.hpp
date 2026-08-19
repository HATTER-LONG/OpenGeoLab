/**
 * @file font_atlas.hpp
 * @brief MSDF font atlas loader — texture + glyph metrics.
 */

#pragma once

#include <opengeolab/render/render_export.hpp>

#include <glm/vec2.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace OpenGeoLab::Render {

/// Per-glyph metrics loaded from the atlas JSON.
struct GlyphMetrics {
    float advance{};            ///< Horizontal advance in EM units
    float planeBounds[4]{};     ///< left, bottom, right, top in EM space
    float atlasBounds[4]{};     ///< left, bottom, right, top in atlas pixels
};

/**
 * @brief Loads a pre-generated MSDF font atlas for label rendering.
 *
 * GPU texture ownership belongs to the active QRhi renderer. This class only
 * stores backend-neutral atlas metrics.
 */
class OPENGEOLAB_RENDER_EXPORT FontAtlas final {
public:
    /// Parse glyph metrics from a JSON string. No GL context needed.
    /// @return true on success.
    bool parseMetrics(std::string_view json_string);

    /// Look up glyph metrics by Unicode code point.
    /// @return nullptr if the glyph is not in the atlas.
    [[nodiscard]] const GlyphMetrics* glyph(uint32_t code_point) const;

    [[nodiscard]] glm::ivec2 atlasSize() const noexcept { return m_atlasSize; }
    [[nodiscard]] float lineHeight() const noexcept { return m_lineHeight; }
    [[nodiscard]] float ascender() const noexcept { return m_ascender; }
    [[nodiscard]] float descender() const noexcept { return m_descender; }
    [[nodiscard]] float pxRange() const noexcept { return m_pxRange; }
private:
    glm::ivec2 m_atlasSize{};
    float m_lineHeight{};
    float m_ascender{};
    float m_descender{};
    float m_pxRange{4.0F};
    std::unordered_map<uint32_t, GlyphMetrics> m_glyphs;
};

} // namespace OpenGeoLab::Render
