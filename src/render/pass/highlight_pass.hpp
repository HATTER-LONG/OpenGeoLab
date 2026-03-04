/**
 * @file highlight_pass.hpp
 * @brief HighlightPass — renders selected/hovered entity overlay.
 */

#pragma once

#include "render/core/shader_program.hpp"
#include "render/pass/render_pass_base.hpp"
#include "render/pass/render_pass_context.hpp"

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
    void render(const RenderPassContext& ctx);

private:
    void renderGeometry(const PassRenderParams& params,
                        GpuBuffer& geomBuffer,
                        const std::vector<DrawRangeEx>& triangleRanges,
                        const std::vector<DrawRangeEx>& lineRanges,
                        const std::vector<DrawRangeEx>& pointRanges);

    void renderMesh(const PassRenderParams& params,
                    GpuBuffer& meshBuffer,
                    const std::vector<DrawRangeEx>& meshTriangleRanges,
                    const std::vector<DrawRangeEx>& meshLineRanges,
                    const std::vector<DrawRangeEx>& meshPointRanges,
                    RenderDisplayModeMask meshDisplayMode);

    bool onInitialize() override;
    void onCleanup() override;

    ShaderProgram m_surfaceShader;     ///< Lit shader for face highlighting
    ShaderProgram m_flatShader;        ///< Flat shader for edge/point highlighting
    ShaderProgram m_meshSurfaceShader; ///< Mesh surface shader with pickId-based highlight
    ShaderProgram m_meshFlatShader;    ///< Mesh flat shader with pickId-based highlight
};

} // namespace OpenGeoLab::Render
