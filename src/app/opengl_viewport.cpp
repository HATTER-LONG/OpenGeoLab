#include "app/opengl_viewport.hpp"
#include "render/render_scene_controller.hpp"
#include "render/trackball_controller.hpp"

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
    m_sceneNeedsUpdateConn = scene_controller.subscribeToSceneNeedsUpdate([this]() {
        QMetaObject::invokeMethod(this, &GLViewport::onSceneNeedsUpdate, Qt::QueuedConnection);
    });
}

GLViewport::~GLViewport() = default;

void GLViewport::onSceneNeedsUpdate() {
    m_cameraState = Render::RenderSceneController::instance().cameraState();
    if(!m_trackballController.isActive()) {
        m_trackballController.syncFromCamera(m_cameraState);
    }
    update();
}

} // namespace OpenGeoLab::App