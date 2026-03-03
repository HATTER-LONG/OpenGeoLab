/**
 * @file post_process_pass.hpp
 * @brief Stub render pass for future post-processing effects (outlines, SSAO, etc.).
 */

#pragma once

#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {

/**
 * @brief Placeholder for post-processing effects.
 *
 * Will be extended to support outline rendering, screen-space ambient
 * occlusion, or other full-screen image-space effects.
 */
class PostProcessPass {
public:
    PostProcessPass() = default;
    ~PostProcessPass() = default;

    void initialize() { m_initialized = true; }
    void cleanup() { m_initialized = false; }
    void render(QOpenGLFunctions* /*f*/) { /* stub */ }

private:
    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
