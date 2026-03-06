#include "render_sceneImpl.hpp"
#include "pass/render_pass_context.hpp"
#include "render/render_select_manager.hpp"
#include "render/render_types.hpp"

#include "util/color_map.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Render {
namespace {

constexpr int K_PICK_REGION_RADIUS = 6;

template <typename... TPasses> void initializePasses(TPasses&... passes) {
    (passes.initialize(), ...);
}

template <typename... TPasses> void cleanupPasses(TPasses&... passes) { (passes.cleanup(), ...); }

} // anonymous namespace

RenderSceneImpl::RenderSceneImpl() { LOG_DEBUG("RenderSceneImpl: Created"); }

RenderSceneImpl::~RenderSceneImpl() { LOG_DEBUG("RenderSceneImpl: Destroyed"); }

void RenderSceneImpl::initialize() {
    if(m_initialized) {
        return;
    }

    // TODO(layton) - Initialize rendering resources here (e.g., shaders, buffers, etc.)
    m_geometryBuffer.initialize();
    initializePasses(m_opaquePass, m_wireframePass, m_highlightPass);
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

    if(!m_selectionPass.isInitialized()) {
        m_selectionPass.initialize(size.width(), size.height());
    } else {
        m_selectionPass.resize(size.width(), size.height());
    }
}

void RenderSceneImpl::synchronize(const SceneFrameState& state) {
    m_frameState = state;
    if(!m_initialized || !state.m_renderData) {
        return;
    }

    const auto& render_data = *state.m_renderData;
    if(render_data.m_geometryVersion != m_geometryDataVersion ||
       render_data.m_meshVersion != m_meshDataVersion) {
        // Update pick resolver reference data for hierarchy lookups
        m_pickResolver.setPickData(render_data.m_pickData);
    }
    if(render_data.m_geometryVersion != m_geometryDataVersion) {
        auto pass_it = render_data.m_passData.find(RenderPassType::Geometry);
        if(pass_it != render_data.m_passData.end()) {
            if(!m_geometryBuffer.upload(pass_it->second)) {
                LOG_ERROR("RenderSceneImpl: Failed to upload geometry GPU buffer");
            }
        }
        m_geometryTriangleRanges = render_data.m_geometryTriangleRanges;
        m_geometryLineRanges = render_data.m_geometryLineRanges;
        m_geometryPointRanges = render_data.m_geometryPointRanges;
        m_geometryDataVersion = render_data.m_geometryVersion;
    }

    if(render_data.m_meshVersion != m_meshDataVersion) {
        // TODO(layton) - Rebuild mesh-related resources here
        m_meshDataVersion = render_data.m_meshVersion;
    }
}

void RenderSceneImpl::render() {
    if(!m_initialized) {
        return;
    }

    auto* f = QOpenGLContext::currentContext()->functions();

    // Always reset the viewport to the current window size before rendering.
    // This is required after resize events: the selection pass FBO may have
    // set a different viewport, and processHover/processPicking may not have
    // been called yet to restore it.
    f->glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

    f->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    f->glDepthMask(GL_TRUE);
    f->glDisable(GL_BLEND);
    f->glDisable(GL_CULL_FACE);
    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LESS);

    const auto& bg = Util::ColorMap::instance().getBackgroundColor();
    f->glClearColor(bg.m_r, bg.m_g, bg.m_b, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderPassContext pass_context{
        {m_frameState.m_viewMatrix, m_frameState.m_projMatrix, m_frameState.m_cameraPos,
         m_frameState.m_xRayMode},
        {m_geometryBuffer, m_geometryTriangleRanges, m_geometryLineRanges, m_geometryPointRanges},
        {m_meshBuffer, m_meshTriangleRanges, m_meshLineRanges, m_meshPointRanges}};

    // --- Pass 1: Surfaces ---
    m_opaquePass.render(pass_context);

    // --- Pass 2: Wireframe ---
    m_wireframePass.render(pass_context);

    // --- Pass 3: Highlight (selected/hovered entity overdraw) ---
    auto& select_mgr = RenderSelectManager::instance();
    const bool has_hover = select_mgr.hoveredEntity().m_type != RenderEntityType::None;
    const bool has_selection = !select_mgr.selections().empty();
    if(has_hover || has_selection) {
        m_highlightPass.render(pass_context);
    }
};

void RenderSceneImpl::processHover(int pixel_x, int pixel_y) {
    LOG_DEBUG("RenderSceneImpl: Processing hover at pixel position ({}, {})", pixel_x, pixel_y);
    if(!m_initialized || !m_selectionPass.isInitialized()) {
        return;
    }

    auto& select_mgr = RenderSelectManager::instance();
    const RenderEntityTypeMask pick_mask = select_mgr.getPickTypes();
    if(!select_mgr.isPickEnabled() || pick_mask == RenderEntityTypeMask::None) {
        select_mgr.clearHover();
        return;
    }

    // Expand mask for Wire/Part/Solid modes so their sub-entities are rendered.
    const RenderEntityTypeMask effective_mask = m_pickResolver.computeEffectiveMask(pick_mask);

    RenderPassContext pass_context{
        {m_frameState.m_viewMatrix, m_frameState.m_projMatrix, m_frameState.m_cameraPos,
         m_frameState.m_xRayMode, effective_mask},
        {m_geometryBuffer, m_geometryTriangleRanges, m_geometryLineRanges, m_geometryPointRanges},
        {m_meshBuffer, m_meshTriangleRanges, m_meshLineRanges, m_meshPointRanges}};
    m_selectionPass.render(pass_context);

    // Restore main framebuffer viewport
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

    // Read 7x7 region around cursor for priority-based picking
    const auto pick_ids = m_selectionPass.readPickRegion(pixel_x, pixel_y, K_PICK_REGION_RADIUS);

    if(pick_ids.empty()) {
        select_mgr.clearHover();
        return;
    }

    // Resolve hover entity.
    // Try Add first (unselected entity the user can select); if nothing found try
    // Remove (already-selected entity the user can deselect via right-click).
    // This ensures that selected wires / parts still show hover feedback.
    auto resolved = m_pickResolver.resolve(pick_ids, PickAction::Add, pick_mask);
    if(!resolved.isValid()) {
        resolved = m_pickResolver.resolve(pick_ids, PickAction::Remove, pick_mask);
    }
    if(!resolved.isValid()) {
        select_mgr.clearHover();
        return;
    }

    select_mgr.setHoverEntity(resolved.m_uid, resolved.m_type, resolved.m_partUid,
                              resolved.m_wireUid);

    // For Wire mode: pass the full edge set so the highlight pass can draw the whole loop.
    if(resolved.m_type == RenderEntityType::Wire) {
        const auto& wire_edges = m_pickResolver.wireEdges(resolved.m_uid);
        select_mgr.setHoveredWireEdges(wire_edges);
    } else {
        select_mgr.setHoveredWireEdges({});
    }
}

void RenderSceneImpl::processPicking(const PickingInput& input) {
    if(!m_initialized || !m_selectionPass.isInitialized()) {
        return;
    }

    if(input.m_action == PickAction::None) {
        return;
    }

    auto& select_mgr = RenderSelectManager::instance();
    // Convert cursor position to framebuffer pixel coordinates (flip Y for OpenGL)
    const int px = static_cast<int>(input.m_cursorPos.x() * input.m_devicePixelRatio);
    const int py = static_cast<int>((input.m_itemSize.height() - input.m_cursorPos.y()) *
                                    input.m_devicePixelRatio);

    const RenderEntityTypeMask pick_mask = select_mgr.getPickTypes();
    const RenderEntityTypeMask effective_mask = m_pickResolver.computeEffectiveMask(pick_mask);

    RenderPassContext pass_context{
        {m_frameState.m_viewMatrix, m_frameState.m_projMatrix, m_frameState.m_cameraPos,
         m_frameState.m_xRayMode, effective_mask},
        {m_geometryBuffer, m_geometryTriangleRanges, m_geometryLineRanges, m_geometryPointRanges},
        {m_meshBuffer, m_meshTriangleRanges, m_meshLineRanges, m_meshPointRanges}};

    m_selectionPass.render(pass_context);

    // Restore main framebuffer viewport
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

    const auto pick_ids = m_selectionPass.readPickRegion(px, py, K_PICK_REGION_RADIUS);
    if(pick_ids.empty()) {
        return;
    }

    // Resolver handles mode detection, aggregate promotion, and all constraints.
    const auto resolved = m_pickResolver.resolve(pick_ids, input.m_action, pick_mask);
    if(!resolved.isValid()) {
        return;
    }

    if(input.m_action == PickAction::Add) {
        select_mgr.addSelection(resolved.m_uid, resolved.m_type);
        if(resolved.m_type == RenderEntityType::Wire) {
            const auto& wire_edges = m_pickResolver.wireEdges(resolved.m_uid);
            select_mgr.addSelectedWireEdges(resolved.m_uid, wire_edges);
        }
    } else if(input.m_action == PickAction::Remove) {
        select_mgr.removeSelection(resolved.m_uid, resolved.m_type);
        if(resolved.m_type == RenderEntityType::Wire) {
            select_mgr.removeSelectedWireEdges(resolved.m_uid);
        }
    }
}

void RenderSceneImpl::cleanup() {
    if(!m_initialized) {
        return;
    }
    m_initialized = false;
    m_selectionPass.cleanup();
    m_geometryDataVersion = 0;
    m_meshDataVersion = 0;
    LOG_DEBUG("RenderSceneImpl: Cleaning up render scene");
}

} // namespace OpenGeoLab::Render
