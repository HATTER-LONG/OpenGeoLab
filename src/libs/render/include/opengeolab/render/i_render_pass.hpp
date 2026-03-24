/**
 * @file i_render_pass.hpp
 * @brief Declares the abstract render pass interface.
 */
#pragma once

#include <opengeolab/render/render_context.hpp>

namespace OpenGeoLab::Render {

/**
 * @brief Abstract interface for a single render pass.
 *
 * PassManager calls setup → execute → teardown in priority order.
 * Each Pass owns its own GPU resource lifecycle.
 */
class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    /** @brief Allocate GPU resources (on first call or after resize). */
    virtual void setup(int width, int height) = 0;

    /** @brief Execute the render pass. */
    virtual void execute(const RenderContext& ctx) = 0;

    /** @brief Release GPU resources. */
    virtual void teardown() = 0;
};

} // namespace OpenGeoLab::Render