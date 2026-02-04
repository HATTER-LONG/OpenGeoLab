/**
 * @file opengl_viewport.cpp
 * @brief Implementation of GLViewport and GLViewportRenderer
 */

#include "app/opengl_viewport.hpp"
#include "util/logger.hpp"

#include <QMouseEvent>
#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QWheelEvent>
#include <QtCore/QMetaObject>
#include <QtMath>

#include <algorithm>

namespace OpenGeoLab::App {

// =============================================================================
// GLViewport Implementation
// =============================================================================

GLViewport::GLViewport(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemHasContents, true);
    setMirrorVertically(true);

    auto& scene_controller = Render::RenderSceneController::instance();
    m_cameraState = scene_controller.camera();
    m_hasGeometry = scene_controller.hasGeometry();

    // Bridge service events (Util::Signal) onto the Qt/GUI thread
    m_sceneNeedsUpdateConn = scene_controller.subscribeSceneNeedsUpdate([this]() {
        QMetaObject::invokeMethod(this, &GLViewport::onSceneNeedsUpdate, Qt::QueuedConnection);
    });
    m_cameraChangedConn = scene_controller.subscribeCameraChanged([this]() {
        QMetaObject::invokeMethod(this, &GLViewport::onSceneNeedsUpdate, Qt::QueuedConnection);
    });
    m_geometryChangedConn = scene_controller.subscribeGeometryChanged([this]() {
        QMetaObject::invokeMethod(this, &GLViewport::onServiceGeometryChanged,
                                  Qt::QueuedConnection);
    });

    LOG_TRACE("GLViewport created");
}

GLViewport::~GLViewport() { LOG_TRACE("GLViewport destroyed"); }

QQuickFramebufferObject::Renderer* GLViewport::createRenderer() const {
    return new GLViewportRenderer(this);
}

bool GLViewport::hasGeometry() const { return m_hasGeometry; }

const Render::CameraState& GLViewport::cameraState() const { return m_cameraState; }

const Render::DocumentRenderData& GLViewport::renderData() const {
    static Render::DocumentRenderData empty;
    return Render::RenderSceneController::instance().renderData();
}
// =============================================================================
// Entity Picking Implementation
// =============================================================================

bool GLViewport::pickModeEnabled() const { return m_pickModeEnabled; }
void GLViewport::setPickModeEnabled(bool enabled) {
    if(m_pickModeEnabled != enabled) {
        m_pickModeEnabled = enabled;
        emit pickModeEnabledChanged();
        LOG_DEBUG("GLViewport: Pick mode {}", enabled ? "enabled" : "disabled");
    }
}

Geometry::EntityType GLViewport::pickEntityType() const { return m_pickEntityType; }

void GLViewport::setPickEntityType(const Geometry::EntityType& type) {
    if(m_pickEntityType != type) {
        m_pickEntityType = type;
        emit pickEntityTypeChanged();
        LOG_DEBUG("GLViewport: Pick entity type set to '{}'", static_cast<int>(type));
    }
}

void GLViewport::requestPick(int x, int y, int radius) {
    if(!m_pickModeEnabled) {
        return;
    }
    performPick(x, y, radius);
}
bool GLViewport::consumePickRequest(int& x,
                                    int& y,
                                    int& radius,
                                    Geometry::EntityType& filter_type) {
    if(!m_pendingPickRequest.m_pending) {
        return false;
    }
    x = m_pendingPickRequest.m_x;
    y = m_pendingPickRequest.m_y;
    radius = m_pendingPickRequest.m_radius;
    filter_type = m_pendingPickRequest.m_filterType;
    m_pendingPickRequest.m_pending = false;
    return true;
}
void GLViewport::handlePickResult(const Render::PickPixelResult& result) {
    m_lastPickResult = result;

    if(result.m_valid) {
        QString type_str = entityTypeToString(result.m_entityType).data();
        LOG_DEBUG("GLViewport: Entity picked - type={}, uid={}", type_str.toStdString(),
                  result.m_entityUid);

        // Use QMetaObject::invokeMethod to safely emit signal from render thread
        QMetaObject::invokeMethod(
            this,
            [this, type_str, uid = result.m_entityUid]() {
                emit entityPicked(type_str, static_cast<int>(uid));
            },
            Qt::QueuedConnection);
    }
}

void GLViewport::handleUnpickResult(const Render::PickPixelResult& result) {
    if(result.m_valid) {
        QString type_str = entityTypeToString(result.m_entityType).data();
        LOG_DEBUG("GLViewport: Entity unpick requested - type={}, uid={}", type_str.toStdString(),
                  result.m_entityUid);

        // Use QMetaObject::invokeMethod to safely emit signal from render thread
        QMetaObject::invokeMethod(
            this,
            [this, type_str, uid = result.m_entityUid]() {
                emit entityUnpicked(type_str, static_cast<int>(uid));
            },
            Qt::QueuedConnection);
    }
}

void GLViewport::performPick(int x, int y, int radius) {
    const qreal dpr = window() ? window()->devicePixelRatio() : 1.0;
    const int w = static_cast<int>(qRound(width() * dpr));
    const int h = static_cast<int>(qRound(height() * dpr));
    int fb_x = static_cast<int>(qRound(x * dpr));
    int fb_y = static_cast<int>(qRound(y * dpr));
    int fb_radius = static_cast<int>(qRound(radius * dpr));
    fb_radius = std::max(1, fb_radius);
    if(w > 0) {
        fb_x = std::clamp(fb_x, 0, w - 1);
    }
    if(h > 0) {
        fb_y = std::clamp(fb_y, 0, h - 1);
    }

    // Store the pick request - it will be processed by the renderer
    // during the next synchronize/render cycle
    m_pendingPickRequest.m_pending = true;
    m_pendingPickRequest.m_x = fb_x;
    m_pendingPickRequest.m_y = fb_y;
    m_pendingPickRequest.m_radius = fb_radius;
    m_pendingPickRequest.m_filterType = m_pickEntityType;
    LOG_DEBUG("GLViewport: Pick requested at ({}, {}) (fb {}, {}) with radius {} (fb {})", x, y,
              fb_x, fb_y, radius, fb_radius);

    // Trigger an update to process the pick request
    update();
}

void GLViewport::requestUnpick(int x, int y, int radius) {
    if(!m_pickModeEnabled) {
        return;
    }
    performUnpick(x, y, radius);
}

void GLViewport::performUnpick(int x, int y, int radius) {
    const qreal dpr = window() ? window()->devicePixelRatio() : 1.0;
    const int w = static_cast<int>(qRound(width() * dpr));
    const int h = static_cast<int>(qRound(height() * dpr));
    int fb_x = static_cast<int>(qRound(x * dpr));
    int fb_y = static_cast<int>(qRound(y * dpr));
    int fb_radius = static_cast<int>(qRound(radius * dpr));
    fb_radius = std::max(1, fb_radius);
    if(w > 0) {
        fb_x = std::clamp(fb_x, 0, w - 1);
    }
    if(h > 0) {
        fb_y = std::clamp(fb_y, 0, h - 1);
    }

    // Store the unpick request - it will be processed by the renderer
    // during the next synchronize/render cycle
    m_pendingUnpickRequest.m_pending = true;
    m_pendingUnpickRequest.m_x = fb_x;
    m_pendingUnpickRequest.m_y = fb_y;
    m_pendingUnpickRequest.m_radius = fb_radius;
    m_pendingUnpickRequest.m_filterType = m_pickEntityType;
    LOG_DEBUG("GLViewport: Unpick requested at ({}, {}) (fb {}, {}) with radius {} (fb {})", x, y,
              fb_x, fb_y, radius, fb_radius);

    // Trigger an update to process the unpick request
    update();
}

bool GLViewport::consumeUnpickRequest(int& x,
                                      int& y,
                                      int& radius,
                                      Geometry::EntityType& filter_type) {
    if(!m_pendingUnpickRequest.m_pending) {
        return false;
    }
    x = m_pendingUnpickRequest.m_x;
    y = m_pendingUnpickRequest.m_y;
    radius = m_pendingUnpickRequest.m_radius;
    filter_type = m_pendingUnpickRequest.m_filterType;
    m_pendingUnpickRequest.m_pending = false;
    return true;
}

const Render::PickPixelResult& GLViewport::lastPickResult() const { return m_lastPickResult; }

// =============================================================================
// Hover Highlight Implementation
// =============================================================================

bool GLViewport::hoverHighlightEnabled() const { return m_hoverHighlightEnabled; }

void GLViewport::setHoverHighlightEnabled(bool enabled) {
    if(m_hoverHighlightEnabled != enabled) {
        m_hoverHighlightEnabled = enabled;
        emit hoverHighlightEnabledChanged();
        LOG_DEBUG("GLViewport: Hover highlight {}", enabled ? "enabled" : "disabled");
    }
}

void GLViewport::setSelectedEntityUids(const std::vector<Geometry::EntityUID>& selected_uids) {
    m_selectedEntityUids = selected_uids;
}

const std::vector<Geometry::EntityUID>& GLViewport::selectedEntityUids() const {
    return m_selectedEntityUids;
}

Geometry::EntityUID GLViewport::hoveredEntityUid() const { return m_hoveredEntityUid; }

void GLViewport::handleHoverResult(const Render::PickPixelResult& result) {
    Geometry::EntityUID new_hovered_uid = Geometry::INVALID_ENTITY_UID;

    if(result.m_valid) {
        // Check if this entity is in the selected list - if so, don't highlight it
        bool is_selected = std::find(m_selectedEntityUids.begin(), m_selectedEntityUids.end(),
                                     result.m_entityUid) != m_selectedEntityUids.end();
        if(!is_selected) {
            new_hovered_uid = result.m_entityUid;
        }
    }

    if(new_hovered_uid != m_hoveredEntityUid) {
        m_hoveredEntityUid = new_hovered_uid;
        // Trigger re-render to update highlight
        update();
    }
}

bool GLViewport::consumeHoverRequest(int& x, int& y) {
    if(!m_pendingHoverRequest.m_pending) {
        return false;
    }
    x = m_pendingHoverRequest.m_x;
    y = m_pendingHoverRequest.m_y;
    m_pendingHoverRequest.m_pending = false;
    return true;
}

void GLViewport::onSceneNeedsUpdate() {
    m_cameraState = Render::RenderSceneController::instance().camera();
    update();
}

void GLViewport::onServiceGeometryChanged() {
    const bool new_has_geometry = Render::RenderSceneController::instance().hasGeometry();
    if(new_has_geometry != m_hasGeometry) {
        m_hasGeometry = new_has_geometry;
        emit hasGeometryChanged();
    }

    emit geometryChanged();
    update();
}

void GLViewport::keyPressEvent(QKeyEvent* event) {
    m_pressedModifiers = event->modifiers();
    event->accept();
}
void GLViewport::mousePressEvent(QMouseEvent* event) {
    if(!hasFocus()) {
        forceActiveFocus();
    }
    m_lastMousePos = event->position();
    m_pressedButtons = event->buttons();
    m_pressedModifiers = event->modifiers();
    event->accept();
}

void GLViewport::mouseMoveEvent(QMouseEvent* event) {
    m_pressedButtons = event->buttons();
    m_pressedModifiers = event->modifiers();

    const QPointF delta = event->position() - m_lastMousePos;
    m_lastMousePos = event->position();

    if((m_pressedButtons & Qt::LeftButton) && (m_pressedModifiers & Qt::ControlModifier)) {
        // Ctrl + Left button: orbit
        orbitCamera(static_cast<float>(delta.x()), static_cast<float>(delta.y()));

    } else if((m_pressedButtons & Qt::LeftButton) && (m_pressedModifiers & Qt::ShiftModifier) ||
              m_pressedButtons & Qt::MiddleButton) {
        // Shift + Left button: pan
        // Middle button: pan
        panCamera(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
    } else if(m_pressedButtons & Qt::RightButton) {
        // Right button: zoom
        zoomCamera(static_cast<float>(-delta.y()));
    } else if(m_pickModeEnabled && m_hoverHighlightEnabled && m_pressedButtons == Qt::NoButton) {
        // No buttons pressed and in pick mode: request hover detection
        const qreal dpr = window() ? window()->devicePixelRatio() : 1.0;
        const int w = static_cast<int>(qRound(width() * dpr));
        const int h = static_cast<int>(qRound(height() * dpr));
        int x = static_cast<int>(qRound(event->position().x() * dpr));
        int y = static_cast<int>(qRound(event->position().y() * dpr));
        if(w > 0) {
            x = std::clamp(x, 0, w - 1);
        }
        if(h > 0) {
            y = std::clamp(y, 0, h - 1);
        }
        m_pendingHoverRequest.m_pending = true;
        m_pendingHoverRequest.m_x = x;
        m_pendingHoverRequest.m_y = y;
        update();
    }

    event->accept();
}

void GLViewport::mouseReleaseEvent(QMouseEvent* event) {
    m_pressedButtons = event->buttons();
    m_pressedModifiers = event->modifiers();
    // Handle picking in pick mode
    if(m_pickModeEnabled) {
        if(event->button() == Qt::LeftButton && !(m_pressedModifiers & Qt::ControlModifier) &&
           !(m_pressedModifiers & Qt::ShiftModifier)) {
            // Left click: perform pick
            // Use larger radius (8 pixels) for better picking success rate
            int x = static_cast<int>(event->position().x());
            int y = static_cast<int>(event->position().y());
            requestPick(x, y, 8);
        }
        if(event->button() == Qt::RightButton) {
            // Right click: check if there's a picked entity under cursor to unpick
            // Use same radius as pick for consistency
            int x = static_cast<int>(event->position().x());
            int y = static_cast<int>(event->position().y());
            requestUnpick(x, y, 8);
        }
    }
    event->accept();
}
void GLViewport::keyReleaseEvent(QKeyEvent* event) {
    m_pressedModifiers = event->modifiers();
    event->accept();
}

void GLViewport::wheelEvent(QWheelEvent* event) {
    if((event->modifiers() & Qt::ControlModifier)) {
        const float delta = event->angleDelta().y() / 120.0f;
        zoomCamera(delta * 5.0f);
    }
    event->accept();
}

void GLViewport::orbitCamera(float dx, float dy) {
    const float sensitivity = 0.5f;
    const float yaw = -dx * sensitivity;
    const float pitch = -dy * sensitivity;

    // Calculate the direction vector from camera to target
    QVector3D direction = m_cameraState.m_position - m_cameraState.m_target;
    const float distance = direction.length();

    // Convert to spherical coordinates
    float theta = qAtan2(direction.x(), direction.z());
    float phi = qAsin(qBound(-1.0f, direction.y() / distance, 1.0f));

    // Apply rotation
    theta += qDegreesToRadians(yaw);
    phi += qDegreesToRadians(pitch);

    // Clamp phi to avoid gimbal lock
    phi = qBound(-1.5f, phi, 1.5f);

    // Convert back to Cartesian coordinates
    direction.setX(distance * qCos(phi) * qSin(theta));
    direction.setY(distance * qSin(phi));
    direction.setZ(distance * qCos(phi) * qCos(theta));

    m_cameraState.m_position = m_cameraState.m_target + direction;

    Render::RenderSceneController::instance().setCamera(m_cameraState, false);
    update();
}

void GLViewport::panCamera(float dx, float dy) {
    const float sensitivity = 0.005f;

    // Calculate right and up vectors
    const QVector3D forward = (m_cameraState.m_target - m_cameraState.m_position).normalized();
    const QVector3D right = QVector3D::crossProduct(forward, m_cameraState.m_up).normalized();
    const QVector3D up = QVector3D::crossProduct(right, forward).normalized();

    // Calculate pan distance based on camera distance
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    const float pan_scale = distance * sensitivity;

    const QVector3D pan = right * (-dx * pan_scale) + up * (dy * pan_scale);
    m_cameraState.m_position += pan;
    m_cameraState.m_target += pan;

    Render::RenderSceneController::instance().setCamera(m_cameraState, false);

    update();
}

void GLViewport::zoomCamera(float delta) {
    const float sensitivity = 0.1f;

    QVector3D direction = m_cameraState.m_position - m_cameraState.m_target;
    float distance = direction.length();

    // Apply zoom
    distance *= (1.0f - delta * sensitivity);
    distance = qMax(0.1f, distance);

    direction = direction.normalized() * distance;
    m_cameraState.m_position = m_cameraState.m_target + direction;

    m_cameraState.updateClipping(distance);

    Render::RenderSceneController::instance().setCamera(m_cameraState);

    update();
}

// =============================================================================
// GLViewportRenderer Implementation
// =============================================================================

GLViewportRenderer::GLViewportRenderer(const GLViewport* viewport)
    : m_viewport(viewport), m_sceneRenderer(std::make_unique<Render::SceneRenderer>()) {
    LOG_TRACE("GLViewportRenderer created");
}

GLViewportRenderer::~GLViewportRenderer() {
    m_sceneRenderer->cleanup();
    LOG_TRACE("GLViewportRenderer destroyed");
}

QOpenGLFramebufferObject* GLViewportRenderer::createFramebufferObject(const QSize& size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4); // Enable MSAA
    m_viewportSize = size;
    m_sceneRenderer->setViewportSize(size);
    return new QOpenGLFramebufferObject(size, format);
}

void GLViewportRenderer::synchronize(QQuickFramebufferObject* item) {
    auto* viewport = qobject_cast<GLViewport*>(item);
    if(!viewport) {
        return;
    }

    m_cameraState = viewport->cameraState();

    // Check if render data changed using version number
    const auto& new_render_data = viewport->renderData();
    if(new_render_data.m_version != m_renderData.m_version) {
        LOG_DEBUG("GLViewportRenderer: Render data changed, version {} -> {}, uploading {} meshes",
                  m_renderData.m_version, new_render_data.m_version, new_render_data.meshCount());
        m_renderData = new_render_data;
        m_needsDataUpload = true;
    }

    // Check for pending pick request
    int pick_x, pick_y, pick_radius;
    Geometry::EntityType filter_type;
    if(viewport->consumePickRequest(pick_x, pick_y, pick_radius, filter_type)) {
        m_pendingPick = true;
        m_pickX = pick_x;
        m_pickY = pick_y;
        m_pickRadius = pick_radius;
        m_pickFilterType = filter_type;
    }

    // Check for pending unpick request (right-click)
    int unpick_x, unpick_y, unpick_radius;
    Geometry::EntityType unpick_filter_type;
    if(viewport->consumeUnpickRequest(unpick_x, unpick_y, unpick_radius, unpick_filter_type)) {
        m_pendingUnpick = true;
        m_unpickX = unpick_x;
        m_unpickY = unpick_y;
        m_unpickRadius = unpick_radius;
        m_unpickFilterType = unpick_filter_type;
    }

    // Check for pending hover request
    int hover_x, hover_y;
    if(viewport->consumeHoverRequest(hover_x, hover_y)) {
        m_pendingHover = true;
        m_hoverX = hover_x;
        m_hoverY = hover_y;
    }

    // Sync hover highlight state
    m_hoverHighlightEnabled = viewport->hoverHighlightEnabled();
    m_selectedEntityUids = viewport->selectedEntityUids();
    m_hoveredEntityUid = viewport->hoveredEntityUid();
}

void GLViewportRenderer::render() {
    if(!m_sceneRenderer->isInitialized()) {
        m_sceneRenderer->initialize();
    }

    if(m_needsDataUpload) {
        m_sceneRenderer->uploadMeshData(m_renderData);
        m_needsDataUpload = false;
    }

    // Calculate matrices
    const float aspect_ratio = m_viewportSize.width() / static_cast<float>(m_viewportSize.height());
    const QMatrix4x4 projection = m_cameraState.projectionMatrix(aspect_ratio);
    const QMatrix4x4 view = m_cameraState.viewMatrix();
    // Handle pending pick request
    if(m_pendingPick) {
        m_pendingPick = false;
        performPickInRenderThread(view, projection);
    }
    // Handle pending unpick request (right-click)
    if(m_pendingUnpick) {
        m_pendingUnpick = false;
        performUnpickInRenderThread(view, projection);
    }
    // Handle pending hover request
    if(m_pendingHover) {
        m_pendingHover = false;
        performHoverInRenderThread(view, projection);
    }

    // Set highlighted entity UID for rendering (if not in selected list)
    m_sceneRenderer->setHighlightedEntityUid(m_hoveredEntityUid);

    // Delegate rendering to SceneRenderer
    m_sceneRenderer->render(m_cameraState.m_position, view, projection);
}

void GLViewportRenderer::performPickInRenderThread(const QMatrix4x4& view,
                                                   const QMatrix4x4& projection) {
    // Perform picking with snap-to-entity in a radius
    Render::PickPixelResult best_result;
    int best_distance = m_pickRadius * m_pickRadius + 1;

    // Search in a square area around the click point
    for(int dy = -m_pickRadius; dy <= m_pickRadius; ++dy) {
        for(int dx = -m_pickRadius; dx <= m_pickRadius; ++dx) {
            int dist_sq = dx * dx + dy * dy;
            if(dist_sq > m_pickRadius * m_pickRadius) {
                continue;
            }

            auto pixel_result =
                m_sceneRenderer->pickAtPixel(m_pickX + dx, m_pickY + dy, view, projection);
            if(pixel_result.m_valid) {
                // Check if this matches our filter
                if(m_pickFilterType != Geometry::EntityType::None &&
                   pixel_result.m_entityType != m_pickFilterType) {
                    continue;
                }

                if(dist_sq < best_distance) {
                    best_distance = dist_sq;
                    best_result.m_valid = true;
                    best_result.m_entityType = pixel_result.m_entityType;
                    best_result.m_entityUid = pixel_result.m_entityUid;
                }
            }
        }
    }

    // Report result back to viewport (will be delivered via queued connection)
    if(m_viewport) {
        const_cast<GLViewport*>(m_viewport)->handlePickResult(best_result);
    }
}

void GLViewportRenderer::performUnpickInRenderThread(const QMatrix4x4& view,
                                                     const QMatrix4x4& projection) {
    // Perform unpicking with snap-to-entity in a radius (same logic as pick)
    Render::PickPixelResult best_result;
    int best_distance = m_unpickRadius * m_unpickRadius + 1;

    // Search in a square area around the click point
    for(int dy = -m_unpickRadius; dy <= m_unpickRadius; ++dy) {
        for(int dx = -m_unpickRadius; dx <= m_unpickRadius; ++dx) {
            int dist_sq = dx * dx + dy * dy;
            if(dist_sq > m_unpickRadius * m_unpickRadius) {
                continue;
            }

            auto pixel_result =
                m_sceneRenderer->pickAtPixel(m_unpickX + dx, m_unpickY + dy, view, projection);
            if(pixel_result.m_valid) {
                // Check if this matches our filter
                if(m_unpickFilterType != Geometry::EntityType::None &&
                   pixel_result.m_entityType != m_unpickFilterType) {
                    continue;
                }

                if(dist_sq < best_distance) {
                    best_distance = dist_sq;
                    best_result.m_valid = true;
                    best_result.m_entityType = pixel_result.m_entityType;
                    best_result.m_entityUid = pixel_result.m_entityUid;
                }
            }
        }
    }

    // Report result back to viewport for unpick handling
    if(m_viewport) {
        const_cast<GLViewport*>(m_viewport)->handleUnpickResult(best_result);
    }
}

void GLViewportRenderer::performHoverInRenderThread(const QMatrix4x4& view,
                                                    const QMatrix4x4& projection) {
    // Perform hover detection at single pixel (no radius needed for hover)
    auto pixel_result = m_sceneRenderer->pickAtPixel(m_hoverX, m_hoverY, view, projection);

    // Report result back to viewport for hover handling
    if(m_viewport) {
        const_cast<GLViewport*>(m_viewport)->handleHoverResult(pixel_result);
    }
}

Render::PickPixelResult
GLViewportRenderer::pickAt(int x, int y, int /*radius*/, Geometry::EntityType /*filterType*/) {
    Render::PickPixelResult result;

    if(!m_sceneRenderer->isInitialized()) {
        return result;
    }

    // Calculate matrices
    const float aspect_ratio = m_viewportSize.width() / static_cast<float>(m_viewportSize.height());
    const QMatrix4x4 projection = m_cameraState.projectionMatrix(aspect_ratio);
    const QMatrix4x4 view = m_cameraState.viewMatrix();

    // Perform single-pixel pick
    auto pixel_result = m_sceneRenderer->pickAtPixel(x, y, view, projection);

    if(pixel_result.m_valid) {
        result.m_valid = true;
        result.m_entityType = pixel_result.m_entityType;
        result.m_entityUid = pixel_result.m_entityUid;
    }

    return result;
}
} // namespace OpenGeoLab::App
