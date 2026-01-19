/**
 * @file viewport_item.cpp
 * @brief Implementation of QML viewport item
 */

#include "app/viewport_item.hpp"
#include "util/logger.hpp"

#include <QMouseEvent>
#include <QOpenGLFramebufferObjectFormat>
#include <QQuickWindow>
#include <QWheelEvent>

#include <cmath>

namespace OpenGeoLab::App {

// ============================================================================
// ViewportRenderer Implementation
// ============================================================================

ViewportRenderer::ViewportRenderer(ViewportItem* item) : m_item(item) {
    m_renderer = std::make_unique<Render::GLRenderer>();
    m_tessellator = std::make_unique<Render::Tessellator>();
}

ViewportRenderer::~ViewportRenderer() {
    if(m_renderer) {
        m_renderer->cleanup();
    }
}

QOpenGLFramebufferObject* ViewportRenderer::createFramebufferObject(const QSize& size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4); // MSAA

    m_width = size.width();
    m_height = size.height();

    // Initialize renderer if needed
    if(!m_renderer->isInitialized()) {
        m_renderer->initialize();
    }

    return new QOpenGLFramebufferObject(size, format);
}

void ViewportRenderer::render() {
    if(m_needsRebuild) {
        rebuildScene();
        m_needsRebuild = false;
    }

    m_renderer->render(m_width, m_height);
}

void ViewportRenderer::synchronize(QQuickFramebufferObject* item) {
    auto* viewport = dynamic_cast<ViewportItem*>(item);
    if(!viewport) {
        return;
    }

    m_item = viewport;

    // Check if document has changed using version tracking
    auto document = Geometry::GeometryDocument::instance();
    uint64_t currentVersion = document->version();
    if(currentVersion != m_lastDocumentVersion) {
        LOG_INFO("Document version changed: {} -> {}, triggering rebuild", m_lastDocumentVersion,
                 currentVersion);
        m_needsRebuild = true;
        m_lastDocumentVersion = currentVersion;
    }

    // Sync selection state
    m_renderer->setSelectedEntities(document->selectedIds());

    // Sync camera
    m_renderer->setCamera(viewport->camera());

    // Sync display settings
    Render::DisplaySettings settings = viewport->displaySettings();
    QColor bg = viewport->backgroundColor();
    settings.backgroundColor =
        Geometry::Color(static_cast<float>(bg.redF()), static_cast<float>(bg.greenF()),
                        static_cast<float>(bg.blueF()), static_cast<float>(bg.alphaF()));
    settings.showFaces = viewport->showFaces();
    settings.showEdges = viewport->showEdges();
    m_renderer->setDisplaySettings(settings);

    // Sync size
    m_width = static_cast<int>(viewport->width());
    m_height = static_cast<int>(viewport->height());
}

void ViewportRenderer::rebuildScene() {
    m_scene.clear();
    m_renderer->clearMeshes();

    auto document = Geometry::GeometryDocument::instance();
    LOG_INFO("Rebuilding scene with {} parts", document->parts().size());

    for(const auto& part : document->parts()) {
        if(!part->isVisible()) {
            LOG_DEBUG("Skipping invisible part: {}", part->name());
            continue;
        }

        LOG_INFO("Tessellating part: {} (ID: {})", part->name(), part->id());
        auto meshes = m_tessellator->tessellatePart(part);
        LOG_INFO("Generated {} meshes from part {}", meshes.size(), part->name());

        for(auto& mesh : meshes) {
            LOG_DEBUG("Mesh has {} vertices, {} face indices, {} edge vertices",
                      mesh->vertices.size(), mesh->faceIndices.size(), mesh->edgeVertices.size());
            m_scene.addMesh(mesh);
            m_renderer->updateMesh(mesh);
        }
    }

    LOG_INFO("Scene rebuilt: {} meshes, {} triangles", m_scene.meshes.size(),
             m_scene.totalTriangles());
}

// ============================================================================
// ViewportItem Implementation
// ============================================================================

ViewportItem::ViewportItem(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setFlag(ItemAcceptsInputMethod, true);

    // Initialize camera
    m_camera.reset();
    updateCameraFromOrbit();

    // Initialize display settings
    m_displaySettings.showFaces = m_showFaces;
    m_displaySettings.showEdges = m_showEdges;
}

ViewportItem::~ViewportItem() = default;

QQuickFramebufferObject::Renderer* ViewportItem::createRenderer() const {
    return new ViewportRenderer(const_cast<ViewportItem*>(this));
}

void ViewportItem::setSelectionMode(SelectionMode mode) {
    if(m_selectionMode == mode) {
        return;
    }
    m_selectionMode = mode;
    emit selectionModeChanged();
}

void ViewportItem::setShowFaces(bool show) {
    if(m_showFaces == show) {
        return;
    }
    m_showFaces = show;
    m_displaySettings.showFaces = show;
    emit showFacesChanged();
    update();
}

void ViewportItem::setShowEdges(bool show) {
    if(m_showEdges == show) {
        return;
    }
    m_showEdges = show;
    m_displaySettings.showEdges = show;
    emit showEdgesChanged();
    update();
}

void ViewportItem::setBackgroundColor(const QColor& color) {
    if(m_backgroundColor == color) {
        return;
    }
    m_backgroundColor = color;
    emit backgroundColorChanged();
    update();
}

void ViewportItem::fitAll() {
    auto document = Geometry::GeometryDocument::instance();
    Geometry::BoundingBox bbox = document->totalBoundingBox();

    if(!bbox.isValid()) {
        resetView();
        return;
    }

    Geometry::Point3D center = bbox.center();
    m_orbitCenter = {static_cast<float>(center.x), static_cast<float>(center.y),
                     static_cast<float>(center.z)};

    double diagonal = bbox.diagonalLength();
    m_orbitDistance = static_cast<float>(diagonal * 2.0);

    updateCameraFromOrbit();
    update();
}

void ViewportItem::resetView() {
    m_orbitCenter = {0.0f, 0.0f, 0.0f};
    m_orbitDistance = 100.0f;
    m_orbitAzimuth = 45.0f;
    m_orbitElevation = 30.0f;

    updateCameraFromOrbit();
    update();
}

void ViewportItem::zoomToEntity(quint64 entityId) {
    auto document = Geometry::GeometryDocument::instance();
    auto entity = document->findEntity(static_cast<Geometry::EntityId>(entityId));

    if(!entity) {
        return;
    }

    Geometry::BoundingBox bbox = entity->boundingBox();
    if(!bbox.isValid()) {
        return;
    }

    Geometry::Point3D center = bbox.center();
    m_orbitCenter = {static_cast<float>(center.x), static_cast<float>(center.y),
                     static_cast<float>(center.z)};

    double diagonal = bbox.diagonalLength();
    m_orbitDistance = static_cast<float>(diagonal * 2.0);

    updateCameraFromOrbit();
    update();
}

void ViewportItem::mousePressEvent(QMouseEvent* event) {
    m_lastMousePos = event->position();

    if(event->button() == Qt::MiddleButton ||
       (event->button() == Qt::LeftButton && event->modifiers() & Qt::AltModifier)) {
        m_rotating = true;
        event->accept();
    } else if(event->button() == Qt::RightButton ||
              (event->button() == Qt::MiddleButton && event->modifiers() & Qt::ShiftModifier)) {
        m_panning = true;
        event->accept();
    } else if(event->button() == Qt::LeftButton) {
        // TODO: Perform picking
        event->accept();
    }
}

void ViewportItem::mouseReleaseEvent(QMouseEvent* event) {
    m_rotating = false;
    m_panning = false;
    event->accept();
}

void ViewportItem::mouseMoveEvent(QMouseEvent* event) {
    QPointF delta = event->position() - m_lastMousePos;
    m_lastMousePos = event->position();

    if(m_rotating) {
        handleRotation(delta);
        event->accept();
    } else if(m_panning) {
        handlePan(delta);
        event->accept();
    }
}

void ViewportItem::wheelEvent(QWheelEvent* event) {
    float delta = static_cast<float>(event->angleDelta().y()) / 120.0f;
    handleZoom(delta);
    event->accept();
}

void ViewportItem::hoverMoveEvent(QHoverEvent* event) {
    // TODO: Perform hover picking and emit entityHovered
    (void)event;
}

void ViewportItem::handleRotation(const QPointF& delta) {
    const float sensitivity = 0.5f;

    m_orbitAzimuth -= static_cast<float>(delta.x()) * sensitivity;
    m_orbitElevation += static_cast<float>(delta.y()) * sensitivity;

    // Clamp elevation to avoid gimbal lock
    m_orbitElevation = std::clamp(m_orbitElevation, -89.0f, 89.0f);

    // Wrap azimuth
    while(m_orbitAzimuth < 0.0f)
        m_orbitAzimuth += 360.0f;
    while(m_orbitAzimuth >= 360.0f)
        m_orbitAzimuth -= 360.0f;

    updateCameraFromOrbit();
    update();
}

void ViewportItem::handlePan(const QPointF& delta) {
    const float sensitivity = m_orbitDistance * 0.001f;

    // Calculate camera right and up vectors
    float azimuthRad = m_orbitAzimuth * 3.14159265f / 180.0f;
    float elevationRad = m_orbitElevation * 3.14159265f / 180.0f;

    float rightX = -std::sin(azimuthRad);
    float rightY = 0.0f;
    float rightZ = std::cos(azimuthRad);

    float upX = -std::cos(azimuthRad) * std::sin(elevationRad);
    float upY = std::cos(elevationRad);
    float upZ = -std::sin(azimuthRad) * std::sin(elevationRad);

    float dx = static_cast<float>(-delta.x()) * sensitivity;
    float dy = static_cast<float>(delta.y()) * sensitivity;

    m_orbitCenter[0] += rightX * dx + upX * dy;
    m_orbitCenter[1] += rightY * dx + upY * dy;
    m_orbitCenter[2] += rightZ * dx + upZ * dy;

    updateCameraFromOrbit();
    update();
}

void ViewportItem::handleZoom(float delta) {
    const float zoomFactor = 0.1f;

    m_orbitDistance *= (1.0f - delta * zoomFactor);
    m_orbitDistance = std::max(0.1f, m_orbitDistance);

    updateCameraFromOrbit();
    update();
}

void ViewportItem::updateCameraFromOrbit() {
    float azimuthRad = m_orbitAzimuth * 3.14159265f / 180.0f;
    float elevationRad = m_orbitElevation * 3.14159265f / 180.0f;

    // Calculate camera position on orbit sphere
    float cosElev = std::cos(elevationRad);
    float sinElev = std::sin(elevationRad);
    float cosAzim = std::cos(azimuthRad);
    float sinAzim = std::sin(azimuthRad);

    m_camera.position[0] = m_orbitCenter[0] + m_orbitDistance * cosElev * cosAzim;
    m_camera.position[1] = m_orbitCenter[1] + m_orbitDistance * sinElev;
    m_camera.position[2] = m_orbitCenter[2] + m_orbitDistance * cosElev * sinAzim;

    m_camera.target[0] = m_orbitCenter[0];
    m_camera.target[1] = m_orbitCenter[1];
    m_camera.target[2] = m_orbitCenter[2];

    m_camera.up[0] = 0.0f;
    m_camera.up[1] = 1.0f;
    m_camera.up[2] = 0.0f;
}

} // namespace OpenGeoLab::App
