/**
 * @file opaque_pass.hpp
 * @brief Draws filled triangles with 4-component lighting model
 */

#pragma once

#include "core/shader_program.hpp"
#include "render_pass_base.hpp"

namespace OpenGeoLab::Render {

class OpaquePass final : public RenderPassBase {
public:
    void render(const FrameState& state, const GpuBufferManager& buffers) override;

protected:
    bool onInitialize() override;
    void onCleanup() override;

private:
    ShaderProgram m_shader;
    GLint m_normalMatrixLoc{0};
};

} // namespace OpenGeoLab::Render
