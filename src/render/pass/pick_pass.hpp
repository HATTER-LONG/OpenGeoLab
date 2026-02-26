/**
 * @file pick_pass.hpp
 * @brief GPU picking pass — renders entity IDs to an offscreen FBO
 */

#pragma once

#include "render/core/pick_fbo.hpp"
#include "render/core/shader_program.hpp"
#include "render/render_data.hpp"
#include "render/render_select_manager.hpp"

#include <QMatrix4x4>

namespace OpenGeoLab::Render {

class GpuBuffer;

class PickPass {
public:
    void initialize(int width, int height);
    void resize(int width, int height);
    void cleanup();

    uint64_t execute(int pixel_x,
                     int pixel_y,
                     const QMatrix4x4& view,
                     const QMatrix4x4& projection,
                     GpuBuffer& geom_buffer,
                     GpuBuffer& mesh_buffer);

private:
    ShaderProgram m_pickShader;
    PickFbo m_fbo;
    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
