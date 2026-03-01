#include "render_sceneImpl.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Render {
RenderSceneImpl::RenderSceneImpl() { LOG_DEBUG("RenderSceneImpl: Created"); }

RenderSceneImpl::~RenderSceneImpl() { LOG_DEBUG("RenderSceneImpl: Destroyed"); }

void RenderSceneImpl::initialize() {
    if(m_initialized) {
        return;
    }

    // TODO(layton) - Initialize rendering resources here (e.g., shaders, buffers, etc.)

    m_initialized = true;

    LOG_DEBUG("RenderSceneImpl: Initializing render scene");
}

[[nodiscard]] bool RenderSceneImpl::isInitialized() const { return m_initialized; }

void RenderSceneImpl::setViewportSize(const QSize& size) {
    if(size == m_viewportSize) {
        return;
    }
    m_viewportSize = size;
    LOG_DEBUG("RenderSceneImpl: Viewport size set to {}x{}", size.width(), size.height());
    if(!m_initialized) {
        return;
    }

    if(!m_pickPassInitialized) {
        // TODO(layton) - Initialize picking pass resources here
    }
}

void RenderSceneImpl::synchronize(const SceneFrameState& state) {
    m_frameState = state;
    if(!m_initialized || !state.m_renderData) {
        return;
    }

    const auto& render_data = *state.m_renderData;

    // TODO(layton) - Update rendering resources based on the new frame state (e.g., update buffers,
    // textures, etc.)

    // Rebuild pick resolver when geometry changes
    if(render_data.m_geometryVersion != m_geometryDataVersion) {
        // TODO(layton) - Rebuild pick resolver resources here
    }

    if(render_data.m_meshVersion != m_meshDataVersion) {
        // TODO(layton) - Rebuild mesh-related resources here
    }
}

void RenderSceneImpl::render() { LOG_DEBUG("RenderSceneImpl: Rendering frame"); }

void RenderSceneImpl::processHover(int pixel_x, int pixel_y) {
    LOG_DEBUG("RenderSceneImpl: Processing hover at pixel position ({}, {})", pixel_x, pixel_y);
}

void RenderSceneImpl::processPicking(const PickingInput& input) {
    LOG_DEBUG("RenderSceneImpl: Processing picking at cursor position ({}, {}) with action {}",
              input.m_cursorPos.x(), input.m_cursorPos.y(), static_cast<int>(input.m_action));
}

void RenderSceneImpl::cleanup() { LOG_DEBUG("RenderSceneImpl: Cleaning up render scene"); }

} // namespace OpenGeoLab::Render