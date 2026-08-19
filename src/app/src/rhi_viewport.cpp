/**
 * @file rhi_viewport.cpp
 * @brief RhiViewport implementation
 */

#include <opengeolab/app/rhi_viewport.hpp>

#include <opengeolab/app/rhi_viewport_renderer.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/viewport_state.hpp>

#include <QHoverEvent>
#include <QLineF>
#include <QMetaObject>
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

RhiViewport::RhiViewport(QQuickItem* parent) : QQuickRhiItem(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setFlag(ItemAcceptsInputMethod, true);
    // All render passes use QRhi::clipSpaceCorrMatrix(), so their output already
    // follows the backend framebuffer convention. Mirroring the item again makes
    // the live Qt Quick composition upside-down even though texture readback is
    // correct.
    setMirrorVertically(false);
    setSampleCount(4);
}

QQuickRhiItemRenderer* RhiViewport::createRenderer() { return new RhiViewportRenderer(); }

void RhiViewport::setSceneGraph(Scene::SceneGraph* scene) {
    if(m_sceneGraph == scene) {
        return;
    }

    m_displayModeConnection = {};
    m_sceneGraph = scene;

    if(m_sceneGraph != nullptr) {
        // Sync initial display mode from ViewportState.
        m_xRayMode = m_sceneGraph->viewportState().xRayMode();
        m_showTessellation = m_sceneGraph->viewportState().showTessellation();

        // Keep Q_PROPERTYs in sync when actions change display mode.
        m_displayModeConnection =
            m_sceneGraph->viewportState().displayModeChanged.connect([this]() {
                QMetaObject::invokeMethod(this, [this]() {
                    if(m_sceneGraph == nullptr) {
                        return;
                    }
                    const bool xray = m_sceneGraph->viewportState().xRayMode();
                    const bool tess = m_sceneGraph->viewportState().showTessellation();
                    if(m_xRayMode != xray) {
                        m_xRayMode = xray;
                        Q_EMIT xRayModeChanged();
                    }
                    if(m_showTessellation != tess) {
                        m_showTessellation = tess;
                        Q_EMIT showTessellationChanged();
                    }
                    update();
                });
            });
    }

    update();
}

void RhiViewport::setPickingEnabled(bool enabled) {
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

void RhiViewport::setPickMode(int mode) {
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

void RhiViewport::setXRayMode(bool enabled) {
    if(m_xRayMode == enabled) {
        return;
    }

    m_xRayMode = enabled;
    if(m_sceneGraph != nullptr) {
        m_sceneGraph->viewportState().setXRayMode(enabled);
    }
    Q_EMIT xRayModeChanged();
    update();
}

RhiViewport::PendingPick RhiViewport::consumePendingPick() {
    const PendingPick pending_pick = m_pendingPick;
    m_pendingPick = {};
    return pending_pick;
}

RhiViewport::PendingBoxSelect RhiViewport::consumePendingBoxSelect() {
    const PendingBoxSelect pending = m_pendingBoxSelect;
    m_pendingBoxSelect = {};
    return pending;
}

void RhiViewport::notifyPickResult(const Render::PickResult& result) {
    if(!result.valid) {
        Q_EMIT pickCleared();
        return;
    }

    Q_EMIT entityPicked(static_cast<int>(result.shapeId), static_cast<int>(result.entityType),
                        static_cast<int>(result.localId));
}

void RhiViewport::notifyHoverResult(const Render::PickResult& result) {
    if(!result.valid) {
        Q_EMIT pickCleared();
        return;
    }

    Q_EMIT entityHovered(static_cast<int>(result.shapeId), static_cast<int>(result.entityType),
                         static_cast<int>(result.localId));
}

void RhiViewport::fitToScene() {
    if(m_sceneGraph == nullptr) {
        return;
    }
    m_sceneGraph->viewportState().fitToBounds(m_sceneGraph->sceneBounds());
    update();
}

void RhiViewport::setViewPreset(int preset) {
    if(m_sceneGraph == nullptr) {
        return;
    }
    if(preset < static_cast<int>(Scene::ViewPreset::Front) ||
       preset > static_cast<int>(Scene::ViewPreset::Isometric)) {
        return;
    }
    m_sceneGraph->viewportState().setViewPreset(static_cast<Scene::ViewPreset>(preset));
    update();
}

void RhiViewport::toggleXRay() { setXRayMode(!m_xRayMode); }

void RhiViewport::setShowTessellation(bool enabled) {
    if(m_showTessellation == enabled) {
        return;
    }

    m_showTessellation = enabled;
    if(m_sceneGraph != nullptr) {
        m_sceneGraph->viewportState().setShowTessellation(enabled);
    }
    Q_EMIT showTessellationChanged();
    update();
}

void RhiViewport::toggleShowTessellation() { setShowTessellation(!m_showTessellation); }

void RhiViewport::mousePressEvent(QMouseEvent* event) {
    m_pressPos = event->position();
    m_movedSincePress = false;
    m_pressedButtons = event->buttons();
    m_pressedModifiers = event->modifiers();
    event->accept();
}

void RhiViewport::mouseMoveEvent(QMouseEvent* event) {
    if(m_trackball.isActive()) {
        if(m_sceneGraph != nullptr) {
            auto state = m_sceneGraph->viewportState().camera();
            m_trackball.update(static_cast<float>(event->position().x()),
                               static_cast<float>(event->position().y()), state);
            m_sceneGraph->viewportState().setCamera(state);
        }
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

        const auto mode = mapMouseMode(m_pressedButtons, m_pressedModifiers);
        if(mode != TrackballController::Mode::None) {
            m_trackball.setViewportSize(static_cast<float>(std::max(width(), 1.0)),
                                        static_cast<float>(std::max(height(), 1.0)));
            if(m_sceneGraph != nullptr) {
                auto state = m_sceneGraph->viewportState().camera();
                m_trackball.begin(static_cast<float>(m_pressPos.x()),
                                  static_cast<float>(m_pressPos.y()), mode, state);
                m_trackball.update(static_cast<float>(event->position().x()),
                                   static_cast<float>(event->position().y()), state);
                m_sceneGraph->viewportState().setCamera(state);
            }
            update();
        } else if(m_selectionActive) {
            m_boxSelectActive = true;
            Q_EMIT boxSelectActiveChanged();
        }
    }

    if(m_boxSelectActive) {
        m_boxSelectRect = QRectF(m_pressPos, event->position()).normalized();
        Q_EMIT boxSelectRectChanged();
        update();
    }

    event->accept();
}

void RhiViewport::mouseReleaseEvent(QMouseEvent* event) {
    if(m_trackball.isActive()) {
        m_trackball.end();
        update();
    } else if(!m_movedSincePress && m_pickingEnabled) {
        if(event->button() == Qt::LeftButton) {
            m_pendingPick = {true, static_cast<float>(event->position().x()),
                             static_cast<float>(event->position().y()), Core::PickAction::Add};
            update();
        } else if(event->button() == Qt::RightButton && m_selectionActive) {
            m_pendingPick = {true, static_cast<float>(event->position().x()),
                             static_cast<float>(event->position().y()), Core::PickAction::Remove};
            update();
        }
    } else if(m_movedSincePress && m_pickingEnabled && m_boxSelectActive) {
        m_pendingBoxSelect = {true,
                              static_cast<float>(m_pressPos.x()),
                              static_cast<float>(m_pressPos.y()),
                              static_cast<float>(event->position().x()),
                              static_cast<float>(event->position().y()),
                              (m_pressedButtons & Qt::LeftButton) != 0 ? Core::PickAction::Add
                                                                       : Core::PickAction::Remove};
        m_boxSelectActive = false;
        m_boxSelectRect = {};
        Q_EMIT boxSelectActiveChanged();
        Q_EMIT boxSelectRectChanged();
        update();
    }

    m_pressedButtons &= ~event->button();
    if(m_pressedButtons == Qt::NoButton) {
        m_pressedModifiers = Qt::NoModifier;
    }

    event->accept();
}

void RhiViewport::wheelEvent(QWheelEvent* event) {
    const float steps = static_cast<float>(event->angleDelta().y()) / 120.0F;
    if(steps != 0.0F && m_sceneGraph != nullptr) {
        m_trackball.setViewportSize(static_cast<float>(std::max(width(), 1.0)),
                                    static_cast<float>(std::max(height(), 1.0)));
        auto state = m_sceneGraph->viewportState().camera();
        const float speed = (event->modifiers() & Qt::ControlModifier) != 0 ? 2.0F : 1.0F;
        m_trackball.wheelZoom(steps * speed, state);
        m_sceneGraph->viewportState().setCamera(state);
        update();
        event->accept();
        return;
    }

    event->ignore();
}

TrackballController::Mode RhiViewport::mapMouseMode(Qt::MouseButtons buttons,
                                                    Qt::KeyboardModifiers modifiers) const {
    // Navigation remains available while selecting through Ctrl/Alt + left.
    if(((modifiers & (Qt::ControlModifier | Qt::AltModifier)) != 0 &&
        (buttons & Qt::LeftButton) != 0) ||
       (!m_selectionActive && (buttons & Qt::LeftButton) != 0)) {
        return TrackballController::Mode::Orbit;
    }

    if(((modifiers & Qt::ShiftModifier) != 0 && (buttons & Qt::LeftButton) != 0) ||
       (buttons & Qt::MiddleButton) != 0) {
        return TrackballController::Mode::Pan;
    }

    if((buttons & Qt::RightButton) != 0 && !m_selectionActive) {
        return TrackballController::Mode::Zoom;
    }

    return TrackballController::Mode::None;
}

void RhiViewport::hoverMoveEvent(QHoverEvent* event) {
    m_hoverPos = event->position();
    m_hoverActive = true;
    if(m_pickingEnabled) {
        update();
    }
    event->accept();
}

} // namespace OpenGeoLab::App
