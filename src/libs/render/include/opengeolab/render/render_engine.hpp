/**
 * @file render_engine.hpp
 * @brief OpenGL core renderer with two-pass Phong + edge pipeline.
 */

#pragma once

#include <opengeolab/render/camera.hpp>
#include <opengeolab/render/render_export.hpp>
#include <opengeolab/render/render_scene.hpp>
#include <opengeolab/render/shader_program.hpp>

namespace OpenGeoLab::Render {

/// OpenGL core-profile renderer.
///
/// Call initialize() once from the render thread (GL context must be current).
/// Call render() each frame.  Two-pass pipeline: Phong surfaces then edges.
class RenderEngine {
public:
    /// Initialize GL state and compile shaders.  Requires current GL context.
    void initialize();

    /// Handle viewport resize.
    void resize(int width, int height);

    /// Render a complete frame.
    void render(const RenderScene& scene, const Camera& camera);

    [[nodiscard]] bool isInitialized() const;

private:
    ShaderProgram m_phongShader;
    ShaderProgram m_edgeShader;
    int m_width{0};
    int m_height{0};
    bool m_initialized{false};

    void renderSurfaces(const RenderScene& scene, const Camera& camera);
    void renderEdges(const RenderScene& scene, const Camera& camera);
    void renderPoints(const RenderScene& scene, const Camera& camera);
};

} // namespace OpenGeoLab::Render
