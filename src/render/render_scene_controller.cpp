/**
 * @file render_scene_controller.cpp
 * @brief RenderSceneController — singleton that bridges document data
 *        (geometry/mesh) with the render pipeline, camera state, and
 *        per-part visibility.
 */

#include "render/render_scene_controller.hpp"
#include "geometry/geometry_document.hpp"
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
    const float distance = (m_position - m_target).length();

    // Orthographic: visible half-height is proportional to camera distance
    const float halfHeight = distance * 0.5f;
    const float halfWidth = halfHeight * aspect_ratio;
    projection.ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, m_nearPlane, m_farPlane);
    return projection;
}

void CameraState::updateClipping(float distance) {
    // Orthographic projection: use a symmetric depth range centered at the
    // camera so geometry that extends behind the camera (e.g. when zoomed in
    // very close to the model) is not clipped. Linear depth distribution in
    // orthographic projection preserves depth buffer precision even with a
    // wide range.
    const float d = std::max(distance, 1e-4f);
    const float half_range = d * 10.0f;
    m_nearPlane = -half_range;
    m_farPlane = half_range;
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
    m_sceneNeedsUpdate.emitSignal(SceneUpdateType::GeometryChanged);
}

void RenderSceneController::updateGeometryRenderData() {
    auto document = GeoDocumentInstance;

    auto default_options = Render::TessellationOptions::defaultOptions();
    auto ret = document->getRenderData(m_renderData, default_options);
    if(!ret) {
        LOG_ERROR("RenderSceneController: Failed to get geometry render data");
    }

    // Apply per-part visibility from controller state
    for(auto& root : m_renderData.m_roots) {
        if(root.m_key.m_type == RenderEntityType::Part) {
            std::lock_guard lock(m_visibilityMutex);
            auto it = m_partVisibility.find(root.m_key.m_uid);
            if(it != m_partVisibility.end()) {
                root.m_visible = it->second.m_geometryVisible;
            }
        }
    }
}

void RenderSceneController::subscribeToMeshDocument() {
    auto document = MeshDocumentInstance;
    m_meshDocumentConnection =
        document->subscribeToChanges([this]() { handleDocumentMeshChanged(); });
}

void RenderSceneController::handleDocumentMeshChanged() {
    LOG_DEBUG("RenderSceneController: Mesh document changed");
    updateMeshRenderData();
    m_sceneNeedsUpdate.emitSignal(SceneUpdateType::MeshChanged);
}

void RenderSceneController::updateMeshRenderData() {
    auto document = MeshDocumentInstance;

    const bool ret = document->getRenderData(m_renderData);
    if(!ret) {
        LOG_ERROR("RenderSceneController: Failed to get mesh render data");
    }
}

void RenderSceneController::setCamera(const CameraState& camera, bool notify) {
    m_cameraState = camera;
    if(notify) {
        m_sceneNeedsUpdate.emitSignal(SceneUpdateType::CameraChanged);
    }
}

void RenderSceneController::refreshScene(bool notify) {
    LOG_DEBUG("RenderSceneController: Refreshing scene");
    updateGeometryRenderData();
    updateMeshRenderData();
    if(notify) {
        m_sceneNeedsUpdate.emitSignal(SceneUpdateType::GeometryChanged);
    }
}

void RenderSceneController::fitToScene(bool notify) {
    if(m_renderData.m_sceneBBox.isValid()) {
        m_cameraState.fitToBoundingBox(m_renderData.m_sceneBBox);
    }
    if(notify) {
        m_sceneNeedsUpdate.emitSignal(SceneUpdateType::GeometryChanged);
    }
}
void RenderSceneController::resetCamera(bool notify) {
    m_cameraState.reset();
    if(notify) {
        m_sceneNeedsUpdate.emitSignal(SceneUpdateType::CameraChanged);
    }
}

void RenderSceneController::setFrontView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting front view");
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    m_cameraState.m_position = m_cameraState.m_target + QVector3D(0.0f, 0.0f, distance);
    m_cameraState.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    if(notify) {
        m_sceneNeedsUpdate.emitSignal(SceneUpdateType::CameraChanged);
    }
}

void RenderSceneController::setTopView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting top view");
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    m_cameraState.m_position = m_cameraState.m_target + QVector3D(0.0f, distance, 0.0f);
    m_cameraState.m_up = QVector3D(0.0f, 0.0f, -1.0f);
    if(notify) {
        m_sceneNeedsUpdate.emitSignal(SceneUpdateType::CameraChanged);
    }
}

void RenderSceneController::setLeftView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting left view");
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    m_cameraState.m_position = m_cameraState.m_target + QVector3D(-distance, 0.0f, 0.0f);
    m_cameraState.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    if(notify) {
        m_sceneNeedsUpdate.emitSignal(SceneUpdateType::CameraChanged);
    }
}

void RenderSceneController::setRightView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting right view");
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    m_cameraState.m_position = m_cameraState.m_target + QVector3D(distance, 0.0f, 0.0f);
    m_cameraState.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    if(notify) {
        m_sceneNeedsUpdate.emitSignal(SceneUpdateType::CameraChanged);
    }
}

void RenderSceneController::setBackView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting back view");
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    m_cameraState.m_position = m_cameraState.m_target + QVector3D(0.0f, 0.0f, -distance);
    m_cameraState.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    if(notify) {
        m_sceneNeedsUpdate.emitSignal(SceneUpdateType::CameraChanged);
    }
}

void RenderSceneController::setBottomView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting bottom view");
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    m_cameraState.m_position = m_cameraState.m_target + QVector3D(0.0f, -distance, 0.0f);
    m_cameraState.m_up = QVector3D(0.0f, 0.0f, 1.0f);
    if(notify) {
        m_sceneNeedsUpdate.emitSignal(SceneUpdateType::CameraChanged);
    }
}
void RenderSceneController::toggleXRayMode(bool notify) {
    const bool next = !m_xRayMode.load(std::memory_order_relaxed);
    m_xRayMode.store(next, std::memory_order_release);
    LOG_DEBUG("RenderSceneController: X-ray mode {}", next ? "enabled" : "disabled");
    if(notify) {
        m_sceneNeedsUpdate.emitSignal(SceneUpdateType::GeometryChanged);
    }
}

bool RenderSceneController::isXRayMode() const noexcept {
    return m_xRayMode.load(std::memory_order_acquire);
}
void RenderSceneController::cycleMeshDisplayMode(bool notify) {
    constexpr uint8_t WIREFRAME_POINTS =
        static_cast<uint8_t>(RenderDisplayModeMask::Wireframe | RenderDisplayModeMask::Points);
    constexpr uint8_t SURFACE_POINTS =
        static_cast<uint8_t>(RenderDisplayModeMask::Surface | RenderDisplayModeMask::Points);
    constexpr uint8_t SURFACE_POINTS_WIRE =
        static_cast<uint8_t>(RenderDisplayModeMask::Surface | RenderDisplayModeMask::Points |
                             RenderDisplayModeMask::Wireframe);

    const uint8_t current = m_meshDisplayMode.load(std::memory_order_relaxed);
    uint8_t next = WIREFRAME_POINTS;
    if(current == WIREFRAME_POINTS) {
        next = SURFACE_POINTS;
    } else if(current == SURFACE_POINTS) {
        next = SURFACE_POINTS_WIRE;
    } else {
        next = WIREFRAME_POINTS;
    }
    m_meshDisplayMode.store(next, std::memory_order_release);

    LOG_DEBUG("RenderSceneController: Mesh display mode → {}",
              next == WIREFRAME_POINTS ? "wireframe+points"
              : next == SURFACE_POINTS ? "surface+points"
                                       : "surface+points+wireframe");

    if(notify) {
        m_sceneNeedsUpdate.emitSignal(SceneUpdateType::MeshChanged);
    }
}

RenderDisplayModeMask RenderSceneController::meshDisplayMode() const noexcept {
    return static_cast<RenderDisplayModeMask>(m_meshDisplayMode.load(std::memory_order_acquire));
}
// =============================================================================
// Render data access
// =============================================================================

const RenderData& RenderSceneController::renderData() const { return m_renderData; }

Geometry::GeometryDocumentPtr RenderSceneController::currentGeometryDocument() const {
    return GeoDocumentInstance;
}
// =============================================================================
// Per-part visibility
// =============================================================================

void RenderSceneController::setPartGeometryVisible(Geometry::EntityUID part_uid, bool visible) {
    {
        std::lock_guard lock(m_visibilityMutex);
        m_partVisibility[part_uid].m_geometryVisible = visible;
    }
    m_sceneNeedsUpdate.emitSignal(SceneUpdateType::GeometryChanged);
}

void RenderSceneController::setPartMeshVisible(Geometry::EntityUID part_uid, bool visible) {
    {
        std::lock_guard lock(m_visibilityMutex);
        m_partVisibility[part_uid].m_meshVisible = visible;
    }
    m_sceneNeedsUpdate.emitSignal(SceneUpdateType::MeshChanged);
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