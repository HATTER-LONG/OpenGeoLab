/**
 * @file selection_pass.hpp
 * @brief SelectionPass — offscreen FBO picking for entity selection.
 *
 * Formerly PickPass. Renders per-entity integer IDs to an offscreen
 * framebuffer for GPU-based cursor picking and hover detection.
 */

#pragma once

#include "render/core/pick_fbo.hpp"
#include "render/core/shader_program.hpp"
#include "render/render_data.hpp"
#include "render/render_select_manager.hpp"

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

    /**
     * @brief Render to pick FBO using per-entity draw ranges and type mask.
     */
    void renderToFbo(const QMatrix4x4& view,
                     const QMatrix4x4& projection,
                     GpuBuffer& geomBuffer,
                     const std::vector<DrawRangeEx>& triRanges,
                     const std::vector<DrawRangeEx>& lineRanges,
                     const std::vector<DrawRangeEx>& pointRanges,
                     GpuBuffer& meshBuffer,
                     uint32_t meshSurfaceCount,
                     uint32_t meshWireframeCount,
                     uint32_t meshNodeCount,
                     RenderEntityTypeMask pickMask);

    [[nodiscard]] uint64_t readPickId(int pixelX, int pixelY) const;
    [[nodiscard]] std::vector<uint64_t> readPickRegion(int cx, int cy, int radius) const;
    [[nodiscard]] PickFbo& fbo() { return m_fbo; }

private:
    ShaderProgram m_pickShader;
    PickFbo m_fbo;
    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
