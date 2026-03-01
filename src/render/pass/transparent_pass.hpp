/**
 * @file transparent_pass.hpp
 * @brief TransparentPass — renders transparent (X-ray) surfaces.
 */

#pragma once

#include "render/core/shader_program.hpp"
#include "render/pass/render_pass_base.hpp"
#include "render/render_data.hpp"


#include <QMatrix4x4>
#include <QVector3D>

namespace OpenGeoLab::Render {

class GpuBuffer;

/**
 * @brief Renders transparent surfaces in X-ray mode.
 *
 * Uses premultiplied alpha blending with depth write disabled.
 * Only active when X-ray mode is enabled; otherwise does nothing.
 */
class TransparentPass : public RenderPassBase {
public:
    void render(const PassRenderParams& params,
                GpuBuffer& geomBuffer,
                const std::vector<DrawRangeEx>& triangleRanges,
                GpuBuffer& meshBuffer,
                uint32_t meshSurfaceCount,
                RenderDisplayModeMask meshDisplayMode);

private:
    bool onInitialize() override;
    void onCleanup() override;

    ShaderProgram m_surfaceShader;
};

} // namespace OpenGeoLab::Render
