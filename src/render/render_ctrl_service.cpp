/**
 * @file render_service.cpp
 * @brief Implementation of RenderCtrlService for OpenGL scene management
 */

#include "render/render_ctrl_service.hpp"
#include "geometry/geometry_document_manager.hpp"
#include "util/logger.hpp"

#include <QCoreApplication>
#include <QMetaObject>
#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::Render {

RenderCtrlService& RenderCtrlService::instance() {
    static RenderCtrlService s_instance;
    return s_instance;
}
// =============================================================================
// CameraState Implementation
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

void CameraState::reset() {
    m_position = QVector3D(0.0f, 0.0f, 50.0f);
    m_target = QVector3D(0.0f, 0.0f, 0.0f);
    m_up = QVector3D(0.0f, 1.0f, 0.0f);
    m_fov = 45.0f;
    LOG_TRACE("CameraState: Reset to default position");
}

void CameraState::fitToBoundingBox(const Geometry::BoundingBox3D& bbox) {
    if(!bbox.isValid()) {
        LOG_DEBUG("CameraState: Invalid bounding box, resetting camera");
        reset();
        return;
    }

    // Calculate bounding box center and size
    const auto center = bbox.center();
    m_target = QVector3D(static_cast<float>(center.m_x), static_cast<float>(center.m_y),
                         static_cast<float>(center.m_z));

    // Calculate required distance to fit the bounding box
    const double diagonal = bbox.diagonal();
    const float distance = static_cast<float>(diagonal * 1.5);

    // Position camera along the Z axis looking at the center
    m_position = m_target + QVector3D(0.0f, 0.0f, distance);
    m_up = QVector3D(0.0f, 1.0f, 0.0f);

    // Adjust near/far planes based on scene size
    m_nearPlane = std::max(0.1f, distance * 0.01f);
    m_farPlane = distance * 10.0f;

    LOG_DEBUG("CameraState: Fit to bounding box, distance={:.2f}", distance);
}

// =============================================================================
// RenderCtrlService Implementation
// =============================================================================

RenderCtrlService::RenderCtrlService() {
    LOG_TRACE("RenderCtrlService created");
    subscribeToCurrentDocument();
}

RenderCtrlService::~RenderCtrlService() { LOG_TRACE("RenderCtrlService destroyed"); }

bool RenderCtrlService::hasGeometry() const { return m_hasGeometry; }

bool RenderCtrlService::needsDefaultGeometry() const { return !m_hasGeometry; }

const DocumentRenderData& RenderCtrlService::renderData() const { return m_renderData; }

CameraState& RenderCtrlService::camera() { return m_camera; }

const CameraState& RenderCtrlService::camera() const { return m_camera; }

void RenderCtrlService::setCamera(const CameraState& camera) {
    m_camera = camera;
    m_cameraChanged.emitSignal();
    m_sceneNeedsUpdate.emitSignal();
}

void RenderCtrlService::refreshScene() {
    LOG_DEBUG("RenderCtrlService: Refreshing scene");
    updateRenderData();
    m_geometryChanged.emitSignal();
    m_sceneNeedsUpdate.emitSignal();
}

void RenderCtrlService::fitToScene() {
    if(m_renderData.isEmpty()) {
        m_camera.reset();
    } else {
        m_camera.fitToBoundingBox(m_renderData.m_boundingBox);
    }
    m_cameraChanged.emitSignal();
}

void RenderCtrlService::resetCamera() {
    m_camera.reset();
    m_cameraChanged.emitSignal();
}

void RenderCtrlService::createDefaultGeometry() {
    LOG_INFO("RenderCtrlService: Creating default box geometry");

    try {
        // Get the document manager and create a default box
        auto manager_factory =
            g_ComponentFactory.getInstanceObject<Geometry::IGeoDocumentManagerSingletonFactory>();
        if(!manager_factory) {
            LOG_ERROR("RenderCtrlService: Failed to get document manager factory");
            return;
        }

        auto document = manager_factory->currentDocument();
        if(!document) {
            LOG_ERROR("RenderCtrlService: Failed to get current document");
            return;
        }

        // Build a simple box using OCC
        // This will be handled by the create action through the normal service path
        // For now, we just trigger a refresh to pick up any existing geometry
        refreshScene();

        if(!m_hasGeometry) {
            // If still no geometry, the create action should be triggered from QML
            LOG_DEBUG("RenderCtrlService: No geometry found, default should be created via action");
        }
    } catch(const std::exception& e) {
        LOG_ERROR("RenderCtrlService: Exception creating default geometry: {}", e.what());
    }
}

void RenderCtrlService::setFrontView() {
    LOG_DEBUG("RenderCtrlService: Setting front view");
    // Camera looks along -Z axis at the target
    const float distance = (m_camera.m_position - m_camera.m_target).length();
    m_camera.m_position = m_camera.m_target + QVector3D(0.0f, 0.0f, distance);
    m_camera.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    m_cameraChanged.emitSignal();
}

void RenderCtrlService::setTopView() {
    LOG_DEBUG("RenderCtrlService: Setting top view");
    // Camera looks along -Y axis at the target
    const float distance = (m_camera.m_position - m_camera.m_target).length();
    m_camera.m_position = m_camera.m_target + QVector3D(0.0f, distance, 0.0f);
    m_camera.m_up = QVector3D(0.0f, 0.0f, -1.0f);
    m_cameraChanged.emitSignal();
}

void RenderCtrlService::setLeftView() {
    LOG_DEBUG("RenderCtrlService: Setting left view");
    // Camera looks along +X axis at the target
    const float distance = (m_camera.m_position - m_camera.m_target).length();
    m_camera.m_position = m_camera.m_target + QVector3D(-distance, 0.0f, 0.0f);
    m_camera.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    m_cameraChanged.emitSignal();
}

void RenderCtrlService::setRightView() {
    LOG_DEBUG("RenderCtrlService: Setting right view");
    // Camera looks along -X axis at the target
    const float distance = (m_camera.m_position - m_camera.m_target).length();
    m_camera.m_position = m_camera.m_target + QVector3D(distance, 0.0f, 0.0f);
    m_camera.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    m_cameraChanged.emitSignal();
}

void RenderCtrlService::setBackView() {
    LOG_DEBUG("RenderCtrlService: Setting back view");
    // Camera looks along +Z axis at the target
    const float distance = (m_camera.m_position - m_camera.m_target).length();
    m_camera.m_position = m_camera.m_target + QVector3D(0.0f, 0.0f, -distance);
    m_camera.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    m_cameraChanged.emitSignal();
}
void RenderCtrlService::setBottomView() {
    LOG_DEBUG("RenderCtrlService: Setting bottom view");
    // Camera looks along +Y axis at the target
    const float distance = (m_camera.m_position - m_camera.m_target).length();
    m_camera.m_position = m_camera.m_target + QVector3D(0.0f, -distance, 0.0f);
    m_camera.m_up = QVector3D(0.0f, 0.0f, 1.0f);
    m_cameraChanged.emitSignal();
}

Util::ScopedConnection RenderCtrlService::subscribeGeometryChanged(std::function<void()> callback) {
    return m_geometryChanged.connect(std::move(callback));
}

Util::ScopedConnection RenderCtrlService::subscribeCameraChanged(std::function<void()> callback) {
    return m_cameraChanged.connect(std::move(callback));
}

Util::ScopedConnection
RenderCtrlService::subscribeSceneNeedsUpdate(std::function<void()> callback) {
    return m_sceneNeedsUpdate.connect(std::move(callback));
}

void RenderCtrlService::handleDocumentGeometryChanged(const Geometry::GeometryChangeEvent& event) {
    LOG_DEBUG("RenderCtrlService: Document geometry changed, type={}",
              static_cast<int>(event.m_type));
    updateRenderData();
    LOG_DEBUG("RenderCtrlService: After updateRenderData, hasGeometry={}", m_hasGeometry);
    m_geometryChanged.emitSignal();
    m_sceneNeedsUpdate.emitSignal();
}

void RenderCtrlService::subscribeToCurrentDocument() {
    try {
        auto manager_factory =
            g_ComponentFactory.getInstanceObject<Geometry::IGeoDocumentManagerSingletonFactory>();
        if(!manager_factory) {
            LOG_WARN("Document manager factory not available");
            return;
        }

        auto document = manager_factory->currentDocument();
        if(!document) {
            LOG_WARN("No current document available");
            return;
        }

        // Subscribe to changes
        m_documentConnection =
            document->subscribeToChanges([this](const Geometry::GeometryChangeEvent& event) {
                // Keep UI-thread semantics: queue handling via QCoreApplication
                if(auto* app = QCoreApplication::instance()) {
                    QMetaObject::invokeMethod(
                        app, [this, event]() { handleDocumentGeometryChanged(event); },
                        Qt::QueuedConnection);
                } else {
                    handleDocumentGeometryChanged(event);
                }
            });

        LOG_DEBUG("RenderCtrlService: Subscribed to document changes");
    } catch(const std::exception& e) {
        LOG_ERROR("RenderCtrlService: Exception subscribing to document: {}", e.what());
    }
}

void RenderCtrlService::updateRenderData() {
    try {
        auto manager_factory =
            g_ComponentFactory.getInstanceObject<Geometry::IGeoDocumentManagerSingletonFactory>();
        if(!manager_factory) {
            LOG_WARN("RenderCtrlService: Document manager factory not available during update");
            m_renderData.clear();
            m_hasGeometry = false;
            return;
        }

        auto document = manager_factory->currentDocument();
        if(!document) {
            LOG_WARN("RenderCtrlService: No current document available during update");
            m_renderData.clear();
            m_hasGeometry = false;
            return;
        }

        // Get render data with default tessellation options
        m_renderData = document->getRenderData(TessellationOptions::defaultOptions());
        m_renderData.markModified(); // Increment version to trigger viewport update
        m_hasGeometry = !m_renderData.isEmpty();

        LOG_DEBUG(
            "RenderCtrlService: Updated render data, meshCount={}, hasGeometry={}, version={}",
            m_renderData.meshCount(), m_hasGeometry, m_renderData.m_version);
    } catch(const std::exception& e) {
        LOG_ERROR("RenderCtrlService: Exception updating render data: {}", e.what());
        m_renderData.clear();
        m_hasGeometry = false;
    }
}

} // namespace OpenGeoLab::Render
