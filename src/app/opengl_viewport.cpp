/**
 * @file opengl_viewport.cpp
 * @brief Implementation of GLViewport and GLViewportRenderer
 */

#include "app/opengl_viewport.hpp"
#include "render/select_manager.hpp"
#include "util/logger.hpp"

#include <QMouseEvent>
#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QWheelEvent>
#include <QtCore/QMetaObject>
#include <QtMath>
#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::App {
namespace {
[[nodiscard]] Render::TrackballController::Mode pickMode(Qt::MouseButtons buttons,
                                                         Qt::KeyboardModifiers modifiers) {
    if((buttons & Qt::LeftButton) && (modifiers & Qt::ControlModifier)) {
        return Render::TrackballController::Mode::Orbit;
    }

    if(((buttons & Qt::LeftButton) && (modifiers & Qt::ShiftModifier)) ||
       (buttons & Qt::MiddleButton)) {
        return Render::TrackballController::Mode::Pan;
    }

    if(buttons & Qt::RightButton) {
        return Render::TrackballController::Mode::Zoom;
    }

    return Render::TrackballController::Mode::None;
}
} // namespace
// =============================================================================
// GLViewport Implementation
// =============================================================================

GLViewport::GLViewport(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemHasContents, true);
    setMirrorVertically(true);
    setAcceptHoverEvents(true);

    auto& scene_controller = Render::RenderSceneController::instance();
    m_cameraState = scene_controller.camera();
    m_hasGeometry = scene_controller.hasGeometry();
    m_trackballController.syncFromCamera(m_cameraState);

    // Bridge service events (Util::Signal) onto the Qt/GUI thread
    m_sceneNeedsUpdateConn = scene_controller.subscribeSceneNeedsUpdate([this]() {
        QMetaObject::invokeMethod(this, &GLViewport::onSceneNeedsUpdate, Qt::QueuedConnection);
    });
    m_geometryChangedConn = scene_controller.subscribeGeometryChanged([this]() {
        QMetaObject::invokeMethod(this, &GLViewport::onServiceGeometryChanged,
                                  Qt::QueuedConnection);
    });

    auto& select_manager = Render::SelectManager::instance();
    m_pickSettingsChangedConn =
        select_manager.subscribePickSettingsChanged([this](Render::SelectManager::PickTypes) {
            QMetaObject::invokeMethod(this, &GLViewport::onSceneNeedsUpdate, Qt::QueuedConnection);
        });
    m_selectionChangedConn = select_manager.subscribeSelectionChanged(
        [this](Render::SelectManager::PickResult, Render::SelectManager::SelectionChangeAction) {
            QMetaObject::invokeMethod(this, &GLViewport::onSceneNeedsUpdate, Qt::QueuedConnection);
        });

    LOG_TRACE("GLViewport created");
}

GLViewport::~GLViewport() { LOG_TRACE("GLViewport destroyed"); }

Render::PickAction GLViewport::consumePickAction() {
    const auto action = m_pendingPickAction;
    m_pendingPickAction = Render::PickAction::None;
    return action;
}

QQuickFramebufferObject::Renderer* GLViewport::createRenderer() const {
    return new GLViewportRenderer(this);
}

bool GLViewport::hasGeometry() const { return m_hasGeometry; }

const Render::CameraState& GLViewport::cameraState() const { return m_cameraState; }

const Render::DocumentRenderData& GLViewport::renderData() const {
    static Render::DocumentRenderData empty;
    return Render::RenderSceneController::instance().renderData();
}

void GLViewport::onSceneNeedsUpdate() {
    m_cameraState = Render::RenderSceneController::instance().camera();
    if(!m_trackballController.isActive()) {
        m_trackballController.syncFromCamera(m_cameraState);
    }
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
    m_cursorPos = event->position();
    m_pressPos = event->position();
    m_movedSincePress = false;
    if(window()) {
        m_devicePixelRatio = window()->devicePixelRatio();
    }
    m_pressedButtons = event->buttons();

    event->accept();
}

void GLViewport::mouseMoveEvent(QMouseEvent* event) {
    if(!hasFocus()) {
        forceActiveFocus();
    }
    m_cursorPos = event->position();
    if(window()) {
        m_devicePixelRatio = window()->devicePixelRatio();
    }

    constexpr qreal drag_threshold = 4.0;
    if((m_pressedButtons != Qt::NoButton) && !m_movedSincePress) {
        const auto delta = m_cursorPos - m_pressPos;
        if((delta.x() * delta.x() + delta.y() * delta.y()) > (drag_threshold * drag_threshold)) {
            m_movedSincePress = true;
        }
    }

    // If modifiers/buttons changed mid-drag, switch mode seamlessly.
    const auto mode = pickMode(m_pressedButtons, m_pressedModifiers);
    if(mode != m_trackballController.mode()) {
        m_trackballController.end();
        m_trackballController.setViewportSize(size());
        if(mode != Render::TrackballController::Mode::None) {
            m_trackballController.begin(event->position(), mode, m_cameraState);
        }
    }

    if(mode != Render::TrackballController::Mode::None) {
        m_trackballController.setViewportSize(size());
        m_trackballController.update(event->position(), m_cameraState);
        Render::RenderSceneController::instance().setCamera(m_cameraState, false);
    }

    update();
    event->accept();
}

void GLViewport::mouseReleaseEvent(QMouseEvent* event) {
    m_cursorPos = event->position();
    m_pressedButtons = event->buttons();

    if(m_trackballController.isActive() && m_pressedButtons == Qt::NoButton) {
        m_trackballController.end();
        Render::RenderSceneController::instance().setCamera(m_cameraState, true);
    }

    const bool pick_enabled = Render::SelectManager::instance().isPickEnabled();
    if(pick_enabled && !m_trackballController.isActive() && !m_movedSincePress) {
        if(event->button() == Qt::LeftButton) {
            m_pendingPickAction = Render::PickAction::Add;
            update();
        } else if(event->button() == Qt::RightButton) {
            m_pendingPickAction = Render::PickAction::Remove;
            update();
        }
    }

    event->accept();
}
void GLViewport::keyReleaseEvent(QKeyEvent* event) {
    m_pressedModifiers = event->modifiers();
    event->accept();
}

void GLViewport::wheelEvent(QWheelEvent* event) {
    // Default behavior: Ctrl + wheel zoom (keeps app-level scroll gestures safe).
    if((m_pressedModifiers & Qt::ControlModifier)) {
        m_trackballController.setViewportSize(size());
        const float steps = event->angleDelta().y() / 120.0f;
        m_trackballController.wheelZoom(steps * 2.0f, m_cameraState);
        Render::RenderSceneController::instance().setCamera(m_cameraState, true);
        update();
    }
    event->accept();
}

void GLViewport::hoverMoveEvent(QHoverEvent* event) {
    if(Render::SelectManager::instance().isPickEnabled()) {
        m_cursorPos = event->position();
        if(window()) {
            m_devicePixelRatio = window()->devicePixelRatio();
        }
        update();
    }
    event->accept();
}
// =============================================================================
// GLViewportRenderer Implementation
// =============================================================================

GLViewportRenderer::GLViewportRenderer(const GLViewport* viewport)
    : m_viewport(viewport),
      m_sceneRenderer(
          g_ComponentFactory.createObjectWithID<Render::SceneRendererFactory>("SceneRenderer")) {
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
    m_cursorPos = viewport->cursorPos();
    m_devicePixelRatio = viewport->devicePixelRatio();
    m_itemSize = viewport->itemSize();

    m_pendingPickAction = viewport->consumePickAction();

    // Check if render data changed using version number
    const auto& new_render_data = viewport->renderData();
    if(new_render_data.m_version != m_renderData.m_version) {
        LOG_DEBUG(
            "GLViewportRenderer: Render data changed, version {} -> {}, uploading {} entities",
            m_renderData.m_version, new_render_data.m_version, new_render_data.entityCount());
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

    // Picking is fully handled inside ISceneRenderer (checks pick-enable internally)
    Render::PickingInput pick_input;
    pick_input.m_cursorPos = m_cursorPos;
    pick_input.m_itemSize = m_itemSize;
    pick_input.m_devicePixelRatio = m_devicePixelRatio;
    pick_input.m_viewMatrix = view;
    pick_input.m_projectionMatrix = projection;
    pick_input.m_action = m_pendingPickAction;
    m_sceneRenderer->processPicking(pick_input);
    m_pendingPickAction = Render::PickAction::None;

    // Delegate rendering to ISceneRenderer
    m_sceneRenderer->render(m_cameraState.m_position, view, projection);
}

} // namespace OpenGeoLab::App
