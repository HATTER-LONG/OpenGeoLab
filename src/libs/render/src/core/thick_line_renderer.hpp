/**
 * @file thick_line_renderer.hpp
 * @brief Screen-space thick line rendering via TBO-based instanced quads.
 *
 * Replaces glLineWidth (unreliable in Core Profile) with a vertex shader
 * that expands each GL_LINES segment into a screen-space quad with
 * per-pixel anti-aliasing.
 */

#pragma once

#include "shader_program.hpp"

#include <opengeolab/scene/render_mesh_data.hpp>

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <span>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Renders line segments as screen-space quads with configurable width and AA.
 *
 * Uses Texture Buffer Objects to zero-copy read vertex positions and indices
 * from the existing GpuBufferManager VBO/IBO. Each line segment becomes 6
 * vertices (2 triangles) computed entirely in the vertex shader via gl_VertexID.
 *
 * Lifecycle: initialize() once → drawLines() per frame → cleanup() on shutdown.
 */
class ThickLineRenderer final {
public:
    /** @brief Parameters for a single thick-line draw batch. */
    struct DrawParams {
        GLuint positionVbo{0};      ///< Main VBO handle (interleaved RenderVertex, 40B stride).
        GLuint indexBuffer{0};      ///< IBO handle (uint32_t indices).
        glm::mat4 mvp{1.0F};        ///< Model-view-projection matrix.
        glm::vec2 viewport{};       ///< Viewport size in physical pixels (width×dpr, height×dpr).
        float lineWidth{1.5F};      ///< Line width in physical pixels.
        glm::vec4 color{};          ///< Override color (used when useVertexColor is false).
        bool useVertexColor{false}; ///< true: use original vertex color; false: use color uniform.
        float colorMix{0.0F};       ///< Blend factor: 0=pure vertex color, 1=pure override color.
        float depthBias{0.0005F};   ///< Clip-space depth offset towards camera.
    };

    /** @brief Compile shaders and create TBO/VAO resources. Returns false on failure. */
    [[nodiscard]] bool initialize();
    void cleanup();

    /**
     * @brief Draw edge segments from DrawRanges as screen-space thick quads.
     *
     * Caller must have a current GL context. This method manages its own
     * shader, TBO bindings, blend state, and empty VAO — the caller's
     * VAO binding is NOT required.
     */
    void drawLines(const DrawParams& params, std::span<const Scene::DrawRange> ranges);

private:
    ShaderProgram m_shader;
    GLuint m_positionTbo{0}; ///< Texture for R32F view of position VBO.
    GLuint m_indexTbo{0};    ///< Texture for R32I view of index buffer.
    GLuint m_emptyVao{0};    ///< Empty VAO (Core Profile requires a bound VAO).
};

} // namespace OpenGeoLab::Render
