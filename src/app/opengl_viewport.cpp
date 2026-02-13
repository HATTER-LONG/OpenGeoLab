#include "app/opengl_viewport.hpp"

namespace OpenGeoLab::App {
GLViewport::GLViewport(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemHasContents, true);
    setMirrorVertically(true);
    setAcceptHoverEvents(true);
}
GLViewport::~GLViewport() = default;
} // namespace OpenGeoLab::App