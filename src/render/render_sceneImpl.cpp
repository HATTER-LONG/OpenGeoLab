#include "render_sceneImpl.hpp"
#include "render/render_select_manager.hpp"

#include "util/color_map.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Render {
RenderSceneImpl::RenderSceneImpl() { LOG_DEBUG("RenderSceneImpl: Created"); }

RenderSceneImpl::~RenderSceneImpl() { LOG_DEBUG("RenderSceneImpl: Destroyed"); }

void RenderSceneImpl::initialize() {
    if(m_initialized) {
        return;
    }

    // TODO(layton) - Initialize rendering resources here (e.g., shaders, buffers, etc.)
    m_geometryPass.initialize();

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
    m_geometryPass.updateBuffers(render_data);

    // Rebuild pick resolver when geometry changes
    if(render_data.m_geometryVersion != m_geometryDataVersion) {
        // TODO(layton) - Rebuild pick resolver resources here
    }

    if(render_data.m_meshVersion != m_meshDataVersion) {
        // TODO(layton) - Rebuild mesh-related resources here
    }
}

void RenderSceneImpl::render() {
    if(!m_initialized) {
        return;
    }

    auto* f = QOpenGLContext::currentContext()->functions();

    f->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    f->glDepthMask(GL_TRUE);
    f->glDisable(GL_BLEND);
    f->glDisable(GL_CULL_FACE);
    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LESS);

    const auto& bg = Util::ColorMap::instance().getBackgroundColor();
    f->glClearColor(bg.m_r, bg.m_g, bg.m_b, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_geometryPass.render(m_frameState.m_viewMatrix, m_frameState.m_projMatrix,
                          m_frameState.m_cameraPos, m_frameState.m_xRayMode);
}

void RenderSceneImpl::processHover(int pixel_x, int pixel_y) {
    LOG_DEBUG("RenderSceneImpl: Processing hover at pixel position ({}, {})", pixel_x, pixel_y);
    if(!m_initialized || !m_pickPassInitialized) {
        return;
    }

    auto& select_mgr = RenderSelectManager::instance();
    if(!select_mgr.isPickEnabled()) {
        select_mgr.clearHover();
        return;
    }

    const RenderEntityTypeMask pick_mask = select_mgr.getPickTypes();
    if(pick_mask == RenderEntityTypeMask::None) {
        select_mgr.clearHover();
        return;
    }
}

void RenderSceneImpl::processPicking(const PickingInput& input) {
    LOG_DEBUG("RenderSceneImpl: Processing picking at cursor position ({}, {}) with action {}",
              input.m_cursorPos.x(), input.m_cursorPos.y(), static_cast<int>(input.m_action));
}

void RenderSceneImpl::cleanup() { LOG_DEBUG("RenderSceneImpl: Cleaning up render scene"); }

} // namespace OpenGeoLab::Render