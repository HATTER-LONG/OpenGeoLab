/**
 * @file font_atlas.cpp
 * @brief FontAtlas implementation — JSON parsing + GL texture loading
 */

#include "font/font_atlas.hpp"

#include <nlohmann/json.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>

#include <fstream>
#include <sstream>

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

bool FontAtlas::loadTexture(const std::string& png_path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(1);
    auto* data = stbi_load(png_path.c_str(), &width, &height, &channels, 3);
    if(data == nullptr) {
        return false;
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    return true;
}

void FontAtlas::cleanup() {
    if(m_texture != 0) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
}

void FontAtlas::bind(GLuint texture_unit) const {
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, m_texture);
}

const GlyphMetrics* FontAtlas::glyph(uint32_t code_point) const {
    auto it = m_glyphs.find(code_point);
    return it != m_glyphs.end() ? &it->second : nullptr;
}

} // namespace OpenGeoLab::Render
