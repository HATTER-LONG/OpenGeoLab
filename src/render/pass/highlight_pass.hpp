/**
 * @file highlight_pass.hpp
 * @brief Render pass for selected/hovered entity overlay rendering.
 */

#pragma once

#include "../core/gpu_buffer.hpp"
#include "../core/shader_program.hpp"
#include "render/render_data.hpp"

#include <QMatrix4x4>
#include <QVector3D>

namespace OpenGeoLab::Render {

/**
 * @brief Renders only selected and hovered entities with highlight color overlay.
 *
 * Uses GL_LEQUAL depth test to draw exactly on top of existing geometry.
 * Per-entity draw calls are issued only for entities that are currently
 * selected or hovered (typically 0-5 per frame).
 */
class HighlightPass {
public:
    HighlightPass() = default;
    ~HighlightPass() = default;

    void initialize();
    void cleanup();

    /**
     * @brief Render highlighted entities.
     * @param f OpenGL functions
     * @param gpu_buffer Shared geometry GPU buffer
     * @param view View matrix
     * @param projection Projection matrix
     * @param camera_pos Camera world-space position (for face lighting)
     * @param triangle_ranges Draw ranges for triangle primitives
     * @param line_ranges Draw ranges for line primitives
     * @param point_ranges Draw ranges for point primitives
     */
    void render(QOpenGLFunctions* f,
                GpuBuffer& gpu_buffer,
                const QMatrix4x4& view,
                const QMatrix4x4& projection,
                const QVector3D& camera_pos,
                const std::vector<DrawRange>& triangle_ranges,
                const std::vector<DrawRange>& line_ranges,
                const std::vector<DrawRange>& point_ranges);

private:
    ShaderProgram m_surfaceShader; ///< For triangle (face) highlights
    ShaderProgram m_flatShader;    ///< For line/point (edge/vertex) highlights
    bool m_initialized{false};

    void renderHighlightedTriangles(QOpenGLFunctions* f,
                                    const QMatrix4x4& view,
                                    const QMatrix4x4& projection,
                                    const QVector3D& camera_pos,
                                    const std::vector<DrawRange>& triangle_ranges);

    void renderHighlightedLines(QOpenGLFunctions* f,
                                const QMatrix4x4& view,
                                const QMatrix4x4& projection,
                                const std::vector<DrawRange>& line_ranges);

    void renderHighlightedPoints(QOpenGLFunctions* f,
                                 const QMatrix4x4& view,
                                 const QMatrix4x4& projection,
                                 const std::vector<DrawRange>& point_ranges);
};

} // namespace OpenGeoLab::Render
