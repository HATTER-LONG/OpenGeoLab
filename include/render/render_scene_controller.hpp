#pragma once

#include "geometry/geometry_types.hpp"

#include <QMatrix4x4>
#include <QVector3D>
#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Render {
/**
 * @brief Camera configuration for a 3D viewport
 */
struct CameraState {
    QVector3D m_position{0.0f, 0.0f, 50.0f}; ///< Camera position in world space
    QVector3D m_target{0.0f, 0.0f, 0.0f};    ///< Look-at target point
    QVector3D m_up{0.0f, 1.0f, 0.0f};        ///< Up vector
    float m_fov{45.0f};                      ///< Field of view in degrees
    float m_nearPlane{0.1f};                 ///< Near clipping plane
    float m_farPlane{10000.0f};              ///< Far clipping plane

    /**
     * @brief Update clipping planes based on the current camera distance
     * @param distance Distance between camera position and target
     */
    void updateClipping(float distance);

    /**
     * @brief Build a view matrix
     * @return View transformation matrix
     */
    [[nodiscard]] QMatrix4x4 viewMatrix() const;

    /**
     * @brief Build a perspective projection matrix
     * @param aspect_ratio Viewport aspect ratio (width/height)
     * @return Perspective projection matrix
     */
    [[nodiscard]] QMatrix4x4 projectionMatrix(float aspect_ratio) const;

    /**
     * @brief Reset camera to default configuration
     */
    void reset();

    /**
     * @brief Fit camera to view a bounding box
     * @param bbox Target bounding box
     */
    void fitToBoundingBox(const Geometry::BoundingBox3D& bbox);
};

class RenderSceneController : public Kangaroo::Util::NonCopyMoveable {
public:
    RenderSceneController() = default;
    virtual ~RenderSceneController() = default;

    virtual CameraState& cameraState() { return m_cameraState; }

private:
    CameraState m_cameraState;
};
} // namespace OpenGeoLab::Render