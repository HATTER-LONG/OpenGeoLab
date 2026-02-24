#include "render_sceneImpl.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Render {
RenderSceneImpl::RenderSceneImpl() {
    LOG_DEBUG("RenderSceneImpl: Created new render scene instance");
}
RenderSceneImpl::~RenderSceneImpl() {
    LOG_DEBUG("RenderSceneImpl: Destroying render scene instance");
}

void RenderSceneImpl::initialize() { LOG_DEBUG("RenderSceneImpl: Initializing render scene"); }

[[nodiscard]] bool RenderSceneImpl::isInitialized() const { return true; }

void RenderSceneImpl::setViewportSize(const QSize& size) {
    LOG_DEBUG("RenderSceneImpl: Viewport size set to {}x{}", size.width(), size.height());
}

void RenderSceneImpl::processPicking(const PickingInput& input) {
    LOG_DEBUG("RenderSceneImpl: Processing picking at cursor position ({}, {}) with action {}",
              input.m_cursorPos.x(), input.m_cursorPos.y(), static_cast<int>(input.m_action));
}

void RenderSceneImpl::render(const QVector3D& camera_pos,
                             const QMatrix4x4& view_matrix,
                             const QMatrix4x4& projection_matrix) {
    LOG_DEBUG("RenderSceneImpl: Rendering scene with camera at ({}, {}, {})", camera_pos.x(),
              camera_pos.y(), camera_pos.z());
}

void RenderSceneImpl::cleanup() { LOG_DEBUG("RenderSceneImpl: Cleaning up render scene"); }

} // namespace OpenGeoLab::Render