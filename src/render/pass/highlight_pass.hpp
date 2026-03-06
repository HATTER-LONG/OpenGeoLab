/**
 * @file highlight_pass.hpp
 * @brief HighlightPass — renders selected/hovered entity overlay.
 */

#pragma once

#include "../core/shader_program.hpp"
#include "render_pass_base.hpp"
#include "render_pass_context.hpp"

#include <QMatrix4x4>
#include <QVector3D>

namespace OpenGeoLab::Render {

class GpuBuffer;

/**
 * @brief Overdraw pass that highlights selected and hovered entities.
 *
 * Draws only entities that are currently selected or hovered by the cursor,
 * using GL_LEQUAL depth test to overdraw on top of the normal rendering.
 * Surfaces use a color mix highlight; edges use thicker lines + highlight
 * color; points use larger size + highlight color.
 *
 * For mesh highlighting, uses a per-vertex pickId-based shader approach
 * similar to the original MeshPass highlighting.
 */
class HighlightPass : public RenderPassBase {
public:
    void render(RenderPassContext& ctx) override;

private:
    void renderGeometry(const RenderPassContext& ctx);
    void renderMesh(const RenderPassContext& ctx);

    bool onInitialize() override;
    void onCleanup() override;

private:
    ShaderProgram m_surfaceShader;     ///< Lit shader for face highlighting
    ShaderProgram m_flatShader;        ///< Flat shader for edge/point highlighting
    ShaderProgram m_meshSurfaceShader; ///< Mesh surface shader with pickId-based highlight
    ShaderProgram m_meshFlatShader;    ///< Mesh flat shader with pickId-based highlight

    /// Per-topology pick-ID textures (GL_RG32UI, width = selection count).
    /// Index 0 = surface, 1 = line, 2 = node.  Uploaded each frame via glTexImage2D.
    std::array<GLuint, 3> m_selectPickTex{0, 0, 0};
};

} // namespace OpenGeoLab::Render
