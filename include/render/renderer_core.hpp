/**
 * @file renderer_core.hpp
 * @brief Central rendering engine managing GL context, resources, and pass scheduling
 *
 * RendererCore owns all GPU resources (shaders, buffers, FBOs) and dispatches
 * registered render passes in order. It replaces the monolithic SceneRenderer
 * with a modular, extensible architecture.
 */

#pragma once

#include "render/render_data.hpp"
#include "render/render_pass.hpp"
#include "render/renderable.hpp"

#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QSize>
#include <QVector3D>
#include <memory>
#include <string>
#include <vector>

namespace OpenGeoLab::Render {

class SelectManager;

/**
 * @brief Central rendering engine
 *
 * Responsibilities:
 * - Manage GL context and global GL state
 * - Own ShaderPool and RenderBatch
 * - Register/dispatch RenderPass instances
 * - Provide runtime GL capability checks
 */
class RendererCore : protected QOpenGLFunctions {
public:
    RendererCore();
    ~RendererCore();

    RendererCore(const RendererCore&) = delete;
    RendererCore& operator=(const RendererCore&) = delete;
    RendererCore(RendererCore&&) = delete;
    RendererCore& operator=(RendererCore&&) = delete;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Initialize OpenGL and all registered passes.
     */
    void initialize();

    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief Release all GPU resources.
     */
    void cleanup();

    // -------------------------------------------------------------------------
    // Runtime GL check
    // -------------------------------------------------------------------------

    /**
     * @brief Check that the current GL context meets minimum requirements.
     * @return true if GL >= 4.3 and integer textures / shader integer output are supported.
     */
    [[nodiscard]] bool checkGLCapabilities() const;

    // -------------------------------------------------------------------------
    // Viewport
    // -------------------------------------------------------------------------

    void setViewportSize(const QSize& size);
    [[nodiscard]] const QSize& viewportSize() const { return m_viewportSize; }

    // -------------------------------------------------------------------------
    // Data
    // -------------------------------------------------------------------------

    /**
     * @brief Upload render data to GPU.
     * @param data Document render data containing all category batches to upload
     */
    void uploadMeshData(const DocumentRenderData& data);

    [[nodiscard]] RenderBatch& batch() { return m_batch; }
    [[nodiscard]] const RenderBatch& batch() const { return m_batch; }

    // -------------------------------------------------------------------------
    // Pass management
    // -------------------------------------------------------------------------

    /**
     * @brief Register a pass at the end of the pipeline.
     * @param pass Unique pointer to the pass (ownership transferred).
     */
    void registerPass(std::unique_ptr<RenderPass> pass);

    /**
     * @brief Get a pass by name.
     * @param name Null-terminated pass name to search for
     * @return Pointer to the matching RenderPass, or nullptr if not found
     */
    [[nodiscard]] RenderPass* findPass(const char* name) const;

    // -------------------------------------------------------------------------
    // Rendering
    // -------------------------------------------------------------------------

    /**
     * @brief Execute all enabled passes in order.
     * @param camera_pos Camera position in world space
     * @param view_matrix View transformation matrix
     * @param projection_matrix Projection transformation matrix
     */
    void render(const QVector3D& camera_pos,
                const QMatrix4x4& view_matrix,
                const QMatrix4x4& projection_matrix);

    // -------------------------------------------------------------------------
    // Shader pool (simple key -> program map)
    // -------------------------------------------------------------------------

    /**
     * @brief Get or create a shader program by key.
     * @param key Unique string identifier for the shader program
     * @return Pointer to the shader program, or nullptr if not found
     */
    [[nodiscard]] QOpenGLShaderProgram* shader(const std::string& key) const;

    /**
     * @brief Register a compiled shader program.
     * @param key Unique string identifier for the shader program
     * @param program Compiled shader program (ownership transferred)
     */
    void registerShader(const std::string& key, std::unique_ptr<QOpenGLShaderProgram> program);

private:
    void setupDefaultShaders();

    bool m_initialized{false};
    QSize m_viewportSize{800, 600};
    RenderBatch m_batch;

    std::vector<std::unique_ptr<RenderPass>> m_passes;

    struct ShaderEntry {
        std::string m_key;
        std::unique_ptr<QOpenGLShaderProgram> m_program;
    };
    std::vector<ShaderEntry> m_shaders;
};

} // namespace OpenGeoLab::Render
