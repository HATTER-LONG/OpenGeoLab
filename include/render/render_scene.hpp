/**
 * @file render_scene.hpp
 * @brief Abstract render-scene interface and its factory.
 */

#pragma once

#include "render/render_types.hpp"

#include <QMatrix4x4>
#include <QPointF>
#include <QSizeF>

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Render {

/**
 * @brief Input parameters for pixel picking
 *
 * Encapsulates all information needed to perform a pick operation,
 * including cursor position, viewport geometry, and camera matrices.
 */
struct PickingInput {
    QPointF m_cursorPos;                   ///< Cursor position in item coordinates
    QSizeF m_itemSize;                     ///< Item size in logical pixels
    qreal m_devicePixelRatio{1.0};         ///< Device pixel ratio (HiDPI scaling)
    QMatrix4x4 m_viewMatrix;               ///< View transformation matrix
    QMatrix4x4 m_projectionMatrix;         ///< Projection transformation matrix
    PickAction m_action{PickAction::None}; ///< Pending pick action
};

/**
 * @brief Abstract interface for an OpenGL render scene.
 *
 * Manages the full render lifecycle: initialization, per-frame rendering,
 * GPU picking, hover highlighting, and cleanup.
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
     * @brief Execute a GPU pick at the cursor position described by @p input.
     * @param input Pick parameters (cursor position, matrices, action).
     */
    virtual void processPicking(const PickingInput& input) = 0;

    /**
     * @brief Update hover highlight at the given pixel position.
     * @param pixel_x Pixel X in device coordinates.
     * @param pixel_y Pixel Y in device coordinates.
     * @param view Current view matrix.
     * @param projection Current projection matrix.
     */
    virtual void processHover(int pixel_x,
                              int pixel_y,
                              const QMatrix4x4& view,
                              const QMatrix4x4& projection) = 0;

    /**
     * @brief Render a full frame.
     * @param camera_pos Camera world-space position (used for lighting).
     * @param view_matrix View transformation matrix.
     * @param projection_matrix Projection transformation matrix.
     */
    virtual void render(const QVector3D& camera_pos,
                        const QMatrix4x4& view_matrix,
                        const QMatrix4x4& projection_matrix) = 0;

    /** @brief Release all GPU resources. */
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