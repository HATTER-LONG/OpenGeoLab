/**
 * @file opengl_viewport.cpp
 * @brief GLViewport input handling and GUI-thread scene update dispatch.
 *
 * Scene-update and selection-change signals are received via
 * Qt::QueuedConnection to safely cross from background threads
 * into the GUI thread.
 */

#include "app/opengl_viewport.hpp"
#include "opengl_viewport_render.hpp"
#include "render/render_scene_controller.hpp"
#include "render/render_select_manager.hpp"
#include "render/trackball_controller.hpp"

#include <QQuickWindow>
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

GLViewport::GLViewport(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemHasContents, true);
    setMirrorVertically(true);
    setAcceptHoverEvents(true);

    auto& scene_controller = Render::RenderSceneController::instance();
    m_cameraState = scene_controller.cameraState();
    m_sceneNeedsUpdateConn =
        scene_controller.subscribeToSceneNeedsUpdate([this](Render::SceneUpdateType type) {
            QMetaObject::invokeMethod(this, &GLViewport::onSceneNeedsUpdate, Qt::QueuedConnection,
                                      type);
        });
    m_cameraChangedConn = Render::RenderSelectManager::instance().subscribeSelectionChanged(
        [this](Render::PickResult, Render::SelectionChangeAction) {
            QMetaObject::invokeMethod(this, &GLViewport::onSceneNeedsUpdate, Qt::QueuedConnection,
                                      Render::SceneUpdateType::CameraChanged);
        });
}

GLViewport::~GLViewport() = default;

void GLViewport::onSceneNeedsUpdate(Render::SceneUpdateType type) {
    if(type == Render::SceneUpdateType::GeometryChanged) {
        emit geometryChanged();
    }
    m_cameraState = Render::RenderSceneController::instance().cameraState();
    if(!m_trackballController.isActive()) {
        m_trackballController.syncFromCamera(m_cameraState);
    }
    update();
}

QQuickFramebufferObject::Renderer* GLViewport::createRenderer() const {
    return new GLViewportRender();
}

void GLViewport::keyPressEvent(QKeyEvent* event) {
    m_pressedModifiers = event->modifiers();
    event->accept();
}

void GLViewport::keyReleaseEvent(QKeyEvent* event) {
    m_pressedModifiers = event->modifiers();
    event->accept();
}

void GLViewport::mousePressEvent(QMouseEvent* event) {
    if(!this->hasActiveFocus()) {
        this->forceActiveFocus();
    }

    m_cursorPos = event->position();
    m_pressPos = m_cursorPos;
    m_movedSincePress = false;
    if(window()) {
        m_devicePixelRatio = window()->devicePixelRatio();
    }

    m_pressedModifiers = QGuiApplication::queryKeyboardModifiers();
    m_pressedButtons = event->buttons();

    event->accept();
}

void GLViewport::mouseMoveEvent(QMouseEvent* event) {
    if(!this->hasActiveFocus()) {
        this->forceActiveFocus();
    }
    m_cursorPos = event->position();
    if(window()) {
        m_devicePixelRatio = window()->devicePixelRatio();
    }
    if(!m_movedSincePress && (m_cursorPos - m_pressPos).manhattanLength() > 5) {
        m_movedSincePress = true;
    }

    constexpr qreal drag_threshold = 4.0;
    if((m_pressedButtons != Qt::NoButton) && !m_movedSincePress) {
        const auto delta = m_cursorPos - m_pressPos;
        if((delta.x() * delta.x() + delta.y() * delta.y()) > (drag_threshold * drag_threshold)) {
            m_movedSincePress = true;
        }
    }

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

    bool need_update = false;
    if(m_trackballController.isActive() && m_pressedButtons == Qt::NoButton) {
        m_trackballController.end();
        Render::RenderSceneController::instance().setCamera(m_cameraState, false);
        need_update = true;
    }

    const bool pick_enabled = Render::RenderSelectManager::instance().isPickEnabled();
    if(pick_enabled && !m_trackballController.isActive() && !m_movedSincePress) {
        if(event->button() == Qt::LeftButton) {
            m_pendingPickAction = Render::PickAction::Add;
        } else if(event->button() == Qt::RightButton) {
            m_pendingPickAction = Render::PickAction::Remove;
        }
        need_update = true;
    }

    if(need_update) {
        update();
    }
    event->accept();
}

void GLViewport::wheelEvent(QWheelEvent* event) {
    if(!hasFocus()) {
        forceActiveFocus();
    }
    m_pressedModifiers = QGuiApplication::queryKeyboardModifiers();
    if((m_pressedModifiers & Qt::ControlModifier)) {
        m_trackballController.setViewportSize(size());
        const float steps = event->angleDelta().y() / 120.0f;
        m_trackballController.wheelZoom(steps * 2.0f, m_cameraState);
        Render::RenderSceneController::instance().setCamera(m_cameraState, false);
        update();
    }

    event->accept();
}

void GLViewport::hoverMoveEvent(QHoverEvent* event) {
    if(Render::RenderSelectManager::instance().isPickEnabled()) {
        const auto new_pos = event->position();
        if((new_pos - m_cursorPos).manhattanLength() > 2) {
            m_cursorPos = new_pos;
            if(window()) {
                m_devicePixelRatio = window()->devicePixelRatio();
            }
            update();
        }
    }
    event->accept();
}
} // namespace OpenGeoLab::App