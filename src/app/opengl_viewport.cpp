#include "app/opengl_viewport.hpp"
#include "render/render_scene_controller.hpp"

namespace OpenGeoLab::App {
GLViewport::GLViewport(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemHasContents, true);
    setMirrorVertically(true);
    setAcceptHoverEvents(true);

    auto& scene_controller = Render::RenderSceneController::instance();
}
GLViewport::~GLViewport() = default;
} // namespace OpenGeoLab::App