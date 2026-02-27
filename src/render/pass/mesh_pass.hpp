/**
 * @file mesh_pass.hpp
 * @brief Render pass for mesh data (surfaces, wireframes, and node points)
 */

#pragma once

#include "render/core/gpu_buffer.hpp"
#include "render/core/shader_program.hpp"
#include "render/render_data.hpp"

#include <QMatrix4x4>
#include <QVector3D>

namespace OpenGeoLab::Render {

/**
 * @brief Renders FEM mesh elements with separate surface, wireframe, and point draws.
 *
 * The vertex buffer is laid out as [surface triangles | wireframe lines | node points].
 * Display mode controls which sections are drawn each frame.
 */
class MeshPass {
public:
    /** @brief Compile shaders and initialise GPU buffer. */
    void initialize();
    /** @brief Release all GPU resources. */
    void cleanup();
    /** @brief Rebuild per-topology vertex counts and optionally re-upload data.
     *  @param data Current render data snapshot.
     */
    void updateBuffers(const RenderData& data);
    /** @brief Draw mesh surfaces, wireframes, and node points.
     *  @param view View matrix
     *  @param projection Projection matrix
     *  @param camera_pos Camera world position (for lighting)
     *  @param x_ray_mode When true, surfaces are rendered semi-transparent
     */
    void render(const QMatrix4x4& view,
                const QMatrix4x4& projection,
                const QVector3D& camera_pos,
                bool x_ray_mode = false);

    /** @brief Read-only access to the GPU buffer used by this pass. */
    GpuBuffer& gpuBuffer() { return m_gpuBuffer; }
    /** @brief Total number of vertices across all topology sections. */
    [[nodiscard]] uint32_t totalVertexCount() const { return m_totalVertexCount; }
    /** @brief Number of vertices in the surface (triangle) section. */
    [[nodiscard]] uint32_t surfaceVertexCount() const { return m_surfaceVertexCount; }
    /** @brief Number of vertices in the wireframe (line) section. */
    [[nodiscard]] uint32_t wireframeVertexCount() const { return m_wireframeVertexCount; }
    /** @brief Number of vertices in the node (point) section. */
    [[nodiscard]] uint32_t nodeVertexCount() const { return m_nodeVertexCount; }

    /** @brief Set the display-mode bitmask controlling which sections are drawn. */
    void setDisplayMode(RenderDisplayModeMask mode) { m_displayMode = mode; }
    /** @brief Current display-mode bitmask. */
    [[nodiscard]] RenderDisplayModeMask displayMode() const { return m_displayMode; }

private:
    ShaderProgram m_surfaceShader;  ///< Lit shader for mesh surface triangles
    ShaderProgram m_flatShader;     ///< Flat-color shader for wireframe and points
    GpuBuffer m_gpuBuffer;          ///< Shared vertex GPU buffer
    bool m_initialized{false};      ///< True after initialize() succeeds

    uint32_t m_totalVertexCount{0};     ///< Total vertices in buffer
    uint32_t m_surfaceVertexCount{0};   ///< Vertices for surface triangles
    uint32_t m_wireframeVertexCount{0}; ///< Vertices for wireframe lines
    uint32_t m_nodeVertexCount{0};      ///< Vertices for node points

    RenderDisplayModeMask m_displayMode{RenderDisplayModeMask::Wireframe}; ///< Active display mode
};

} // namespace OpenGeoLab::Render
