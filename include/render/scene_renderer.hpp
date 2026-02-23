/**
 * @file scene_renderer.hpp
 * @brief Abstract scene renderer interface for OpenGL rendering
 *
 * ISceneRenderer defines the public API for the rendering pipeline.
 * All internal details (render passes, highlight strategies, renderer core)
 * are hidden behind this interface. Concrete implementations are created
 * via SceneRendererFactory.
 */

#pragma once

#include "render/render_data.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>

#include <QMatrix4x4>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <QVector3D>
#include <cstdint>
#include <memory>

namespace OpenGeoLab::Render {

/**
 * @brief Action requested by the viewport for entity picking
 */
enum class PickAction : uint8_t {
    None = 0,   ///< No action
    Add = 1,    ///< Add entity to selection
    Remove = 2, ///< Remove entity from selection
};

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
 * @brief Abstract interface for the scene rendering pipeline
 *
 * Provides a clean public API for viewport rendering, data upload,
 * and picking. All internal implementation details (render passes,
 * shader management, highlight strategies) are hidden.
 *
 * Create instances via SceneRendererFactory.
 */
class ISceneRenderer : public Kangaroo::Util::NonCopyMoveable {
public:
    ISceneRenderer() = default;
    virtual ~ISceneRenderer() = default;

    /**
     * @brief Initialize OpenGL resources and the rendering pipeline
     * @note Must be called with a valid OpenGL context current
     */
    virtual void initialize() = 0;

    /**
     * @brief Check whether the renderer has been initialized
     */
    [[nodiscard]] virtual bool isInitialized() const = 0;

    /**
     * @brief Update the viewport size
     * @param size New viewport size in pixels
     */
    virtual void setViewportSize(const QSize& size) = 0;

    /**
     * @brief Upload render data to GPU buffers
     * @param render_data Document render data to upload
     */
    virtual void uploadMeshData(const DocumentRenderData& render_data) = 0;

    /**
     * @brief Process pixel picking for hover and selection
     *
     * Internally handles:
     * - Pick-enable check via SelectManager
     * - Rendering the pick pass
     * - Reading and decoding pick pixels
     * - Owner entity mapping (Face -> Part/Solid/Wire)
     * - Hover highlight updates
     * - Selection add/remove via SelectManager
     *
     * @param input Picking parameters (cursor, viewport, matrices, action)
     */
    virtual void processPicking(const PickingInput& input) = 0;

    /**
     * @brief Render the complete scene
     * @param camera_pos Camera position in world space
     * @param view_matrix View transformation matrix
     * @param projection_matrix Projection transformation matrix
     */
    virtual void render(const QVector3D& camera_pos,
                        const QMatrix4x4& view_matrix,
                        const QMatrix4x4& projection_matrix) = 0;

    /**
     * @brief Release all GPU resources
     */
    virtual void cleanup() = 0;
};

/**
 * @brief Factory for creating ISceneRenderer instances
 *
 * Registered in g_ComponentFactory. Create instances via:
 * @code
 * auto renderer = g_ComponentFactory.createObjectWithID<SceneRendererFactory>("SceneRenderer");
 * @endcode
 */
class SceneRendererFactory
    : public Kangaroo::Util::FactoryTraits<SceneRendererFactory, ISceneRenderer> {
public:
    SceneRendererFactory() = default;
    ~SceneRendererFactory() = default;

    /** @brief Create a new SceneRenderer instance. @return Unique pointer to the implementation. */
    virtual tObjectPtr create() = 0;
};

/**
 * @brief Register SceneRendererFactory into the global component factory
 * @note Called once during application startup (typically from registerServices)
 */
void registerSceneRendererFactory();

} // namespace OpenGeoLab::Render
