/**
 * @file highlight_pass.hpp
 * @brief Redraws selected/hovered geometry with highlight color overlay
 */

#pragma once

#include "core/shader_program.hpp"
#include "render_pass_base.hpp"

namespace OpenGeoLab::Render {

class ThickLineRenderer;

class HighlightPass final : public RenderPassBase {
public:
    void render(const FrameState& state, const GpuBufferManager& buffers) override;

    /** @brief Set the shared thick-line renderer (owned by RenderPipeline). */
    void setThickLineRenderer(ThickLineRenderer* renderer) { m_thickLine = renderer; }

protected:
    bool onInitialize() override;
    void onCleanup() override;

private:
    ShaderProgram m_faceShader;  /**< Lit faces with highlight color mix */
    ShaderProgram m_pointShader; /**< Flat-color points with highlight */
    GLint m_normalMatrixLoc{0};
    ThickLineRenderer* m_thickLine{nullptr};
};

} // namespace OpenGeoLab::Render
