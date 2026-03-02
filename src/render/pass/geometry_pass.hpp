/**
 * @file geometry_pass.hpp
 * @brief Render pass for CAD geometry (surfaces, wireframes, points)
 */

#pragma once

#include "../core/gpu_buffer.hpp"
#include "../core/shader_program.hpp"
#include "kangaroo/util/noncopyable.hpp"
#include "render/render_data.hpp"

#include <QMatrix4x4>
#include <QVector3D>

namespace OpenGeoLab::Render {
class GeometryPass : private Kangaroo::Util::NonCopyMoveable {
public:
    GeometryPass() = default;
    ~GeometryPass() = default;

    /** @brief Compile shaders and initialise GPU buffer. */
    void initialize();
    /** @brief Release all GPU resources. */
    void cleanup();
    /** @brief Rebuild draw-range lists and optionally re-upload vertex data.
     *  @param data Current render data snapshot.
     */
    void updateBuffers(const RenderData& data);
    /**
     * @brief Render all geometry (surfaces, wireframes, points).
     * @param view View matrix
     * @param projection Projection matrix
     * @param camera_pos Camera world position (for lighting)
     * @param x_ray_mode When true, surfaces are rendered semi-transparent
     */
    void render(const QMatrix4x4& view,
                const QMatrix4x4& projection,
                const QVector3D& camera_pos,
                bool x_ray_mode);
    // Rendering subroutines split out from `render()` for clarity and testability
    void renderTriangles(QOpenGLFunctions* f,
                         const QMatrix4x4& view,
                         const QMatrix4x4& projection,
                         const QVector3D& camera_pos,
                         bool x_ray_mode);
    void renderLines(QOpenGLFunctions* f, const QMatrix4x4& view, const QMatrix4x4& projection);
    void renderPoints(QOpenGLFunctions* f, const QMatrix4x4& view, const QMatrix4x4& projection);

    /** @brief Read-only access to the GPU buffer used by this pass. */
    GpuBuffer& gpuBuffer() { return m_gpuBuffer; }

    /** @brief Draw ranges for triangle (surface) primitives. */
    const std::vector<DrawRangeEx>& triangleRanges() const { return m_triangleRanges; }
    /** @brief Draw ranges for line (edge/wireframe) primitives. */
    const std::vector<DrawRangeEx>& lineRanges() const { return m_lineRanges; }
    /** @brief Draw ranges for point (vertex) primitives. */
    const std::vector<DrawRangeEx>& pointRanges() const { return m_pointRanges; }

private:
    ShaderProgram m_surfaceShader; ///< Lit shader for surface triangles
    ShaderProgram m_flatShader;    ///< Flat-color shader for edges and points
    GpuBuffer m_gpuBuffer;         ///< Shared vertex/index GPU buffer
    bool m_initialized{false};     ///< True after initialize() succeeds

    std::vector<DrawRangeEx> m_triangleRanges; ///< Per-entity triangle draw ranges
    std::vector<DrawRangeEx> m_lineRanges;     ///< Per-entity line draw ranges
    std::vector<DrawRangeEx> m_pointRanges;    ///< Per-entity point draw ranges

    uint64_t m_uploadedVertexVersion{0}; ///< Last RenderPassData vertex version uploaded to GPU
};
} // namespace OpenGeoLab::Render