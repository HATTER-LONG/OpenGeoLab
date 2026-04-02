/**
 * @file label_pass.cpp
 * @brief LabelPass implementation — MSDF billboard label rendering
 */

#include "pass/label_pass.hpp"

#include "font/font_atlas.hpp"

#include <opengeolab/core/label_colors.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string_view>

namespace OpenGeoLab::Render {

namespace {

constexpr std::string_view LABEL_VS = R"glsl(
#version 330 core

layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_texCoord;
layout(location = 2) in vec4 a_color;
layout(location = 3) in float a_isMsdf;
layout(location = 4) in float a_occlusionAlpha;

uniform vec2 u_viewportSize;

out vec2 v_texCoord;
out vec4 v_color;
out float v_isMsdf;

void main() {
    vec2 ndc = (a_pos / u_viewportSize) * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    v_texCoord = a_texCoord;
    v_color = vec4(a_color.rgb, a_color.a * a_occlusionAlpha);
    v_isMsdf = a_isMsdf;
}
)glsl";

constexpr std::string_view LABEL_FS = R"glsl(
#version 330 core

uniform sampler2D u_atlas;
uniform float u_pxRange;

in vec2 v_texCoord;
in vec4 v_color;
in float v_isMsdf;

out vec4 fragColor;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    if (v_isMsdf > 0.5) {
        vec3 msd = texture(u_atlas, v_texCoord).rgb;
        float sd = median(msd.r, msd.g, msd.b);
        vec2 unitRange = u_pxRange / vec2(textureSize(u_atlas, 0));
        vec2 screenTexSize = 1.0 / fwidth(v_texCoord);
        float screenPxRange = max(0.5 * dot(unitRange, screenTexSize), 1.0);
        float opacity = smoothstep(0.5 - 1.0 / screenPxRange,
                                   0.5 + 1.0 / screenPxRange, sd);
        fragColor = vec4(v_color.rgb, v_color.a * opacity);
    } else {
        fragColor = v_color;
    }
}
)glsl";

// Billboard sizing constants
constexpr float K_FONT_SCALE = 24.0F;   ///< Base font size in pixels
constexpr float K_PAD_H = 4.0F;         ///< Horizontal padding
constexpr float K_PAD_V = 2.0F;         ///< Vertical padding
constexpr float K_POINTER_HEIGHT = 6.0F; ///< Pointer triangle height
constexpr float K_POINTER_HALF_W = 4.0F; ///< Pointer triangle half-width
constexpr float K_STACK_GAP = 4.0F;     ///< Gap between stacked labels
constexpr float K_MIN_PX = 12.0F;       ///< Minimum label pixel size
constexpr float K_MAX_PX = 48.0F;       ///< Maximum label pixel size

} // namespace

bool LabelPass::onInitialize() {
    if(!m_shader.create(LABEL_VS, LABEL_FS)) {
        return false;
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // Layout: pos(2f) + texCoord(2f) + color(4f) + isMsdf(1f) + occlusionAlpha(1f) = 10 floats
    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(LabelVertex));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(4 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(8 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(9 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void LabelPass::onCleanup() {
    m_shader.destroy();
    if(m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if(m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
}

void LabelPass::buildLabelGeometry(const FrameState& state) {
    m_vertices.clear();
    if(m_fontAtlas == nullptr) {
        return;
    }

    const auto vp_w = static_cast<float>(state.viewportWidth);
    const auto vp_h = static_cast<float>(state.viewportHeight);
    const auto mvp = state.projMatrix * state.viewMatrix;

    for(const auto& label : state.resolvedLabels) {
        // Project anchor to screen space
        auto clip = mvp * glm::vec4(label.anchorWorld, 1.0F);
        if(clip.w <= 0.0F) {
            continue; // Behind camera
        }
        auto ndc = glm::vec3(clip) / clip.w;
        float screen_x = (ndc.x * 0.5F + 0.5F) * vp_w;
        float screen_y = (ndc.y * 0.5F + 0.5F) * vp_h;

        // Compute text width from glyph advances
        float text_width = 0.0F;
        for(char ch : label.text) {
            const auto* gm = m_fontAtlas->glyph(static_cast<uint32_t>(ch));
            if(gm != nullptr) {
                text_width += gm->advance * K_FONT_SCALE;
            }
        }

        float text_height = m_fontAtlas->lineHeight() * K_FONT_SCALE;
        float bg_w = text_width + 2.0F * K_PAD_H;
        float bg_h = text_height + 2.0F * K_PAD_V;

        // Stack offset (upward)
        float stack_offset = static_cast<float>(label.stackIndex) *
                             (bg_h + K_POINTER_HEIGHT + K_STACK_GAP);

        // Label center (above anchor)
        float label_cx = screen_x;
        float label_cy = screen_y + K_POINTER_HEIGHT + bg_h * 0.5F + stack_offset;

        float occlusion_alpha = label.occluded ? Core::K_LABEL_OCCLUDED_ALPHA : 1.0F;

        // -- Background rectangle (two triangles) --
        float bg_left = label_cx - bg_w * 0.5F;
        float bg_right = label_cx + bg_w * 0.5F;
        float bg_bottom = label_cy - bg_h * 0.5F;
        float bg_top = label_cy + bg_h * 0.5F;

        auto bg = label.bgColor;
        auto push_bg = [&](float x, float y) {
            m_vertices.push_back(
                {{x, y}, {0.0F, 0.0F}, {bg.r, bg.g, bg.b, bg.a}, 0.0F, occlusion_alpha});
        };
        // Triangle 1
        push_bg(bg_left, bg_bottom);
        push_bg(bg_right, bg_bottom);
        push_bg(bg_right, bg_top);
        // Triangle 2
        push_bg(bg_left, bg_bottom);
        push_bg(bg_right, bg_top);
        push_bg(bg_left, bg_top);

        // -- Pointer triangle --
        float ptr_top = bg_bottom;
        float ptr_bottom = screen_y + stack_offset;
        push_bg(label_cx - K_POINTER_HALF_W, ptr_top);
        push_bg(label_cx + K_POINTER_HALF_W, ptr_top);
        push_bg(label_cx, ptr_bottom);

        // -- Glyph quads --
        float cursor_x = bg_left + K_PAD_H;
        float baseline_y = label_cy - text_height * 0.5F +
                           m_fontAtlas->ascender() * K_FONT_SCALE;

        auto atlas_w = static_cast<float>(m_fontAtlas->atlasSize().x);
        auto atlas_h = static_cast<float>(m_fontAtlas->atlasSize().y);
        auto tc = label.textColor;

        for(char ch : label.text) {
            const auto* gm = m_fontAtlas->glyph(static_cast<uint32_t>(ch));
            if(gm == nullptr) {
                continue;
            }

            // Skip whitespace glyphs (no atlas bounds)
            if(gm->atlasBounds[0] == 0.0F && gm->atlasBounds[2] == 0.0F) {
                cursor_x += gm->advance * K_FONT_SCALE;
                continue;
            }

            // Glyph screen-space quad
            float gx0 = cursor_x + gm->planeBounds[0] * K_FONT_SCALE;
            float gy0 = baseline_y - gm->planeBounds[3] * K_FONT_SCALE; // top
            float gx1 = cursor_x + gm->planeBounds[2] * K_FONT_SCALE;
            float gy1 = baseline_y - gm->planeBounds[1] * K_FONT_SCALE; // bottom

            // Atlas UVs (normalized)
            float u0 = gm->atlasBounds[0] / atlas_w;
            float v0 = gm->atlasBounds[1] / atlas_h;
            float u1 = gm->atlasBounds[2] / atlas_w;
            float v1 = gm->atlasBounds[3] / atlas_h;

            auto push_glyph = [&](float x, float y, float u, float v) {
                m_vertices.push_back(
                    {{x, y}, {u, v}, {tc.r, tc.g, tc.b, tc.a}, 1.0F, occlusion_alpha});
            };
            // Triangle 1
            push_glyph(gx0, gy0, u0, v1);
            push_glyph(gx1, gy0, u1, v1);
            push_glyph(gx1, gy1, u1, v0);
            // Triangle 2
            push_glyph(gx0, gy0, u0, v1);
            push_glyph(gx1, gy1, u1, v0);
            push_glyph(gx0, gy1, u0, v0);

            cursor_x += gm->advance * K_FONT_SCALE;
        }
    }
}

void LabelPass::render(const FrameState& state, const GpuBufferManager& /*buffers*/) {
    if(!isInitialized() || m_fontAtlas == nullptr) {
        return;
    }
    if(!state.labelsVisible || state.resolvedLabels.empty()) {
        return;
    }

    buildLabelGeometry(state);
    if(m_vertices.empty()) {
        return;
    }

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_vertices.size() * sizeof(LabelVertex)),
                 m_vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Set GL state: depth read, no write, alpha blending
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader.use();

    // Set viewport size uniform via direct GL call (ShaderProgram has no setVec2)
    const GLint viewport_loc = glGetUniformLocation(m_shader.id(), "u_viewportSize");
    glUniform2f(viewport_loc,
                static_cast<float>(state.viewportWidth),
                static_cast<float>(state.viewportHeight));

    m_shader.setFloat("u_pxRange", m_fontAtlas->pxRange());

    m_fontAtlas->bind(0);
    m_shader.setInt("u_atlas", 0);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));
    glBindVertexArray(0);

    glUseProgram(0);

    // Restore GL state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

} // namespace OpenGeoLab::Render
