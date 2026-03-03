/**
 * @file transparent_pass.hpp
 * @brief Render pass for semi-transparent surface triangles (X-ray mode).
 */

#pragma once

#include "../core/gpu_buffer.hpp"
#include "../core/shader_program.hpp"
#include "render/render_data.hpp"

#include <QMatrix4x4>
#include <QVector3D>

namespace OpenGeoLab::Render {

/**
 * @brief Renders all face triangles with semi-transparency for X-ray mode.
 *
 * Uses premultiplied alpha blending and disables depth writes
 * so back faces show through front faces.
 * Mutually exclusive with OpaquePass in the render pipeline.
 */
class TransparentPass {
public:
    TransparentPass() = default;
    ~TransparentPass() = default;

    void initialize();
    void cleanup();

    /**
     * @brief Render all triangles with semi-transparency.
     * @param f OpenGL functions
     * @param gpu_buffer Shared geometry GPU buffer
     * @param view View matrix
     * @param projection Projection matrix
     * @param camera_pos Camera world-space position (for lighting)
     * @param triangle_ranges Draw ranges for triangle primitives
     */
    void render(QOpenGLFunctions* f,
                GpuBuffer& gpu_buffer,
                const QMatrix4x4& view,
                const QMatrix4x4& projection,
                const QVector3D& camera_pos,
                const std::vector<DrawRange>& triangle_ranges);

private:
    ShaderProgram m_shader;
    bool m_initialized{false};
    static constexpr float XRAY_ALPHA = 0.25f;
};

} // namespace OpenGeoLab::Render
