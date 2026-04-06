/**
 * @file tessellation_overlay_pass.hpp
 * @brief Draws tessellation triangle edges (white) and vertices (purple) as debug overlay
 */

#pragma once

#include "core/shader_program.hpp"
#include "render_pass_base.hpp"

namespace OpenGeoLab::Render {

/**
 * @brief Debug overlay rendering tessellation mesh wireframe and vertex dots.
 *
 * Re-draws existing triangleRanges with glPolygonMode(GL_LINE) for white triangle
 * edges, then with GL_POINTS for purple vertex markers. Only active when
 * FrameState::showTessellation is true.
 */
class TessellationOverlayPass final : public RenderPassBase {
public:
    void render(const FrameState& state, const GpuBufferManager& buffers) override;

protected:
    bool onInitialize() override;
    void onCleanup() override;

private:
    ShaderProgram m_shader;
};

} // namespace OpenGeoLab::Render
