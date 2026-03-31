/**
 * @file wireframe_pass.hpp
 * @brief Draws edges (GL_LINES) and vertex points (GL_POINTS)
 */

#pragma once

#include "core/shader_program.hpp"
#include "render_pass_base.hpp"

namespace OpenGeoLab::Render {

class WireframePass final : public RenderPassBase {
public:
    void render(const FrameState& state, const GpuBufferManager& buffers) override;

protected:
    bool onInitialize() override;
    void onCleanup() override;

private:
    ShaderProgram m_lineShader;
    ShaderProgram m_pointShader;
};

} // namespace OpenGeoLab::Render
