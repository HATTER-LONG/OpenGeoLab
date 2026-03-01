/**
 * @file ui_pass.hpp
 * @brief UIPass — stub for future UI overlay rendering.
 */

#pragma once

#include "render/pass/render_pass_base.hpp"

namespace OpenGeoLab::Render {

/**
 * @brief Stub pass for future UI overlay rendering.
 */
class UIPass : public RenderPassBase {
public:
    void render() {}

private:
    bool onInitialize() override { return true; }
};

} // namespace OpenGeoLab::Render
