/**
 * @file opaque_pass.hpp
 * @brief OpaquePass — renders opaque surfaces from geometry and mesh buffers.
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
 * @brief Renders opaque (non-transparent) surfaces.
 *
 * Handles both geometry (CAD) face triangles via indexed drawing and mesh
 * (FEM) surface triangles via array drawing. Uses a lit surface shader
 * with headlamp + ambient lighting. Polygon offset is applied to resolve
 * depth fighting with coplanar wireframe edges.
 *
 * Skipped when X-ray mode is active (TransparentPass handles that case).
 */
class OpaquePass : public RenderPassBase {
public:
    void render(RenderPassContext& context) override;

private:
    bool onInitialize() override;
    void onCleanup() override;

    ShaderProgram m_surfaceShader;
};

} // namespace OpenGeoLab::Render
