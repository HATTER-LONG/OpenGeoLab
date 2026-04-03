/**
 * @file wireframe_pass.hpp
 * @brief Draws edges (thick quads) and vertex points (GL_POINTS)
 */

#pragma once

#include "core/shader_program.hpp"
#include "render_pass_base.hpp"

namespace OpenGeoLab::Render {

class ThickLineRenderer;

class WireframePass final : public RenderPassBase {
public:
    void render(const FrameState& state, const GpuBufferManager& buffers) override;

    /** @brief Set the shared thick-line renderer (owned by RenderPipeline). */
    void setThickLineRenderer(ThickLineRenderer* renderer) { m_thickLine = renderer; }

protected:
    bool onInitialize() override;
    void onCleanup() override;

private:
    ShaderProgram m_pointShader;
    ThickLineRenderer* m_thickLine{nullptr};
};

} // namespace OpenGeoLab::Render
