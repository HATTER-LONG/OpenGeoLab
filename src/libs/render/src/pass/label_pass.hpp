/**
 * @file label_pass.hpp
 * @brief LabelPass — billboard MSDF entity label rendering
 */

#pragma once

#include "render_pass_base.hpp"
#include "core/shader_program.hpp"

#include <glad/gl.h>

#include <cstdint>

namespace OpenGeoLab::Render {

class FontAtlas;

/**
 * @brief Renders billboard MSDF labels attached to selected entities.
 *
 * Labels are pre-resolved to world-space anchors in FrameState::resolvedLabels.
 * This pass projects anchors to screen space, generates billboard quads,
 * and renders them with MSDF fragment shading.
 *
 * Depth: disabled (labels render on top of scene geometry).
 * Blending: standard alpha blending for background transparency.
 */
class LabelPass final : public RenderPassBase {
public:
    /// Set the font atlas (must be called before initialize).
    void setFontAtlas(FontAtlas* atlas) { m_fontAtlas = atlas; }

    void render(const FrameState& state, const GpuBufferManager& buffers) override;

protected:
    bool onInitialize() override;
    void onCleanup() override;

private:
    /// Per-vertex data for a label quad (background or glyph).
    struct LabelVertex {
        float pos[2];            ///< Screen-space position in pixels
        float texCoord[2];       ///< Atlas UV (0,0 for background)
        float color[4];          ///< RGBA color
        float isMsdf;            ///< 1.0 for glyph, 0.0 for background/pointer
    };

    void buildLabelGeometry(const FrameState& state);

    ShaderProgram m_shader;
    FontAtlas* m_fontAtlas{nullptr};
    GLuint m_vao{0};
    GLuint m_vbo{0};
    std::vector<LabelVertex> m_vertices;
    uint32_t m_lastLabelVersion{0};
    glm::mat4 m_lastViewMatrix{0.0f};
    glm::mat4 m_lastProjMatrix{0.0f};
    int m_lastViewportWidth{0};
    int m_lastViewportHeight{0};
    GLint m_viewportSizeLoc{0};
};

} // namespace OpenGeoLab::Render
