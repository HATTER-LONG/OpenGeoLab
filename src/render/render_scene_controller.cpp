#include "render/render_scene_controller.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Render {
// =============================================================================
// CameraState
// =============================================================================

QMatrix4x4 CameraState::viewMatrix() const {
    QMatrix4x4 view;
    view.lookAt(m_position, m_target, m_up);
    return view;
}

QMatrix4x4 CameraState::projectionMatrix(float aspect_ratio) const {
    QMatrix4x4 projection;
    projection.perspective(m_fov, aspect_ratio, m_nearPlane, m_farPlane);
    return projection;
}

void CameraState::updateClipping(float distance) {
    // Keep near plane proportional to distance, but allow very small values for tiny models.
    // This avoids the "clipping/sinking" feeling when zooming close to small geometry.
    const float d = std::max(distance, 1e-4f);
    m_nearPlane = std::max(1e-4f, d * 0.001f);

    // Ensure far plane is sufficiently larger than distance to avoid cutting geometry,
    // and keep a reasonable ratio to preserve depth precision.
    m_farPlane = std::max(d * 20.0f, m_nearPlane * 1000.0f);
}

void CameraState::reset() {
    m_position = QVector3D(0.0f, 0.0f, 50.0f);
    m_target = QVector3D(0.0f, 0.0f, 0.0f);
    m_up = QVector3D(0.0f, 1.0f, 0.0f);
    m_fov = 45.0f;
    updateClipping((m_position - m_target).length());
    LOG_TRACE("CameraState: Reset to default position");
}

void CameraState::fitToBoundingBox(const Geometry::BoundingBox3D& bbox) {
    if(!bbox.isValid()) {
        LOG_DEBUG("CameraState: Invalid bounding box, resetting camera");
        reset();
        return;
    }

    const auto center = bbox.center();
    m_target = QVector3D(static_cast<float>(center.x), static_cast<float>(center.y),
                         static_cast<float>(center.z));

    const double diagonal = bbox.diagonal();
    const float distance = static_cast<float>(diagonal * 1.5);

    m_position = m_target + QVector3D(0.0f, 0.0f, distance);
    m_up = QVector3D(0.0f, 1.0f, 0.0f);

    updateClipping(distance);

    LOG_DEBUG("CameraState: Fit to bounding box, distance={:.2f}", distance);
}
} // namespace OpenGeoLab::Render