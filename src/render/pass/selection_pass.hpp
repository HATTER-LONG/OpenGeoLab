/**
 * @file selection_pass.hpp
 * @brief SelectionPass — offscreen FBO picking for entity selection.
 *
 * Formerly PickPass. Renders per-entity integer IDs to an offscreen
 * framebuffer for GPU-based cursor picking and hover detection.
 */

#pragma once

#include "render_pass_base.hpp"
#include "render_pass_context.hpp"

#include "../core/pick_fbo.hpp"
#include "../core/shader_program.hpp"

#include <QMatrix4x4>

namespace OpenGeoLab::Render {

class GpuBuffer;

/**
 * @brief Offscreen picking pass that renders per-entity integer IDs into a
 *        PickFbo, then reads back the ID under the cursor for selection/hover.
 */
class SelectionPass {
public:
    void initialize(int width, int height);
    void resize(int width, int height);
    void cleanup();
    bool isInitialized() const { return m_initialized; }
    void render(RenderPassContext& ctx);

    [[nodiscard]] uint64_t readPickId(int pixel_x, int pixel_y) const;
    [[nodiscard]] std::vector<uint64_t> readPickRegion(int cx, int cy, int radius) const;
    [[nodiscard]] PickFbo& fbo() { return m_fbo; }

private:
    ShaderProgram m_pickShader;
    PickFbo m_fbo;
    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
