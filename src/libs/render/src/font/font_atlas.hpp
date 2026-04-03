/**
 * @file font_atlas.hpp
 * @brief MSDF font atlas loader — texture + glyph metrics.
 */

#pragma once

#include <opengeolab/render/render_export.hpp>

#include <glad/gl.h>
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
 * Two-phase initialization:
 *   1. parseMetrics(jsonString) — parse glyph metrics (no GL required).
 *   2. loadTexture(pngPath) — create GL texture (requires valid GL context).
 *
 * parseMetrics can be called independently for unit testing without GL.
 */
class OPENGEOLAB_RENDER_EXPORT FontAtlas final {
public:
    /// Parse glyph metrics from a JSON string. No GL context needed.
    /// @return true on success.
    bool parseMetrics(std::string_view json_string);

    /// Load the MSDF atlas PNG and create a GL_RGB8 texture.
    /// Requires a valid GL context. Call after parseMetrics().
    /// @return true on success.
    bool loadTexture(const std::string& png_path);

    /// Release the GL texture. Safe to call if not initialized.
    void cleanup();

    /// Bind the atlas texture to the given texture unit.
    void bind(GLuint texture_unit = 0) const;

    /// Look up glyph metrics by Unicode code point.
    /// @return nullptr if the glyph is not in the atlas.
    [[nodiscard]] const GlyphMetrics* glyph(uint32_t code_point) const;

    [[nodiscard]] glm::ivec2 atlasSize() const noexcept { return m_atlasSize; }
    [[nodiscard]] float lineHeight() const noexcept { return m_lineHeight; }
    [[nodiscard]] float ascender() const noexcept { return m_ascender; }
    [[nodiscard]] float descender() const noexcept { return m_descender; }
    [[nodiscard]] float pxRange() const noexcept { return m_pxRange; }
    [[nodiscard]] GLuint textureId() const noexcept { return m_texture; }

private:
    GLuint m_texture{0};
    glm::ivec2 m_atlasSize{};
    float m_lineHeight{};
    float m_ascender{};
    float m_descender{};
    float m_pxRange{4.0F};
    std::unordered_map<uint32_t, GlyphMetrics> m_glyphs;
};

} // namespace OpenGeoLab::Render
