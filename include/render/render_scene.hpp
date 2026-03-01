/**
 * @file render_scene.hpp
 * @brief Abstract render-scene interface and its factory.
 */
#pragma once

#include "render/render_data.hpp"

#include <QMatrix4x4>
#include <QPointF>
#include <QSizeF>

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Render {

/**
 * @brief Per-frame state synchronized from the GUI thread.
 *
 * Built by the viewport renderer during synchronize() and passed to
 * IRenderScene::synchronize(). All data that the render thread needs
 * from the GUI thread is captured here, so render() has no dependency
 * on any GUI-thread singleton.
 */
struct SceneFrameState {
    const RenderData* m_renderData{nullptr}; ///< Pointer to controller's render data snapshot
    QVector3D m_cameraPos;                   ///< Camera world-space position (for lighting)
    QMatrix4x4 m_viewMatrix;                 ///< View transformation matrix
    QMatrix4x4 m_projMatrix;                 ///< Projection transformation matrix
    bool m_xRayMode{false};                  ///< X-ray (semi-transparent surfaces) mode
    RenderDisplayModeMask m_meshDisplayMode{
        RenderDisplayModeMask::None}; ///< Active mesh display modes
};

/**
 * @brief Input parameters for pixel picking
 *
 * Encapsulates cursor position and viewport geometry for a pick operation.
 * Camera matrices are obtained from the cached SceneFrameState.
 */
struct PickingInput {
    QPointF m_cursorPos;                   ///< Cursor position in item coordinates
    QSizeF m_itemSize;                     ///< Item size in logical pixels
    qreal m_devicePixelRatio{1.0};         ///< Device pixel ratio (HiDPI scaling)
    PickAction m_action{PickAction::None}; ///< Pending pick action
};
/**
 * @brief Abstract interface for an OpenGL render scene.
 *
 * Manages the full render lifecycle: initialization, synchronization,
 * per-frame rendering, GPU picking, hover highlighting, and cleanup.
 *
 * Data flow:
 *   synchronize(state) — cache GUI-thread state, update GPU buffers if dirty
 *   render()           — execute render passes using cached state (no singletons)
 *   processHover()     — GPU pick at cursor for hover highlight
 *   processPicking()   — GPU pick at cursor for click selection
 */
class IRenderScene : public Kangaroo::Util::NonCopyMoveable {
public:
    IRenderScene() = default;
    virtual ~IRenderScene() = default;

    /** @brief Allocate GPU resources (shaders, buffers, FBOs). */
    virtual void initialize() = 0;

    /** @brief Check whether the scene has been initialized. */
    [[nodiscard]] virtual bool isInitialized() const = 0;

    /** @brief Notify the scene that the viewport has been resized. */
    virtual void setViewportSize(const QSize& size) = 0;

    /**
     * @brief Synchronize GUI-thread state into the render scene.
     *
     * Called once per frame before render(). Caches render data, camera,
     * and display mode state. Updates GPU buffers only when data is dirty.
     * After this call, render() and processHover()/processPicking() use
     * only the cached state — no GUI-thread singletons are accessed.
     *
     * @param state Per-frame state snapshot from the GUI thread.
     */
    virtual void synchronize(const SceneFrameState& state) = 0;

    /**
     * @brief Execute a GPU pick at the cursor position described by @p input.
     *
     * Camera matrices are obtained from the cached SceneFrameState.
     * @param input Pick parameters (cursor position, action).
     */
    virtual void processPicking(const PickingInput& input) = 0;

    /**
     * @brief Update hover highlight at the given pixel position.
     *
     * Camera matrices are obtained from the cached SceneFrameState.
     * @param pixel_x Pixel X in device coordinates.
     * @param pixel_y Pixel Y in device coordinates.
     */
    virtual void processHover(int pixel_x, int pixel_y) = 0;

    /**
     * @brief Render a full frame using cached SceneFrameState.
     *
     * Must be called after synchronize(). Uses only the cached state —
     * does not access any GUI-thread singletons.
     */
    virtual void render() = 0;

    /** @brief Release GPU resources. */
    virtual void cleanup() = 0;
};

/** @brief Factory for creating IRenderScene instances. */
class SceneRendererFactory
    : public Kangaroo::Util::FactoryTraits<SceneRendererFactory, IRenderScene> {
public:
    SceneRendererFactory() = default;
    ~SceneRendererFactory() = default;

    /** @brief Create a new IRenderScene instance. */
    virtual tObjectPtr create() = 0;
};
} // namespace OpenGeoLab::Render