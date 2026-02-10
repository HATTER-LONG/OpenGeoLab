/**
 * @file render_pass.hpp
 * @brief Abstract render pass interface for the modular rendering pipeline
 *
 * RenderPass defines the interface for pluggable rendering passes.
 * Each pass encapsulates a self-contained stage of the rendering pipeline
 * (geometry display, picking, highlighting, compositing, etc.).
 */

#pragma once

#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QSize>
#include <QVector3D>
#include <cstdint>
#include <string>

namespace OpenGeoLab::Render {

class RendererCore;

/**
 * @brief Context passed to each render pass during execution
 *
 * Contains shared state needed by all passes: matrices, viewport info,
 * and a back-reference to RendererCore for resource access.
 */
struct RenderPassContext {
    RendererCore* m_core{nullptr}; ///< Owning core (for resource access)
    QSize m_viewportSize{800, 600};
    float m_aspectRatio{1.333f};

    struct Matrices {
        QMatrix4x4 m_view;
        QMatrix4x4 m_projection;
        QMatrix4x4 m_mvp; ///< projection * view * model (identity model)
    } m_matrices;

    QVector3D m_cameraPos;
};

/**
 * @brief Abstract base class for render passes
 *
 * Subclasses implement specific rendering stages. RendererCore calls passes
 * in registered order during each frame.
 */
class RenderPass {
public:
    virtual ~RenderPass() = default;

    /**
     * @brief Human-readable name for debugging/profiling.
     */
    [[nodiscard]] virtual const char* name() const = 0;

    /**
     * @brief One-time GPU resource setup (called once after GL context is current).
     * @param gl OpenGL functions
     */
    virtual void initialize(QOpenGLFunctions& gl) = 0;

    /**
     * @brief Called when viewport is resized.
     * @param gl OpenGL functions
     * @param size New viewport size
     */
    virtual void resize(QOpenGLFunctions& gl, const QSize& size) = 0;

    /**
     * @brief Execute the pass.
     * @param gl OpenGL functions
     * @param ctx Render pass context with matrices and shared state
     */
    virtual void execute(QOpenGLFunctions& gl, const RenderPassContext& ctx) = 0;

    /**
     * @brief Release GPU resources.
     * @param gl OpenGL functions
     */
    virtual void cleanup(QOpenGLFunctions& gl) = 0;

    /**
     * @brief Whether this pass is currently enabled.
     */
    [[nodiscard]] bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

protected:
    RenderPass() = default;

private:
    bool m_enabled{true};
};

} // namespace OpenGeoLab::Render
