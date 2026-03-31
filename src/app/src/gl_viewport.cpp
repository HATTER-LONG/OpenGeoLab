/**
 * @file gl_viewport.cpp
 * @brief GLViewport implementation
 */

#include <opengeolab/app/gl_viewport.hpp>

#include <opengeolab/app/gl_viewport_renderer.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <QHoverEvent>
#include <QLineF>
#include <QMouseEvent>
#include <QWheelEvent>

#include <algorithm>

namespace OpenGeoLab::App {

namespace {

constexpr float DRAG_THRESHOLD_PIXELS = 4.0F;

[[nodiscard]] bool isValidPickMode(int mode) noexcept {
    return mode >= static_cast<int>(Render::PickMode::VEF) &&
           mode <= static_cast<int>(Render::PickMode::Part);
}

} // namespace

GLViewport::GLViewport(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setFlag(ItemAcceptsInputMethod, true);
    setMirrorVertically(true);
    m_camera.reset();
}

QQuickFramebufferObject::Renderer* GLViewport::createRenderer() const {
    return new GLViewportRenderer();
}

void GLViewport::setSceneGraph(Scene::SceneGraph* scene) {
    if(m_sceneGraph == scene) {
        return;
    }

    m_sceneGraph = scene;
    update();
}

void GLViewport::setPickingEnabled(bool enabled) {
    if(m_pickingEnabled == enabled) {
        return;
    }

    m_pickingEnabled = enabled;
    if(!m_pickingEnabled) {
        m_pendingPick = {};
    }
    Q_EMIT pickingEnabledChanged();
    update();
}

void GLViewport::setPickMode(int mode) {
    if(!isValidPickMode(mode)) {
        return;
    }

    const auto new_mode = static_cast<Render::PickMode>(mode);
    if(m_pickMode == new_mode) {
        return;
    }

    m_pickMode = new_mode;
    Q_EMIT pickModeChanged();
    update();
}

void GLViewport::setXRayMode(bool enabled) {
    if(m_xRayMode == enabled) {
        return;
    }

    m_xRayMode = enabled;
    Q_EMIT xRayModeChanged();
    update();
}

GLViewport::PendingPick GLViewport::consumePendingPick() {
    const PendingPick pending_pick = m_pendingPick;
    m_pendingPick = {};
    return pending_pick;
}

void GLViewport::notifyPickResult(const Render::PickResult& result) {
    if(!result.valid) {
        Q_EMIT pickCleared();
        return;
    }

    Q_EMIT entityPicked(static_cast<int>(result.shapeId), static_cast<int>(result.entityType),
                        static_cast<int>(result.localId));
}

void GLViewport::notifyHoverResult(const Render::PickResult& result) {
    if(!result.valid) {
        Q_EMIT pickCleared();
        return;
    }

    Q_EMIT entityHovered(static_cast<int>(result.shapeId), static_cast<int>(result.entityType),
                         static_cast<int>(result.localId));
}

void GLViewport::fitToScene() {
    if(m_sceneGraph == nullptr) {
        return;
    }

    m_trackball.fitToScene(m_sceneGraph->sceneBounds(), m_camera);
    update();
}

void GLViewport::setViewPreset(int preset) {
    if(preset < static_cast<int>(TrackballController::ViewPreset::Front) ||
       preset > static_cast<int>(TrackballController::ViewPreset::Isometric)) {
        return;
    }

    m_trackball.setViewPreset(static_cast<TrackballController::ViewPreset>(preset), m_camera);
    update();
}

void GLViewport::toggleXRay() { setXRayMode(!m_xRayMode); }

void GLViewport::mousePressEvent(QMouseEvent* event) {
    m_pressPos = event->position();
    m_movedSincePress = false;
    m_pressedButtons = event->buttons();
    m_pressedModifiers = event->modifiers();
    event->accept();
}

void GLViewport::mouseMoveEvent(QMouseEvent* event) {
    if(m_trackball.isActive()) {
        m_trackball.update(static_cast<float>(event->position().x()),
                           static_cast<float>(event->position().y()), m_camera);
        update();
        event->accept();
        return;
    }

    if(m_pressedButtons == Qt::NoButton) {
        event->ignore();
        return;
    }

    const QLineF drag_line(m_pressPos, event->position());
    if(!m_movedSincePress && drag_line.length() >= DRAG_THRESHOLD_PIXELS) {
        m_movedSincePress = true;
        m_trackball.setViewportSize(static_cast<float>(std::max(width(), 1.0)),
                                    static_cast<float>(std::max(height(), 1.0)));
        if(const auto mode = mapMouseMode(m_pressedButtons, m_pressedModifiers);
           mode != TrackballController::Mode::None) {
            m_trackball.begin(static_cast<float>(m_pressPos.x()),
                              static_cast<float>(m_pressPos.y()), mode, m_camera);
            m_trackball.update(static_cast<float>(event->position().x()),
                               static_cast<float>(event->position().y()), m_camera);
            update();
        }
    }

    event->accept();
}

void GLViewport::mouseReleaseEvent(QMouseEvent* event) {
    if(m_trackball.isActive()) {
        m_trackball.end();
        update();
    } else if(!m_movedSincePress && m_pickingEnabled && event->button() == Qt::LeftButton) {
        m_pendingPick = {true, static_cast<float>(event->position().x()),
                         static_cast<float>(event->position().y())};
        update();
    }

    m_pressedButtons &= ~event->button();
    if(m_pressedButtons == Qt::NoButton) {
        m_pressedModifiers = Qt::NoModifier;
    }

    event->accept();
}

void GLViewport::wheelEvent(QWheelEvent* event) {
    if((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier) {
        const float steps = static_cast<float>(event->angleDelta().y()) / 120.0F;
        if(steps != 0.0F) {
            m_trackball.setViewportSize(static_cast<float>(std::max(width(), 1.0)),
                                        static_cast<float>(std::max(height(), 1.0)));
            m_trackball.wheelZoom(steps * 2.0F, m_camera);
            update();
        }
        event->accept();
        return;
    }

    event->ignore();
}

void GLViewport::hoverMoveEvent(QHoverEvent* event) {
    m_hoverPos = event->position();
    m_hoverActive = true;
    if(m_pickingEnabled) {
        update();
    }
    event->accept();
}

TrackballController::Mode GLViewport::mapMouseMode(Qt::MouseButtons buttons,
                                                   Qt::KeyboardModifiers modifiers) const {
    if((modifiers & Qt::ControlModifier) == Qt::ControlModifier &&
       (buttons & Qt::LeftButton) == Qt::LeftButton) {
        return TrackballController::Mode::Orbit;
    }

    if(((modifiers & Qt::ShiftModifier) == Qt::ShiftModifier &&
        (buttons & Qt::LeftButton) == Qt::LeftButton) ||
       (buttons & Qt::MiddleButton) == Qt::MiddleButton) {
        return TrackballController::Mode::Pan;
    }

    if((buttons & Qt::RightButton) == Qt::RightButton) {
        return TrackballController::Mode::Zoom;
    }

    return TrackballController::Mode::None;
}

} // namespace OpenGeoLab::App
