/**
 * @file mesh_pass.hpp
 * @brief Render pass for mesh data (triangulated FEM elements)
 */

#pragma once

#include "render/core/gpu_buffer.hpp"
#include "render/core/shader_program.hpp"
#include "render/render_data.hpp"

#include <QMatrix4x4>
#include <QVector3D>

namespace OpenGeoLab::Render {

class MeshPass {
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
    uint32_t m_totalVertexCount{0};
};

} // namespace OpenGeoLab::Render
