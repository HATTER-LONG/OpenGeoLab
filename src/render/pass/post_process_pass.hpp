/**
 * @file post_process_pass.hpp
 * @brief PostProcessPass — stub for future post-processing effects.
 */

#pragma once

#include "render/pass/render_pass_base.hpp"

namespace OpenGeoLab::Render {

/**
 * @brief Stub pass for future post-processing effects.
 */
class PostProcessPass : public RenderPassBase {
public:
    void render() {}

private:
    bool onInitialize() override { return true; }
};

} // namespace OpenGeoLab::Render
