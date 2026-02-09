/**
 * @file render_scene_controller.cpp
 * @brief Implementation of RenderSceneController
 */

#include "render/render_scene_controller.hpp"

#include "geometry/geometry_document_manager.hpp"
#include "util/logger.hpp"

#include <QCoreApplication>
#include <QMetaObject>
#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::Render {

RenderSceneController& RenderSceneController::instance() {
    static RenderSceneController s_instance;
    return s_instance;
}

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

RenderSceneController::RenderSceneController() {
    LOG_TRACE("RenderSceneController created");
    subscribeToCurrentDocument();
    updateRenderData();
}

RenderSceneController::~RenderSceneController() { LOG_TRACE("RenderSceneController destroyed"); }

bool RenderSceneController::hasGeometry() const { return m_hasGeometry; }

const DocumentRenderData& RenderSceneController::renderData() const { return m_renderData; }

Geometry::GeometryDocumentPtr RenderSceneController::currentDocument() const {
    return m_currentDocument;
}

CameraState& RenderSceneController::camera() { return m_camera; }

const CameraState& RenderSceneController::camera() const { return m_camera; }

void RenderSceneController::setCamera(const CameraState& camera, bool notify) {
    m_camera = camera;
    if(notify) {
        m_cameraChanged.emitSignal();
    }
}
void RenderSceneController::refreshScene(bool notify) {
    LOG_DEBUG("RenderSceneController: Refreshing scene");
    updateRenderData();
    if(notify) {
        m_geometryChanged.emitSignal();
        m_sceneNeedsUpdate.emitSignal();
    }
}

void RenderSceneController::fitToScene(bool notify) {
    if(m_renderData.isEmpty()) {
        m_camera.reset();
    } else {
        m_camera.fitToBoundingBox(m_renderData.m_boundingBox);
    }
    if(notify) {
        m_cameraChanged.emitSignal();
    }
}

void RenderSceneController::resetCamera(bool notify) {
    m_camera.reset();
    if(notify) {
        m_cameraChanged.emitSignal();
    }
}

void RenderSceneController::setFrontView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting front view");
    const float distance = (m_camera.m_position - m_camera.m_target).length();
    m_camera.m_position = m_camera.m_target + QVector3D(0.0f, 0.0f, distance);
    m_camera.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    if(notify) {
        m_cameraChanged.emitSignal();
    }
}

void RenderSceneController::setTopView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting top view");
    const float distance = (m_camera.m_position - m_camera.m_target).length();
    m_camera.m_position = m_camera.m_target + QVector3D(0.0f, distance, 0.0f);
    m_camera.m_up = QVector3D(0.0f, 0.0f, -1.0f);
    if(notify) {
        m_cameraChanged.emitSignal();
    }
}

void RenderSceneController::setLeftView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting left view");
    const float distance = (m_camera.m_position - m_camera.m_target).length();
    m_camera.m_position = m_camera.m_target + QVector3D(-distance, 0.0f, 0.0f);
    m_camera.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    if(notify) {
        m_cameraChanged.emitSignal();
    }
}

void RenderSceneController::setRightView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting right view");
    const float distance = (m_camera.m_position - m_camera.m_target).length();
    m_camera.m_position = m_camera.m_target + QVector3D(distance, 0.0f, 0.0f);
    m_camera.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    if(notify) {
        m_cameraChanged.emitSignal();
    }
}

void RenderSceneController::setBackView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting back view");
    const float distance = (m_camera.m_position - m_camera.m_target).length();
    m_camera.m_position = m_camera.m_target + QVector3D(0.0f, 0.0f, -distance);
    m_camera.m_up = QVector3D(0.0f, 1.0f, 0.0f);
    if(notify) {
        m_cameraChanged.emitSignal();
    }
}

void RenderSceneController::setBottomView(bool notify) {
    LOG_DEBUG("RenderSceneController: Setting bottom view");
    const float distance = (m_camera.m_position - m_camera.m_target).length();
    m_camera.m_position = m_camera.m_target + QVector3D(0.0f, -distance, 0.0f);
    m_camera.m_up = QVector3D(0.0f, 0.0f, 1.0f);
    if(notify) {
        m_cameraChanged.emitSignal();
    }
}

Util::ScopedConnection
RenderSceneController::subscribeGeometryChanged(std::function<void()> callback) {
    return m_geometryChanged.connect(std::move(callback));
}

Util::ScopedConnection
RenderSceneController::subscribeCameraChanged(std::function<void()> callback) {
    return m_cameraChanged.connect(std::move(callback));
}

Util::ScopedConnection
RenderSceneController::subscribeSceneNeedsUpdate(std::function<void()> callback) {
    return m_sceneNeedsUpdate.connect(std::move(callback));
}

void RenderSceneController::handleDocumentGeometryChanged(
    const Geometry::GeometryChangeEvent& event) {
    LOG_DEBUG("RenderSceneController: Document geometry changed, type={}",
              static_cast<int>(event.m_type));
    updateRenderData();
    m_geometryChanged.emitSignal();
    m_sceneNeedsUpdate.emitSignal();
}

void RenderSceneController::subscribeToCurrentDocument() {
    try {
        auto manager = GeoDocumentMgrInstance;
        if(!manager) {
            LOG_WARN("RenderSceneController: Document manager not available");
            return;
        }

        subscribeToDocument(manager->currentDocument());
    } catch(const std::exception& e) {
        LOG_ERROR("RenderSceneController: Exception subscribing to document: {}", e.what());
    }
}

void RenderSceneController::subscribeToDocument(const Geometry::GeometryDocumentPtr& document) {
    if(!document) {
        LOG_WARN("RenderSceneController: No current document available");
        m_currentDocument.reset();
        m_documentConnection = {};
        return;
    }

    if(m_currentDocument == document) {
        return;
    }

    m_currentDocument = document;

    m_documentConnection =
        document->subscribeToChanges([this](const Geometry::GeometryChangeEvent& event) {
            if(auto* app = QCoreApplication::instance()) {
                QMetaObject::invokeMethod(
                    app, [this, event]() { handleDocumentGeometryChanged(event); },
                    Qt::QueuedConnection);
            } else {
                handleDocumentGeometryChanged(event);
            }
        });

    LOG_DEBUG("RenderSceneController: Subscribed to document changes");
}

void RenderSceneController::updateRenderData() {
    try {
        auto manager = GeoDocumentMgrInstance;
        if(!manager) {
            LOG_WARN("RenderSceneController: Document manager not available during update");
            m_renderData.clear();
            m_hasGeometry = false;
            return;
        }

        auto document = manager->currentDocument();
        subscribeToDocument(document);
        if(!document) {
            LOG_WARN("RenderSceneController: No current document available during update");
            m_renderData.clear();
            m_hasGeometry = false;
            return;
        }

        m_renderData = document->getRenderData(TessellationOptions::defaultOptions());
        m_renderData.markModified();
        m_hasGeometry = !m_renderData.isEmpty();

        LOG_DEBUG(
            "RenderSceneController: Updated render data, meshCount={}, hasGeometry={}, version={}",
            m_renderData.meshCount(), m_hasGeometry, m_renderData.m_version);
    } catch(const std::exception& e) {
        LOG_ERROR("RenderSceneController: Exception updating render data: {}", e.what());
        m_renderData.clear();
        m_hasGeometry = false;
    }
}

} // namespace OpenGeoLab::Render
