/**
 * @file font_atlas.cpp
 * @brief Backend-neutral FontAtlas metric parsing
 */

#include "font/font_atlas.hpp"

#include <nlohmann/json.hpp>

namespace OpenGeoLab::Render {

bool FontAtlas::parseMetrics(std::string_view json_string) {
    try {
        auto root = nlohmann::json::parse(json_string);

        // Atlas metadata
        const auto& atlas = root.at("atlas");
        m_atlasSize.x = atlas.at("width").get<int>();
        m_atlasSize.y = atlas.at("height").get<int>();
        if(atlas.contains("distanceRange")) {
            m_pxRange = atlas["distanceRange"].get<float>();
        }

        // Font metrics
        const auto& metrics = root.at("metrics");
        m_lineHeight = metrics.at("lineHeight").get<float>();
        m_ascender = metrics.at("ascender").get<float>();
        m_descender = metrics.at("descender").get<float>();

        // Glyphs
        m_glyphs.clear();
        for(const auto& g : root.at("glyphs")) {
            GlyphMetrics gm{};
            auto code_point = g.at("unicode").get<uint32_t>();
            gm.advance = g.at("advance").get<float>();

            if(g.contains("planeBounds")) {
                const auto& pb = g["planeBounds"];
                gm.planeBounds[0] = pb.at("left").get<float>();
                gm.planeBounds[1] = pb.at("bottom").get<float>();
                gm.planeBounds[2] = pb.at("right").get<float>();
                gm.planeBounds[3] = pb.at("top").get<float>();
            }

            if(g.contains("atlasBounds")) {
                const auto& ab = g["atlasBounds"];
                gm.atlasBounds[0] = ab.at("left").get<float>();
                gm.atlasBounds[1] = ab.at("bottom").get<float>();
                gm.atlasBounds[2] = ab.at("right").get<float>();
                gm.atlasBounds[3] = ab.at("top").get<float>();
            }

            m_glyphs[code_point] = gm;
        }

        return true;
    } catch(const nlohmann::json::exception&) {
        return false;
    }
}

const GlyphMetrics* FontAtlas::glyph(uint32_t code_point) const {
    auto it = m_glyphs.find(code_point);
    return it != m_glyphs.end() ? &it->second : nullptr;
}

} // namespace OpenGeoLab::Render
