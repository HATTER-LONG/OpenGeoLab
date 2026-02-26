/**
 * @file geometry_pass.hpp
 * @brief Render pass for CAD geometry (surfaces, wireframes, points)
 */

#pragma once

#include "render/core/gpu_buffer.hpp"
#include "render/core/shader_program.hpp"
#include "render/render_data.hpp"

#include <QMatrix4x4>
#include <QVector3D>

namespace OpenGeoLab::Render {

class GeometryPass {
public:
    void initialize();
    void cleanup();
    void updateBuffers(const RenderData& data);
    void render(const QMatrix4x4& view, const QMatrix4x4& projection, const QVector3D& camera_pos);

    GpuBuffer& gpuBuffer() { return m_gpuBuffer; }

private:
    ShaderProgram m_surfaceShader;
    ShaderProgram m_flatShader;
    GpuBuffer m_gpuBuffer;
    bool m_initialized{false};

    // Cached draw ranges from last updateBuffers
    std::vector<DrawRange> m_triangleRanges;
    std::vector<DrawRange> m_lineRanges;
    std::vector<DrawRange> m_pointRanges;
};

} // namespace OpenGeoLab::Render
