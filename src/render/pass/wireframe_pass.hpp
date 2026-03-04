/**
 * @file wireframe_pass.hpp
 * @brief WireframePass — renders edges (lines) and points from geometry and mesh buffers.
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
 * @brief Renders wireframe edges and point vertices.
 *
 * Handles geometry (CAD) edges via indexed GL_LINES drawing and geometry
 * vertices via GL_POINTS. Also handles mesh wireframe edges and mesh node
 * points via array drawing. Uses a flat (unlit) shader.
 */
class WireframePass : public RenderPassBase {
public:
    void render(const RenderPassContext& ctx);

private:
    bool onInitialize() override;
    void onCleanup() override;

    ShaderProgram m_flatShader;
};

} // namespace OpenGeoLab::Render
