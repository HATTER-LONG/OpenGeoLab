/**
 * @file grid_pass.hpp
 * @brief Declares the infinite grid render pass on the XZ plane.
 */
#pragma once

#include <opengeolab/render/i_render_pass.hpp>
#include <opengeolab/render/render_export.hpp>
#include <opengeolab/render/shader_program.hpp>

#include <cstdint>

namespace OpenGeoLab::Render {

/**
 * @brief Renders an infinite grid on the XZ plane.
 *
 * Uses a fullscreen triangle + fragment shader world-space projection.
 * Grid lines fade with distance. Supports major (1m) and minor (0.1m) lines.
 *
 * Algorithm: "Rendering Infinite Grids" (Ben Golus / Alex Evans)
 * - Vertex shader: unprojects fullscreen triangle NDC to world XZ plane
 * - Fragment shader: uses fwidth() for anti-aliased grid lines, alpha fades with depth
 */
class OPENGEOLAB_RENDER_EXPORT GridPass final : public IRenderPass {
public:
    void setup(int width, int height) override;
    void execute(const RenderContext& ctx) override;
    void teardown() override;

private:
    ShaderProgram shader_;
    uint32_t vao_ = 0;
    bool initialized_ = false;
};

} // namespace OpenGeoLab::Render
