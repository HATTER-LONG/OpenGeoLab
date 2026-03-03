/**
 * @file wireframe_pass.hpp
 * @brief Render pass for edge lines and vertex points (batch draw, no per-entity highlight).
 */

#pragma once

#include "../core/gpu_buffer.hpp"
#include "../core/shader_program.hpp"
#include "render/render_data.hpp"

#include <QMatrix4x4>

namespace OpenGeoLab::Render {

/**
 * @brief Renders all edge lines and vertex points with batch draw calls.
 *
 * Uses a flat-color shader without per-entity highlight uniforms.
 * Lines are drawn in a single glDrawElements call; points in a single glDrawArrays call.
 */
class WireframePass {
public:
    WireframePass() = default;
    ~WireframePass() = default;

    void initialize();
    void cleanup();

    /**
     * @brief Render all lines and points in batch draw calls.
     * @param f OpenGL functions
     * @param gpu_buffer Shared geometry GPU buffer
     * @param view View matrix
     * @param projection Projection matrix
     * @param line_ranges Draw ranges for line primitives
     * @param point_ranges Draw ranges for point primitives
     */
    void render(QOpenGLFunctions* f,
                GpuBuffer& gpu_buffer,
                const QMatrix4x4& view,
                const QMatrix4x4& projection,
                const std::vector<DrawRange>& line_ranges,
                const std::vector<DrawRange>& point_ranges);

private:
    ShaderProgram m_shader;
    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
