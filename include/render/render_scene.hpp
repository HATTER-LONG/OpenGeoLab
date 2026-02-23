#pragma once
#include "render/render_data.hpp"

#include <QMatrix4x4>
#include <QPointF>
#include <QSizeF>
#include <cstdint>

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>

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

class IRenderScene : public Kangaroo::Util::NonCopyMoveable {
public:
    IRenderScene() = default;
    virtual ~IRenderScene() = default;

    virtual void initialize() = 0;

    [[nodiscard]] virtual bool isInitialized() const = 0;

    virtual void setViewportSize(const QSize& size) = 0;

    virtual void uploadMeshData(const DocumentRenderData& data) = 0;

    virtual void processPicking(const PickingInput& input) = 0;

    virtual void render(const QVector3D& camera_pos,
                        const QMatrix4x4& view_matrix,
                        const QMatrix4x4& projection_matrix) = 0;

    virtual void cleanup() = 0;
};

class SceneRendererFactory
    : public Kangaroo::Util::FactoryTraits<SceneRendererFactory, IRenderScene> {
public:
    SceneRendererFactory() = default;
    ~SceneRendererFactory() = default;

    virtual tObjectPtr create() = 0;
};
} // namespace OpenGeoLab::Render