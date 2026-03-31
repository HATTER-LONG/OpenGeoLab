/**
 * @file highlight_pass.hpp
 * @brief Redraws selected/hovered geometry with highlight color overlay
 */

#pragma once

#include "core/shader_program.hpp"
#include "render_pass_base.hpp"

namespace OpenGeoLab::Render {

class HighlightPass final : public RenderPassBase {
public:
    void render(const FrameState& state, const GpuBufferManager& buffers) override;

protected:
    bool onInitialize() override;
    void onCleanup() override;

private:
    ShaderProgram m_faceShader; /**< Lit faces with highlight color mix */
    ShaderProgram m_edgeShader; /**< Flat-color edges with highlight */
};

} // namespace OpenGeoLab::Render
