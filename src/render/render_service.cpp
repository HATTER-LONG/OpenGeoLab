/**
 * @file render_service.cpp
 * @brief Implementation of RenderService for OpenGL scene management
 */

#include "render/render_service.hpp"
#include "geometry/geometry_document_manager.hpp"
#include "util/logger.hpp"

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::Render {

// =============================================================================
// CameraState Implementation
// =============================================================================

QMatrix4x4 CameraState::viewMatrix() const {
    QMatrix4x4 view;
    view.lookAt(m_position, m_target, m_up);
    return view;
}

QMatrix4x4 CameraState::projectionMatrix(float aspectRatio) const {
    QMatrix4x4 projection;
    projection.perspective(m_fov, aspectRatio, m_nearPlane, m_farPlane);
    return projection;
}

void CameraState::reset() {
    m_position = QVector3D(0.0f, 0.0f, 50.0f);
    m_target = QVector3D(0.0f, 0.0f, 0.0f);
    m_up = QVector3D(0.0f, 1.0f, 0.0f);
    m_fov = 45.0f;
}

void CameraState::fitToBoundingBox(const Geometry::BoundingBox3D& bbox) {
    if(!bbox.isValid()) {
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
}

// =============================================================================
// RenderService Implementation
// =============================================================================

RenderService::RenderService(QObject* parent) : QObject(parent) {
    LOG_TRACE("RenderService created");
}

RenderService::~RenderService() { LOG_TRACE("RenderService destroyed"); }

bool RenderService::hasGeometry() const { return m_hasGeometry; }

bool RenderService::needsDefaultGeometry() const { return !m_hasGeometry; }

const DocumentRenderData& RenderService::renderData() const { return m_renderData; }

CameraState& RenderService::camera() { return m_camera; }

const CameraState& RenderService::camera() const { return m_camera; }

void RenderService::refreshScene() {
    LOG_DEBUG("RenderService: Refreshing scene");
    subscribeToCurrentDocument();
    updateRenderData();
    emit geometryChanged();
    emit sceneNeedsUpdate();
}

void RenderService::fitToScene() {
    if(m_renderData.isEmpty()) {
        m_camera.reset();
    } else {
        m_camera.fitToBoundingBox(m_renderData.m_boundingBox);
    }
    emit cameraChanged();
}

void RenderService::resetCamera() {
    m_camera.reset();
    emit cameraChanged();
}

void RenderService::createDefaultGeometry() {
    LOG_INFO("RenderService: Creating default box geometry");

    try {
        // Get the document manager and create a default box
        auto manager_factory =
            g_ComponentFactory.getInstanceObject<Geometry::IGeoDocumentManagerSingletonFactory>();
        if(!manager_factory) {
            LOG_ERROR("Failed to get document manager factory");
            return;
        }

        auto document = manager_factory->currentDocument();
        if(!document) {
            LOG_ERROR("Failed to get current document");
            return;
        }

        // Build a simple box using OCC
        // This will be handled by the create action through the normal service path
        // For now, we just trigger a refresh to pick up any existing geometry
        refreshScene();

        if(!m_hasGeometry) {
            // If still no geometry, the create action should be triggered from QML
            LOG_DEBUG("No geometry found, default should be created via action");
        }
    } catch(const std::exception& e) {
        LOG_ERROR("Exception creating default geometry: {}", e.what());
    }
}

void RenderService::onDocumentGeometryChanged(const Geometry::GeometryChangeEvent& event) {
    LOG_INFO("RenderService: Document geometry changed, type={}", static_cast<int>(event.m_type));
    updateRenderData();
    LOG_INFO("RenderService: After updateRenderData, hasGeometry={}", m_hasGeometry);
    emit geometryChanged();
    emit sceneNeedsUpdate();
}

void RenderService::subscribeToCurrentDocument() {
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
                // Use Qt's thread-safe signal mechanism
                QMetaObject::invokeMethod(
                    this, [this, event]() { onDocumentGeometryChanged(event); },
                    Qt::QueuedConnection);
            });

        LOG_DEBUG("RenderService: Subscribed to document changes");
    } catch(const std::exception& e) {
        LOG_ERROR("Exception subscribing to document: {}", e.what());
    }
}

void RenderService::updateRenderData() {
    try {
        auto manager_factory =
            g_ComponentFactory.getInstanceObject<Geometry::IGeoDocumentManagerSingletonFactory>();
        if(!manager_factory) {
            m_renderData.clear();
            m_hasGeometry = false;
            return;
        }

        auto document = manager_factory->currentDocument();
        if(!document) {
            m_renderData.clear();
            m_hasGeometry = false;
            return;
        }

        // Get render data with default tessellation options
        m_renderData = document->getRenderData(TessellationOptions::defaultOptions());
        m_renderData.markModified(); // Increment version to trigger viewport update
        m_hasGeometry = !m_renderData.isEmpty();

        LOG_DEBUG("RenderService: Updated render data, meshCount={}, hasGeometry={}, version={}",
                  m_renderData.meshCount(), m_hasGeometry, m_renderData.m_version);
    } catch(const std::exception& e) {
        LOG_ERROR("Exception updating render data: {}", e.what());
        m_renderData.clear();
        m_hasGeometry = false;
    }
}

} // namespace OpenGeoLab::Render
