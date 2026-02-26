/**
 * @file pick_pass.hpp
 * @brief GPU picking pass — renders entity IDs to an offscreen FBO
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
 * @brief Input data for geometry pick rendering (BRep entities).
 */
struct GeometryPickInput {
    GpuBuffer& m_buffer;
    const std::vector<DrawRangeEx>& m_triRanges;
    const std::vector<DrawRangeEx>& m_lineRanges;
    const std::vector<DrawRangeEx>& m_pointRanges;
};

/**
 * @brief Input data for mesh pick rendering (FEM entities).
 */
struct MeshPickInput {
    GpuBuffer& m_buffer;
    uint32_t m_surfaceCount{0};
    uint32_t m_wireframeCount{0};
    uint32_t m_nodeCount{0};
};

/**
 * @brief Offscreen picking pass that renders per-entity integer IDs into a
 *        PickFbo, then reads back the ID under the cursor for selection/hover.
 */
class PickPass {
public:
    /** @brief Allocate FBO and compile the pick shader.
     *  @param width  Initial FBO width in pixels.
     *  @param height Initial FBO height in pixels.
     */
    void initialize(int width, int height);
    /** @brief Resize the offscreen FBO to match a new viewport size. */
    void resize(int width, int height);
    /** @brief Release all GPU resources (FBO + shader). */
    void cleanup();

    /**
     * @brief Render to pick FBO using per-entity draw ranges and type mask.
     *
     * Only draws primitives whose entity type matches the pickMask.
     */
    void renderToFbo(const QMatrix4x4& view,
                     const QMatrix4x4& projection,
                     const GeometryPickInput& geom,
                     const MeshPickInput& mesh,
                     RenderEntityTypeMask pickMask);

    /**
     * @brief Read the pick ID at a single pixel.
     */
    [[nodiscard]] uint64_t readPickId(int pixel_x, int pixel_y) const;

    /**
     * @brief Read pick IDs in a region around (cx, cy).
     * @param radius Half-size of the region (e.g. 3 → 7x7 pixels)
     */
    [[nodiscard]] std::vector<uint64_t> readPickRegion(int cx, int cy, int radius) const;

    /** @brief Access the underlying offscreen framebuffer. */
    [[nodiscard]] PickFbo& fbo() { return m_fbo; }

private:
    ShaderProgram m_pickShader; ///< Shader that writes encoded pick IDs to the FBO
    PickFbo m_fbo;              ///< Offscreen RG32UI framebuffer for pick readback
    bool m_initialized{false};  ///< True after initialize() succeeds
};

} // namespace OpenGeoLab::Render
