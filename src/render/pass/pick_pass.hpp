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
     * Geometry buffer: triangles for face/solid/part/shell/wire types,
     * lines for edge types, points for vertex types (with enlarged point size).
     * Mesh buffer: triangles for mesh element types, lines for mesh line types,
     * points for mesh node types.
     */
    void renderToFbo(const QMatrix4x4& view,
                     const QMatrix4x4& projection,
                     GpuBuffer& geom_buffer,
                     const std::vector<DrawRangeEx>& tri_ranges,
                     const std::vector<DrawRangeEx>& line_ranges,
                     const std::vector<DrawRangeEx>& point_ranges,
                     GpuBuffer& mesh_buffer,
                     uint32_t mesh_surface_count,
                     uint32_t mesh_wireframe_count,
                     uint32_t mesh_node_count,
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
