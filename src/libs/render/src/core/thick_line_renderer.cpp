/**
 * @file thick_line_renderer.cpp
 * @brief ThickLineRenderer implementation — TBO-based screen-space thick lines
 */

#include "core/thick_line_renderer.hpp"

#include <string_view>

namespace OpenGeoLab::Render {

// GLSL vertex shader assumes stride = sizeof(RenderVertex) / sizeof(float).
static_assert(sizeof(Scene::RenderVertex) == 40,
              "GLSL thick-line vertex shader assumes 10-float (40-byte) stride; update THICK_LINE_VS");

namespace {

constexpr std::string_view THICK_LINE_VS = R"glsl(
#version 330 core

uniform samplerBuffer u_positions;
uniform isamplerBuffer u_indices;
uniform int u_firstIndex;
uniform mat4 u_mvp;
uniform vec2 u_viewport;
uniform float u_lineWidth;
uniform float u_depthBias;

out float v_edgeDist;
out vec4 v_vertexColor;

void main() {
    int segId  = gl_VertexID / 6;
    int corner = gl_VertexID % 6;

    // Map 6 vertices to 4 quad corners (two triangles: 0-1-2, 2-1-3).
    //   corner: 0->BL  1->TL  2->BR  3->BR  4->TL  5->TR
    int qc;
    if      (corner == 0) qc = 0;
    else if (corner == 1) qc = 1;
    else if (corner == 2) qc = 2;
    else if (corner == 3) qc = 2;
    else if (corner == 4) qc = 1;
    else                  qc = 3;

    bool isP1  = (qc >= 2);
    float side = ((qc & 1) == 0) ? -1.0 : 1.0;

    // Fetch vertex indices from IBO TBO.
    int idx0 = texelFetch(u_indices, u_firstIndex + segId * 2    ).r;
    int idx1 = texelFetch(u_indices, u_firstIndex + segId * 2 + 1).r;

    // Fetch positions (stride = 10 floats per vertex).
    int b0 = idx0 * 10;
    int b1 = idx1 * 10;
    vec3 p0 = vec3(texelFetch(u_positions, b0    ).r,
                   texelFetch(u_positions, b0 + 1).r,
                   texelFetch(u_positions, b0 + 2).r);
    vec3 p1 = vec3(texelFetch(u_positions, b1    ).r,
                   texelFetch(u_positions, b1 + 1).r,
                   texelFetch(u_positions, b1 + 2).r);

    // Fetch vertex color (offset 6..9 in the vertex).
    int bc = isP1 ? b1 : b0;
    v_vertexColor = vec4(texelFetch(u_positions, bc + 6).r,
                         texelFetch(u_positions, bc + 7).r,
                         texelFetch(u_positions, bc + 8).r,
                         texelFetch(u_positions, bc + 9).r);

    // Project to clip space.
    vec4 clip0 = u_mvp * vec4(p0, 1.0);
    vec4 clip1 = u_mvp * vec4(p1, 1.0);

    // Screen-space direction and perpendicular.
    vec2 ndc0 = clip0.xy / clip0.w;
    vec2 ndc1 = clip1.xy / clip1.w;
    vec2 dirPx = (ndc1 - ndc0) * u_viewport * 0.5;
    float lenPx = length(dirPx);
    dirPx = (lenPx < 0.001) ? vec2(1.0, 0.0) : dirPx / lenPx;
    vec2 normPx = vec2(-dirPx.y, dirPx.x);

    // Half-width in pixels plus 1.5px AA margin.
    float hw = u_lineWidth * 0.5 + 1.5;
    vec2 offsetNDC = normPx * side * hw / (u_viewport * 0.5);

    // Final position with depth bias towards camera.
    vec4 baseClip = isP1 ? clip1 : clip0;
    gl_Position = vec4(baseClip.xy + offsetNDC * baseClip.w,
                       baseClip.z - u_depthBias * baseClip.w,
                       baseClip.w);

    v_edgeDist = side;
}
)glsl";

constexpr std::string_view THICK_LINE_FS = R"glsl(
#version 330 core

in float v_edgeDist;
in vec4 v_vertexColor;

uniform vec4 u_color;
uniform float u_lineWidth;
uniform bool u_useVertexColor;
uniform float u_colorMix;

out vec4 fragColor;

void main() {
    vec4 baseColor = u_useVertexColor
        ? mix(v_vertexColor, u_color, u_colorMix)
        : u_color;

    // Anti-aliasing: smoothstep at edge with 1.5px transition.
    float halfCore = u_lineWidth / (u_lineWidth + 3.0);
    float alpha = 1.0 - smoothstep(halfCore, 1.0, abs(v_edgeDist));

    fragColor = vec4(baseColor.rgb, baseColor.a * alpha);
}
)glsl";

} // namespace

bool ThickLineRenderer::initialize() {
    if(!m_shader.create(THICK_LINE_VS, THICK_LINE_FS)) {
        return false;
    }

    glGenTextures(1, &m_positionTbo);
    glGenTextures(1, &m_indexTbo);
    glGenVertexArrays(1, &m_emptyVao);
    return true;
}

void ThickLineRenderer::cleanup() {
    if(m_emptyVao != 0) {
        glDeleteVertexArrays(1, &m_emptyVao);
        m_emptyVao = 0;
    }
    if(m_indexTbo != 0) {
        glDeleteTextures(1, &m_indexTbo);
        m_indexTbo = 0;
    }
    if(m_positionTbo != 0) {
        glDeleteTextures(1, &m_positionTbo);
        m_positionTbo = 0;
    }
    m_shader.destroy();
}

void ThickLineRenderer::drawLines(const DrawParams& params,
                                  std::span<const Scene::DrawRange> ranges) {
    if(ranges.empty() || params.positionVbo == 0 || params.indexBuffer == 0) {
        return;
    }

    // Bind TBOs: zero-copy views into existing VBO and IBO.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, m_positionTbo);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, params.positionVbo);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, m_indexTbo);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, params.indexBuffer);

    // Shader setup.
    m_shader.use();
    m_shader.setInt("u_positions", 0);
    m_shader.setInt("u_indices", 1);
    m_shader.setMat4("u_mvp", params.mvp);
    m_shader.setVec4("u_color", params.color);
    m_shader.setFloat("u_lineWidth", params.lineWidth);
    m_shader.setFloat("u_depthBias", params.depthBias);
    m_shader.setFloat("u_colorMix", params.colorMix);

    const GLint vp_loc = glGetUniformLocation(m_shader.id(), "u_viewport");
    glUniform2f(vp_loc, params.viewport.x, params.viewport.y);

    const GLint uvc_loc = glGetUniformLocation(m_shader.id(), "u_useVertexColor");
    glUniform1i(uvc_loc, params.useVertexColor ? 1 : 0);

    // State: blending for AA edges, depth write on.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Bind empty VAO (Core Profile requires a bound VAO).
    glBindVertexArray(m_emptyVao);

    // Draw each range as instanced quads.
    for(const auto& r : ranges) {
        const auto num_segments = r.indexCount / 2;
        if(num_segments == 0) {
            continue;
        }
        m_shader.setInt("u_firstIndex", static_cast<int>(r.indexOffset));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(num_segments * 6));
    }

    // Restore state.
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
}

} // namespace OpenGeoLab::Render
