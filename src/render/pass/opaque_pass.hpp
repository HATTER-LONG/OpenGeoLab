/**
 * @file opaque_pass.hpp
 * @brief OpaquePass — renders opaque surfaces from geometry and mesh buffers.
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
    /**
     * @brief Render opaque surfaces from both geometry and mesh GPU buffers.
     * @param params Shared render parameters (view, projection, camera pos).
     * @param geomBuffer Geometry GPU buffer (indexed drawing).
     * @param triangleRanges Pre-built geometry triangle DrawRangeEx list.
     * @param meshBuffer Mesh GPU buffer (array drawing).
     * @param meshSurfaceCount Number of mesh surface vertices.
     * @param meshDisplayMode Mesh display mode mask.
     */
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
