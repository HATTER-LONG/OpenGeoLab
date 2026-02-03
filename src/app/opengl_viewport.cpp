/**
 * @file opengl_viewport.cpp
 * @brief Implementation of GLViewport and GLViewportRenderer
 */

#include "app/opengl_viewport.hpp"
#include "util/logger.hpp"

#include <QHoverEvent>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQuickWindow>
#include <QWheelEvent>
#include <QtCore/QMetaObject>
#include <QtMath>
#include <unordered_set>

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
    m_highlightChangedConn = scene_controller.subscribeHighlightChanged([this]() {
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

int GLViewport::selectionMode() const { return static_cast<int>(m_selectionMode); }

void GLViewport::setSelectionMode(int mode) {
    auto new_mode = static_cast<Geometry::SelectionMode>(mode);
    if(m_selectionMode != new_mode) {
        m_selectionMode = new_mode;
        emit selectionModeChanged();
        LOG_DEBUG("GLViewport: Selection mode changed to {}", mode);
    }
}

void GLViewport::setPickingEnabled(bool enabled) {
    if(m_pickingEnabled == enabled) {
        return;
    }

    m_pickingEnabled = enabled;
    setAcceptHoverEvents(enabled);
    emit pickingEnabledChanged();

    if(!enabled) {
        clearPickedSelection();
        clearPreviewHighlight();
        m_boxSelecting = false;
        m_boxSelectionRect = QRectF();
        emit boxSelectingChanged();
        emit boxSelectionRectChanged();
    }
}

void GLViewport::clearPickedSelection() {
    if(m_selectedEntityIds.empty()) {
        return;
    }

    auto& controller = Render::RenderSceneController::instance();
    controller.clearHighlight(m_selectedEntityIds);

    m_selectedEntityIds.clear();
    emit selectionChanged({});
    update();
}

void GLViewport::deselectEntity(quint64 entity_id) {
    if(entity_id == 0 || m_selectedEntityIds.empty()) {
        return;
    }

    const auto id = static_cast<Geometry::EntityId>(entity_id);
    std::vector<Geometry::EntityId> to_remove{id};
    removeFromSelectionInternal(to_remove);
}

QVariantList GLViewport::selectedEntities() const {
    QVariantList out;
    out.reserve(static_cast<int>(m_selectedEntityIds.size()));
    for(const auto id : m_selectedEntityIds) {
        out.append(QVariant::fromValue(static_cast<quint64>(id)));
    }
    return out;
}

quint64 GLViewport::pickEntityAt(int x, int y) {
    if(m_selectionMode == Geometry::SelectionMode::None) {
        return Geometry::INVALID_ENTITY_ID;
    }

    // Set up pick request (will be processed in next render pass)
    m_pendingPickRequest = PickRequest{x, y, false, 0, 0, PickReason::Legacy, false};
    m_pickResult.clear();

    // Force a render pass to perform picking
    update();

    // Wait for result (sync with render thread)
    // Note: In a real implementation, this should be async
    // For now, we trigger an update and the result will be available after render
    if(window()) {
        // Force immediate rendering
        QMetaObject::invokeMethod(window(), "update", Qt::DirectConnection);
    }

    if(!m_pickResult.empty()) {
        return m_pickResult[0];
    }
    return Geometry::INVALID_ENTITY_ID;
}

QVariantList GLViewport::pickEntitiesInRect(int x1, int y1, int x2, int y2) {
    if(m_selectionMode == Geometry::SelectionMode::None) {
        return {};
    }

    // Normalize rectangle coordinates
    int min_x = std::min(x1, x2);
    int min_y = std::min(y1, y2);
    int max_x = std::max(x1, x2);
    int max_y = std::max(y1, y2);

    m_pendingPickRequest = PickRequest{min_x, min_y, true, max_x, max_y, PickReason::Legacy, false};
    m_pickResult.clear();

    // Force a render pass
    update();

    if(window()) {
        QMetaObject::invokeMethod(window(), "update", Qt::DirectConnection);
    }

    QVariantList result;
    for(const auto& id : m_pickResult) {
        if(id != Geometry::INVALID_ENTITY_ID) {
            result.append(QVariant::fromValue(static_cast<quint64>(id)));
        }
    }
    return result;
}

const Render::CameraState& GLViewport::cameraState() const { return m_cameraState; }

const Render::DocumentRenderData& GLViewport::renderData() const {
    static Render::DocumentRenderData empty;
    return Render::RenderSceneController::instance().renderData();
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

void GLViewport::hoverMoveEvent(QHoverEvent* event) {
    if(!m_pickingEnabled || m_selectionMode == Geometry::SelectionMode::None || !m_hasGeometry) {
        QQuickFramebufferObject::hoverMoveEvent(event);
        return;
    }

    const QPointF pos = event->position();
    PickRequest request;
    request.m_x = static_cast<int>(pos.x());
    request.m_y = static_cast<int>(pos.y());
    request.m_isRect = false;
    request.reason = PickReason::Hover;
    request.additive = false;
    requestPick(request);

    event->accept();
}

void GLViewport::hoverLeaveEvent(QHoverEvent* event) {
    if(m_pickingEnabled) {
        clearPreviewHighlight();
        emit hoveredEntityChanged(0);
    }
    QQuickFramebufferObject::hoverLeaveEvent(event);
}

void GLViewport::mousePressEvent(QMouseEvent* event) {
    if(!hasFocus()) {
        forceActiveFocus();
    }
    m_lastMousePos = event->position();
    m_pressedButtons = event->buttons();
    m_pressedModifiers = event->modifiers();

    if(m_pickingEnabled &&
       (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)) {
        m_boxButton = event->button();
        m_boxStart = event->position();
        m_boxSelecting = false;
        m_boxSelectionRect = QRectF();
        emit boxSelectingChanged();
        emit boxSelectionRectChanged();
    }

    event->accept();
}

void GLViewport::mouseMoveEvent(QMouseEvent* event) {
    const QPointF delta = event->position() - m_lastMousePos;
    m_lastMousePos = event->position();

    // Picking mode: handle box selection gestures (mouse move with button pressed)
    if(m_pickingEnabled && m_hasGeometry && m_selectionMode != Geometry::SelectionMode::None) {
        const bool wants_camera_orbit =
            (m_pressedButtons & Qt::LeftButton) && (m_pressedModifiers & Qt::ControlModifier);
        const bool wants_camera_pan =
            (((m_pressedButtons & Qt::LeftButton) && (m_pressedModifiers & Qt::ShiftModifier)) ||
             (m_pressedButtons & Qt::MiddleButton));

        const bool is_drag_button =
            (m_pressedButtons & Qt::LeftButton) || (m_pressedButtons & Qt::RightButton);
        const bool is_camera_drag =
            wants_camera_orbit || wants_camera_pan || (m_pressedButtons & Qt::MiddleButton);

        if(is_drag_button && !is_camera_drag) {
            const QPointF current = event->position();
            const QPointF diff = current - m_boxStart;
            const float threshold = 3.0f;
            if(!m_boxSelecting &&
               (std::abs(diff.x()) > threshold || std::abs(diff.y()) > threshold)) {
                m_boxSelecting = true;
                emit boxSelectingChanged();
            }

            if(m_boxSelecting) {
                const qreal x1 = std::min(m_boxStart.x(), current.x());
                const qreal y1 = std::min(m_boxStart.y(), current.y());
                const qreal x2 = std::max(m_boxStart.x(), current.x());
                const qreal y2 = std::max(m_boxStart.y(), current.y());
                m_boxSelectionRect = QRectF(QPointF(x1, y1), QPointF(x2, y2));
                emit boxSelectionRectChanged();
            }

            event->accept();
            return;
        }
    }

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
    }

    event->accept();
}

void GLViewport::mouseReleaseEvent(QMouseEvent* event) {
    m_pressedButtons = event->buttons();

    if(m_pickingEnabled && m_hasGeometry && m_selectionMode != Geometry::SelectionMode::None) {
        const bool ctrl = (m_pressedModifiers & Qt::ControlModifier);

        // Right click without drag: clear all selections
        if(event->button() == Qt::RightButton && !m_boxSelecting) {
            clearPickedSelection();
            clearPreviewHighlight();
            emit hoveredEntityChanged(0);
            event->accept();
            return;
        }

        // Box selection or deselection
        if(m_boxSelecting &&
           (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)) {
            const QRectF rect = m_boxSelectionRect;
            PickRequest request;
            request.m_x = static_cast<int>(rect.left());
            request.m_y = static_cast<int>(rect.top());
            request.m_x2 = static_cast<int>(rect.right());
            request.m_y2 = static_cast<int>(rect.bottom());
            request.m_isRect = true;
            request.additive = ctrl;
            request.reason = (event->button() == Qt::LeftButton) ? PickReason::BoxSelect
                                                                 : PickReason::BoxDeselect;
            requestPick(request);

            m_boxSelecting = false;
            m_boxSelectionRect = QRectF();
            emit boxSelectingChanged();
            emit boxSelectionRectChanged();

            event->accept();
            return;
        }

        // Click selection
        if(event->button() == Qt::LeftButton && !m_boxSelecting) {
            const QPointF pos = event->position();
            PickRequest request;
            request.m_x = static_cast<int>(pos.x());
            request.m_y = static_cast<int>(pos.y());
            request.m_isRect = false;
            request.reason = PickReason::ClickSelect;
            request.additive = ctrl;
            requestPick(request);
            event->accept();
            return;
        }
    }

    event->accept();
}
void GLViewport::keyReleaseEvent(QKeyEvent* event) {
    m_pressedModifiers = event->modifiers();
    event->accept();
}

void GLViewport::wheelEvent(QWheelEvent* event) {
    if((m_pressedModifiers & Qt::ControlModifier)) {
        const float delta = event->angleDelta().y() / 120.0f;
        zoomCamera(delta * 5.0f);
    }
    event->accept();
}

void GLViewport::requestPick(const PickRequest& request) {
    m_pendingPickRequest = request;
    m_pickResult.clear();
    update();
}

void GLViewport::clearPreviewHighlight() {
    if(m_previewEntityId == Geometry::INVALID_ENTITY_ID) {
        return;
    }

    // Do not clear if it's selected
    const bool is_selected = std::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(),
                                       m_previewEntityId) != m_selectedEntityIds.end();
    if(!is_selected) {
        Render::RenderSceneController::instance().setHighlight(m_previewEntityId,
                                                               Render::HighlightState::None);
    }

    m_previewEntityId = Geometry::INVALID_ENTITY_ID;
    update();
}

void GLViewport::setPreviewHighlight(Geometry::EntityId entity_id) {
    if(entity_id == m_previewEntityId) {
        return;
    }

    clearPreviewHighlight();
    m_previewEntityId = entity_id;

    if(entity_id == Geometry::INVALID_ENTITY_ID) {
        return;
    }

    const bool is_selected = std::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(),
                                       entity_id) != m_selectedEntityIds.end();
    if(!is_selected) {
        Render::RenderSceneController::instance().setHighlight(entity_id,
                                                               Render::HighlightState::Preview);
        update();
    }
}

void GLViewport::setSelectionInternal(const std::vector<Geometry::EntityId>& ids, bool replace) {
    auto& controller = Render::RenderSceneController::instance();

    if(replace) {
        if(!m_selectedEntityIds.empty()) {
            controller.clearHighlight(m_selectedEntityIds);
        }
        m_selectedEntityIds.clear();
    }

    // Add unique ids preserving order
    for(const auto id : ids) {
        if(id == Geometry::INVALID_ENTITY_ID) {
            continue;
        }
        if(std::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(), id) !=
           m_selectedEntityIds.end()) {
            continue;
        }
        m_selectedEntityIds.push_back(id);
        controller.setHighlight(id, Render::HighlightState::Selected);
    }

    // If preview is now selected, clear preview state
    if(m_previewEntityId != Geometry::INVALID_ENTITY_ID) {
        const bool is_selected = std::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(),
                                           m_previewEntityId) != m_selectedEntityIds.end();
        if(is_selected) {
            m_previewEntityId = Geometry::INVALID_ENTITY_ID;
            emit hoveredEntityChanged(0);
        }
    }

    QVariantList out;
    out.reserve(static_cast<int>(m_selectedEntityIds.size()));
    for(const auto id : m_selectedEntityIds) {
        out.append(QVariant::fromValue(static_cast<quint64>(id)));
    }
    emit selectionChanged(out);
    update();
}

void GLViewport::removeFromSelectionInternal(const std::vector<Geometry::EntityId>& ids) {
    if(ids.empty() || m_selectedEntityIds.empty()) {
        return;
    }

    auto& controller = Render::RenderSceneController::instance();

    bool changed = false;
    for(const auto id : ids) {
        auto it = std::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(), id);
        if(it != m_selectedEntityIds.end()) {
            m_selectedEntityIds.erase(it);
            controller.setHighlight(id, Render::HighlightState::None);
            changed = true;
        }
    }

    if(changed) {
        QVariantList out;
        out.reserve(static_cast<int>(m_selectedEntityIds.size()));
        for(const auto id : m_selectedEntityIds) {
            out.append(QVariant::fromValue(static_cast<quint64>(id)));
        }
        emit selectionChanged(out);
        update();
    }
}

void GLViewport::handlePickResult(const PickRequest& request,
                                  const std::vector<Geometry::EntityId>& ids) {
    if(request.reason == PickReason::Legacy) {
        return;
    }

    if(request.reason == PickReason::Hover) {
        Geometry::EntityId hovered = Geometry::INVALID_ENTITY_ID;
        if(!ids.empty()) {
            hovered = ids.front();
        }
        setPreviewHighlight(hovered);
        emit hoveredEntityChanged(
            static_cast<quint64>(hovered == Geometry::INVALID_ENTITY_ID ? 0 : hovered));
        return;
    }

    if(request.reason == PickReason::ClickSelect) {
        Geometry::EntityId picked = Geometry::INVALID_ENTITY_ID;
        if(!ids.empty()) {
            picked = ids.front();
        }
        if(picked == Geometry::INVALID_ENTITY_ID) {
            if(!request.additive) {
                // Click empty space clears selection
                clearPickedSelection();
            }
            return;
        }

        if(request.additive) {
            // Ctrl+click toggles
            const bool is_selected =
                std::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(), picked) !=
                m_selectedEntityIds.end();
            if(is_selected) {
                removeFromSelectionInternal({picked});
            } else {
                setSelectionInternal({picked}, false);
            }
        } else {
            setSelectionInternal({picked}, true);
        }
        return;
    }

    if(request.reason == PickReason::BoxSelect) {
        setSelectionInternal(ids, !request.additive);
        return;
    }

    if(request.reason == PickReason::BoxDeselect) {
        removeFromSelectionInternal(ids);
        return;
    }
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
    m_selectionMode = static_cast<Geometry::SelectionMode>(viewport->selectionMode());

    // Check for pending pick requests
    if(viewport->m_pendingPickRequest.has_value()) {
        m_pickRequest = viewport->m_pendingPickRequest;
        m_pickResult = &(const_cast<std::vector<Geometry::EntityId>&>(viewport->m_pickResult));
        viewport->m_pendingPickRequest.reset();
    } else {
        m_pickRequest.reset();
        m_pickResult = nullptr;
    }

    // Check if render data changed using version number
    const auto& new_render_data = viewport->renderData();
    if(new_render_data.m_version != m_renderData.m_version) {
        LOG_DEBUG("GLViewportRenderer: Render data changed, version {} -> {}, uploading {} meshes",
                  m_renderData.m_version, new_render_data.m_version, new_render_data.meshCount());
        m_renderData = new_render_data;
        m_needsDataUpload = true;
    }
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

    // Delegate rendering to SceneRenderer
    m_sceneRenderer->render(m_cameraState.m_position, view, projection);

    // Perform picking if requested
    if(m_pickRequest.has_value() && m_pickResult != nullptr) {
        performPicking();
    }
}

void GLViewportRenderer::performPicking() {
    if(!m_pickRequest.has_value() || m_pickResult == nullptr) {
        return;
    }

    const auto& req = m_pickRequest.value();

    // Get OpenGL functions from current context
    QOpenGLFunctions* gl = QOpenGLContext::currentContext()->functions();
    if(!gl) {
        LOG_ERROR("Failed to get OpenGL functions for picking");
        m_pickRequest.reset();
        return;
    }

    // Create or resize picking FBO if needed
    if(!m_pickingFbo || m_pickingFbo->size() != m_viewportSize) {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(0); // No MSAA for picking (we need exact colors)
        m_pickingFbo = std::make_unique<QOpenGLFramebufferObject>(m_viewportSize, format);
    }

    // Bind picking FBO and render
    m_pickingFbo->bind();

    const float aspect_ratio = m_viewportSize.width() / static_cast<float>(m_viewportSize.height());
    const QMatrix4x4 projection = m_cameraState.projectionMatrix(aspect_ratio);
    const QMatrix4x4 view = m_cameraState.viewMatrix();

    m_sceneRenderer->renderForPicking(view, projection, m_selectionMode);

    // Read back pixel(s)
    m_pickResult->clear();

    if(req.m_isRect) {
        // Rectangle selection - read multiple pixels
        int width = req.m_x2 - req.m_x;
        int height = req.m_y2 - req.m_y;

        if(width > 0 && height > 0) {
            std::vector<uint8_t> pixels(width * height * 4);

            // Flip Y coordinate for OpenGL (origin at bottom-left)
            int gl_y = m_viewportSize.height() - req.m_y2;

            gl->glReadPixels(req.m_x, gl_y, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                             pixels.data());

            // Collect unique entity IDs
            std::unordered_set<Geometry::EntityId> unique_ids;
            for(int i = 0; i < width * height; ++i) {
                uint8_t r = pixels[i * 4 + 0];
                uint8_t g = pixels[i * 4 + 1];
                uint8_t b = pixels[i * 4 + 2];
                uint8_t a = pixels[i * 4 + 3];

                Geometry::EntityId id = Render::SceneRenderer::decodeEntityIdFromColor(r, g, b, a);
                if(id != Geometry::INVALID_ENTITY_ID) {
                    unique_ids.insert(id);
                }
            }

            for(const auto& id : unique_ids) {
                m_pickResult->push_back(id);
            }
        }
    } else {
        // Single point selection
        uint8_t pixel[4] = {0, 0, 0, 0};

        // Flip Y coordinate for OpenGL
        int gl_y = m_viewportSize.height() - req.m_y - 1;

        gl->glReadPixels(req.m_x, gl_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

        Geometry::EntityId id =
            Render::SceneRenderer::decodeEntityIdFromColor(pixel[0], pixel[1], pixel[2], pixel[3]);
        if(id != Geometry::INVALID_ENTITY_ID) {
            m_pickResult->push_back(id);
        }
    }

    m_pickingFbo->release();

    // Push result back to GUI thread for picking-mode state updates
    if(m_viewport && m_pickRequest.has_value()) {
        const auto req = m_pickRequest.value();
        if(req.reason != GLViewport::PickReason::Legacy) {
            auto* viewport = const_cast<GLViewport*>(m_viewport);
            const std::vector<Geometry::EntityId> ids_copy = *m_pickResult;
            QMetaObject::invokeMethod(
                viewport,
                [viewport, req, ids_copy]() { viewport->handlePickResult(req, ids_copy); },
                Qt::QueuedConnection);
        }
    }

    // Clear the request
    m_pickRequest.reset();
}

} // namespace OpenGeoLab::App
