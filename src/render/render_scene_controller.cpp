#include "render/render_scene_controller.hpp"
#include "mesh/mesh_document.hpp"
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
// =============================================================================
// RenderSceneController
// =============================================================================
RenderSceneController& RenderSceneController::instance() {
    static RenderSceneController instance;
    return instance;
}

RenderSceneController::RenderSceneController() {
    LOG_TRACE("RenderSceneController: Initialized");
    subscribeToGeometryDocument();
    subscribeToMeshDocument();
    updateGeometryRenderData();
    updateMeshRenderData();
}

RenderSceneController::~RenderSceneController() {
    LOG_TRACE("RenderSceneController: Destroyed");
    // TODO(Layton): detach signals
}

void RenderSceneController::subscribeToGeometryDocument() {
    auto document = GeoDocumentInstance;
    m_geometryDocumentConnection =
        document->subscribeToChanges([this](const Geometry::GeometryChangeEvent& event) {
            handleDocumentGeometryChanged(event);
        });
}

void RenderSceneController::handleDocumentGeometryChanged(
    const Geometry::GeometryChangeEvent& event) {
    LOG_DEBUG("RenderSceneController: Geometry document changed, type={}",
              static_cast<int>(event.m_type));
    updateGeometryRenderData();
    m_sceneNeedsUpdate.emitSignal();
}

void RenderSceneController::updateGeometryRenderData() {
    auto document = GeoDocumentInstance;

    auto render_data = document->getRenderData(TessellationOptions::defaultOptions());
}

void RenderSceneController::subscribeToMeshDocument() {
    auto document = MeshDocumentInstance;
    m_meshDocumentConnection =
        document->subscribeToChanges([this]() { handleDocumentMeshChanged(); });
}

void RenderSceneController::handleDocumentMeshChanged() {
    LOG_DEBUG("RenderSceneController: Mesh document changed");
    updateMeshRenderData();
    m_sceneNeedsUpdate.emitSignal();
}

void RenderSceneController::updateMeshRenderData() {
    auto document = MeshDocumentInstance;
    auto render_data = document->getRenderData();
}

void RenderSceneController::setCamera(const CameraState& camera, bool notify) {
    m_cameraState = camera;
    if(notify) {
        m_sceneNeedsUpdate.emitSignal();
    }
}

void RenderSceneController::refreshScene(bool notify) {
    LOG_DEBUG("RenderSceneController: Refreshing scene");
    updateGeometryRenderData();
    updateMeshRenderData();
    if(notify) {
        m_sceneNeedsUpdate.emitSignal();
    }
}

void RenderSceneController::fitToScene(bool notify) {
    if(notify) {
        m_sceneNeedsUpdate.emitSignal();
    }
}
void RenderSceneController::resetCamera(bool notify) {
    m_cameraState.reset();
    if(notify) {
        m_sceneNeedsUpdate.emitSignal();
    }
}

void RenderSceneController::setFrontView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting front view");
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    m_cameraState.m_position = m_cameraState.m_target + QVector3D(0.0f, 0.0f, distance);
    m_cameraState.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    if(notify) {
        m_sceneNeedsUpdate.emitSignal();
    }
}

void RenderSceneController::setTopView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting top view");
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    m_cameraState.m_position = m_cameraState.m_target + QVector3D(0.0f, distance, 0.0f);
    m_cameraState.m_up = QVector3D(0.0f, 0.0f, -1.0f);
    if(notify) {
        m_sceneNeedsUpdate.emitSignal();
    }
}

void RenderSceneController::setLeftView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting left view");
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    m_cameraState.m_position = m_cameraState.m_target + QVector3D(-distance, 0.0f, 0.0f);
    m_cameraState.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    if(notify) {
        m_sceneNeedsUpdate.emitSignal();
    }
}

void RenderSceneController::setRightView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting right view");
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    m_cameraState.m_position = m_cameraState.m_target + QVector3D(distance, 0.0f, 0.0f);
    m_cameraState.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    if(notify) {
        m_sceneNeedsUpdate.emitSignal();
    }
}

void RenderSceneController::setBackView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting back view");
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    m_cameraState.m_position = m_cameraState.m_target + QVector3D(0.0f, 0.0f, -distance);
    m_cameraState.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    if(notify) {
        m_sceneNeedsUpdate.emitSignal();
    }
}

void RenderSceneController::setBottomView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting bottom view");
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    m_cameraState.m_position = m_cameraState.m_target + QVector3D(0.0f, -distance, 0.0f);
    m_cameraState.m_up = QVector3D(0.0f, 0.0f, 1.0f);
    if(notify) {
        m_sceneNeedsUpdate.emitSignal();
    }
}
// =============================================================================
// Per-part visibility
// =============================================================================

void RenderSceneController::setPartGeometryVisible(Geometry::EntityUID part_uid, bool visible) {
    {
        std::lock_guard lock(m_visibilityMutex);
        m_partVisibility[part_uid].m_geometryVisible = visible;
    }
    m_sceneNeedsUpdate.emitSignal();
}

void RenderSceneController::setPartMeshVisible(Geometry::EntityUID part_uid, bool visible) {
    {
        std::lock_guard lock(m_visibilityMutex);
        m_partVisibility[part_uid].m_meshVisible = visible;
    }
    m_sceneNeedsUpdate.emitSignal();
}

bool RenderSceneController::isPartGeometryVisible(Geometry::EntityUID part_uid) const {
    std::lock_guard lock(m_visibilityMutex);
    auto it = m_partVisibility.find(part_uid);
    return it == m_partVisibility.end() || it->second.m_geometryVisible;
}

bool RenderSceneController::isPartMeshVisible(Geometry::EntityUID part_uid) const {
    std::lock_guard lock(m_visibilityMutex);
    auto it = m_partVisibility.find(part_uid);
    return it == m_partVisibility.end() || it->second.m_meshVisible;
}
} // namespace OpenGeoLab::Render