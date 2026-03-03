/**
 * @file selection_pass.hpp
 * @brief GPU-based entity picking via off-screen FBO rendering and pixel readback.
 */

#pragma once

#include "../core/gpu_buffer.hpp"
#include "../core/pick_fbo.hpp"
#include "../core/shader_program.hpp"
#include "render/render_data.hpp"
#include "render/render_types.hpp"

#include <QMatrix4x4>

namespace OpenGeoLab::Render {

/**
 * @brief Result of a pick analysis (decoded from GPU pixel readback).
 */
struct PickAnalysisResult {
    uint64_t m_uid{0};
    RenderEntityType m_type{RenderEntityType::None};
    uint64_t m_rawPickId{0};

    [[nodiscard]] bool isValid() const { return m_type != RenderEntityType::None && m_uid != 0; }
};

/**
 * @brief Renders geometry to a PickFBO for GPU-based entity picking.
 *
 * Called from processHover() and processPicking(), NOT during the main render() call.
 * Reads back a region of pixels around the cursor and analyzes them with type priority
 * (Vertex > Edge > Face; MeshNode > MeshLine > MeshElement).
 *
 * Only renders topologies matching the current pick type mask to avoid
 * unnecessary GPU work.
 */
class SelectionPass {
public:
    SelectionPass() = default;
    ~SelectionPass() = default;

    /** @brief Compile pick shader and create the PickFBO. */
    void initialize(int width, int height);

    /** @brief Resize the PickFBO to match the viewport. */
    void resize(int width, int height);

    /** @brief Release all GPU resources. */
    void cleanup();

    /**
     * @brief Render to PickFBO and analyze the region around (pixel_x, pixel_y).
     * @param f OpenGL functions
     * @param gpu_buffer Shared geometry GPU buffer
     * @param view View matrix
     * @param projection Projection matrix
     * @param pixel_x Pixel X in device coordinates
     * @param pixel_y Pixel Y in device coordinates
     * @param pick_mask Bitmask of pickable entity types
     * @param triangle_ranges Draw ranges for triangle primitives
     * @param line_ranges Draw ranges for line primitives
     * @param point_ranges Draw ranges for point primitives
     * @return Best pick result with type priority applied
     */
    [[nodiscard]] PickAnalysisResult pick(QOpenGLFunctions* f,
                                          GpuBuffer& gpu_buffer,
                                          const QMatrix4x4& view,
                                          const QMatrix4x4& projection,
                                          int pixel_x,
                                          int pixel_y,
                                          RenderEntityTypeMask pick_mask,
                                          const std::vector<DrawRange>& triangle_ranges,
                                          const std::vector<DrawRange>& line_ranges,
                                          const std::vector<DrawRange>& point_ranges);

    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    ShaderProgram m_pickShader;
    PickFBO m_pickFBO;
    bool m_initialized{false};

    static constexpr int PICK_REGION_SIZE = 11; ///< 11x11 pixel region for analysis

    /** @brief Analyze readback pixels with type priority and distance weighting. */
    [[nodiscard]] PickAnalysisResult analyzeRegion(const std::vector<PickPixel>& pixels,
                                                   int region_w,
                                                   int region_h,
                                                   RenderEntityTypeMask pick_mask) const;

    /** @brief Should triangles be rendered for this pick mask? */
    [[nodiscard]] static bool shouldRenderTriangles(RenderEntityTypeMask mask);

    /** @brief Should lines be rendered for this pick mask? */
    [[nodiscard]] static bool shouldRenderLines(RenderEntityTypeMask mask);

    /** @brief Should points be rendered for this pick mask? */
    [[nodiscard]] static bool shouldRenderPoints(RenderEntityTypeMask mask);

    /** @brief Type priority for pick resolution (lower = higher priority). */
    [[nodiscard]] static int typePriority(RenderEntityType type);
};

} // namespace OpenGeoLab::Render
