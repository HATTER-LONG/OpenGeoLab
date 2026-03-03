/**
 * @file opaque_pass.hpp
 * @brief Render pass for opaque surface triangles (batch draw, no per-entity highlight).
 */

#pragma once

#include "../core/gpu_buffer.hpp"
#include "../core/shader_program.hpp"
#include "render/render_data.hpp"

#include <QMatrix4x4>
#include <QVector3D>

namespace OpenGeoLab::Render {

/**
 * @brief Renders all opaque face triangles in a single batched glDrawElements call.
 *
 * Uses a lit surface shader without per-entity highlight uniforms.
 * All face triangles are drawn with their baked vertex colors and lighting.
 */
class OpaquePass {
public:
    OpaquePass() = default;
    ~OpaquePass() = default;

    /** @brief Compile surface shader. */
    void initialize();

    /** @brief Release shader resources. */
    void cleanup();

    /**
     * @brief Render all triangles in a single batch draw call.
     * @param f OpenGL functions
     * @param gpu_buffer Shared geometry GPU buffer (must be initialized and uploaded)
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
};

} // namespace OpenGeoLab::Render
