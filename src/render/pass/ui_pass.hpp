/**
 * @file ui_pass.hpp
 * @brief Stub render pass for future UI overlay rendering (axes, grid, annotations).
 */

#pragma once

#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {

/**
 * @brief Placeholder for UI overlay rendering.
 *
 * Will be extended to support coordinate axes, grid, compass,
 * and other 2D/3D UI elements rendered on top of the scene.
 */
class UIPass {
public:
    UIPass() = default;
    ~UIPass() = default;

    void initialize() { m_initialized = true; }
    void cleanup() { m_initialized = false; }
    void render(QOpenGLFunctions* /*f*/) { /* stub */ }

private:
    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
