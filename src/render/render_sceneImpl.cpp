/**
 * @file render_sceneImpl.cpp
 * @brief RenderSceneImpl — multi-pass rendering pipeline orchestration.
 *
 * Owns the shared GpuBuffer and delegates rendering to specialized passes:
 *   OpaquePass / TransparentPass → WireframePass → HighlightPass
 * SelectionPass runs on-demand from processHover()/processPicking().
 */

#include "render_sceneImpl.hpp"

#include "render/render_select_manager.hpp"

#include "util/color_map.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Render {

RenderSceneImpl::RenderSceneImpl() { LOG_DEBUG("RenderSceneImpl: Created"); }

RenderSceneImpl::~RenderSceneImpl() { LOG_DEBUG("RenderSceneImpl: Destroyed"); }

// =============================================================================
// Lifecycle
// =============================================================================

void RenderSceneImpl::initialize() {
    if(m_initialized) {
        return;
    }

    m_geometryBuffer.initialize();

    m_opaquePass.initialize();
    m_transparentPass.initialize();
    m_wireframePass.initialize();
    m_highlightPass.initialize();
    m_postProcessPass.initialize();
    m_uiPass.initialize();
    // SelectionPass initialized lazily when viewport size is known

    m_initialized = true;
    LOG_DEBUG("RenderSceneImpl: Initialized (multi-pass pipeline)");
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

    if(!m_selectionPassReady) {
        m_selectionPass.initialize(size.width(), size.height());
        m_selectionPassReady = true;
    } else {
        m_selectionPass.resize(size.width(), size.height());
    }
}

// =============================================================================
// Synchronize — transfer state from GUI thread to render thread
// =============================================================================

void RenderSceneImpl::synchronize(const SceneFrameState& state) {
    m_frameState = state;
    if(!m_initialized || !state.m_renderData) {
        return;
    }

    const auto& render_data = *state.m_renderData;

    // Upload geometry buffer and cache draw ranges when version changes
    if(render_data.m_geometryVersion != m_geometryDataVersion) {
        auto it = render_data.m_passData.find(RenderPassType::Geometry);
        if(it != render_data.m_passData.end()) {
            if(!m_geometryBuffer.upload(it->second)) {
                LOG_ERROR("RenderSceneImpl: Failed to upload geometry buffer");
                return;
            }
        }

        m_triangleRanges = render_data.m_geometryTriangleRanges;
        m_lineRanges = render_data.m_geometryLineRanges;
        m_pointRanges = render_data.m_geometryPointRanges;

        m_geometryDataVersion = render_data.m_geometryVersion;
    }

    if(render_data.m_meshVersion != m_meshDataVersion) {
        // TODO(layton) - Upload mesh buffer and cache mesh draw ranges
        m_meshDataVersion = render_data.m_meshVersion;
    }
}

// =============================================================================
// Render — execute the multi-pass pipeline
// =============================================================================

void RenderSceneImpl::render() {
    if(!m_initialized) {
        return;
    }

    auto* f = QOpenGLContext::currentContext()->functions();

    // Global GL state
    f->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    f->glDepthMask(GL_TRUE);
    f->glDisable(GL_BLEND);
    f->glDisable(GL_CULL_FACE);
    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LESS);

    // Clear to background color
    const auto& bg = Util::ColorMap::instance().getBackgroundColor();
    f->glClearColor(bg.m_r, bg.m_g, bg.m_b, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(m_geometryBuffer.vertexCount() == 0) {
        return;
    }

    // Pass 1: Triangles (opaque or transparent, mutually exclusive)
    if(!m_frameState.m_xRayMode) {
        m_opaquePass.render(f, m_geometryBuffer, m_frameState.m_viewMatrix,
                            m_frameState.m_projMatrix, m_frameState.m_cameraPos, m_triangleRanges);
    } else {
        m_transparentPass.render(f, m_geometryBuffer, m_frameState.m_viewMatrix,
                                 m_frameState.m_projMatrix, m_frameState.m_cameraPos,
                                 m_triangleRanges);
    }

    // Pass 2: Wireframe (lines + points)
    m_wireframePass.render(f, m_geometryBuffer, m_frameState.m_viewMatrix,
                           m_frameState.m_projMatrix, m_lineRanges, m_pointRanges);

    // Pass 3: Highlight (selected/hovered entities only)
    m_highlightPass.render(f, m_geometryBuffer, m_frameState.m_viewMatrix,
                           m_frameState.m_projMatrix, m_frameState.m_cameraPos, m_triangleRanges,
                           m_lineRanges, m_pointRanges);

    // Pass 4-5: Stubs for future use
    m_postProcessPass.render(f);
    m_uiPass.render(f);
}

// =============================================================================
// GPU Picking — hover and click selection
// =============================================================================

void RenderSceneImpl::processHover(int pixel_x, int pixel_y) {
    if(!m_initialized || !m_selectionPassReady) {
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

    auto* f = QOpenGLContext::currentContext()->functions();
    auto result = m_selectionPass.pick(f, m_geometryBuffer, m_frameState.m_viewMatrix,
                                       m_frameState.m_projMatrix, pixel_x, pixel_y, pick_mask,
                                       m_triangleRanges, m_lineRanges, m_pointRanges);

    if(!result.isValid()) {
        select_mgr.clearHover();
        return;
    }

    resolveAndSetHover(result);
}

void RenderSceneImpl::processPicking(const PickingInput& input) {
    if(!m_initialized || !m_selectionPassReady) {
        return;
    }

    auto& select_mgr = RenderSelectManager::instance();
    if(!select_mgr.isPickEnabled()) {
        return;
    }

    const RenderEntityTypeMask pick_mask = select_mgr.getPickTypes();
    if(pick_mask == RenderEntityTypeMask::None) {
        return;
    }

    // Convert cursor position to device pixel coordinates
    const int px = static_cast<int>(input.m_cursorPos.x() * input.m_devicePixelRatio);
    const int py = static_cast<int>((input.m_itemSize.height() - input.m_cursorPos.y()) *
                                    input.m_devicePixelRatio);

    auto* f = QOpenGLContext::currentContext()->functions();
    auto result = m_selectionPass.pick(f, m_geometryBuffer, m_frameState.m_viewMatrix,
                                       m_frameState.m_projMatrix, px, py, pick_mask,
                                       m_triangleRanges, m_lineRanges, m_pointRanges);

    resolveAndApplySelection(result, input.m_action);
}

// =============================================================================
// Pick resolution helpers
// =============================================================================

void RenderSceneImpl::resolveAndSetHover(const PickAnalysisResult& result) {
    auto& select_mgr = RenderSelectManager::instance();

    if(!m_frameState.m_renderData) {
        select_mgr.clearHover();
        return;
    }

    const auto& pick_data = m_frameState.m_renderData->m_pickData;

    uint64_t part_uid = 0;
    uint64_t wire_uid = 0;

    // Look up parent part
    auto part_it = pick_data.m_entityToPartUid.find(result.m_uid);
    if(part_it != pick_data.m_entityToPartUid.end()) {
        part_uid = part_it->second;
    }

    // For edges, resolve parent wire and set wire edge highlighting
    if(result.m_type == RenderEntityType::Edge) {
        auto wire_it = pick_data.m_edgeToWireUids.find(result.m_uid);
        if(wire_it != pick_data.m_edgeToWireUids.end() && !wire_it->second.empty()) {
            wire_uid = wire_it->second.front();
            // Set all edges in this wire for complete wire highlighting
            auto edges_it = pick_data.m_wireToEdgeUids.find(wire_uid);
            if(edges_it != pick_data.m_wireToEdgeUids.end()) {
                select_mgr.setHoveredWireEdges(edges_it->second);
            }
        }
    }

    select_mgr.setHoverEntity(result.m_uid, result.m_type, part_uid, wire_uid);
}

void RenderSceneImpl::resolveAndApplySelection(const PickAnalysisResult& result,
                                               PickAction action) {
    auto& select_mgr = RenderSelectManager::instance();

    if(!result.isValid()) {
        // Click on background: clear selection if adding
        if(action == PickAction::Add) {
            select_mgr.clearSelection();
        }
        return;
    }

    if(!m_frameState.m_renderData) {
        return;
    }

    const auto& pick_data = m_frameState.m_renderData->m_pickData;

    uint64_t select_uid = result.m_uid;
    RenderEntityType select_type = result.m_type;

    // If in Part pick mode, resolve entity to parent part
    if(select_mgr.isTypePickable(RenderEntityType::Part)) {
        auto it = pick_data.m_entityToPartUid.find(result.m_uid);
        if(it != pick_data.m_entityToPartUid.end()) {
            select_uid = it->second;
            select_type = RenderEntityType::Part;
        }
    }

    // If in Wire pick mode, resolve edge to parent wire
    if(select_mgr.isTypePickable(RenderEntityType::Wire) &&
       result.m_type == RenderEntityType::Edge) {
        auto it = pick_data.m_edgeToWireUids.find(result.m_uid);
        if(it != pick_data.m_edgeToWireUids.end() && !it->second.empty()) {
            select_uid = it->second.front();
            select_type = RenderEntityType::Wire;

            // Track wire edges for selection highlighting
            if(action == PickAction::Add) {
                auto edges_it = pick_data.m_wireToEdgeUids.find(select_uid);
                if(edges_it != pick_data.m_wireToEdgeUids.end()) {
                    select_mgr.addSelectedWireEdges(select_uid, edges_it->second);
                }
            }
        }
    }

    if(action == PickAction::Add) {
        select_mgr.addSelection(select_uid, select_type);
    } else if(action == PickAction::Remove) {
        select_mgr.removeSelection(select_uid, select_type);
        if(select_type == RenderEntityType::Wire) {
            select_mgr.removeSelectedWireEdges(select_uid);
        }
    }
}

// =============================================================================
// Cleanup
// =============================================================================

void RenderSceneImpl::cleanup() {
    if(!m_initialized) {
        return;
    }

    m_opaquePass.cleanup();
    m_transparentPass.cleanup();
    m_wireframePass.cleanup();
    m_highlightPass.cleanup();
    m_selectionPass.cleanup();
    m_postProcessPass.cleanup();
    m_uiPass.cleanup();
    m_geometryBuffer.cleanup();

    m_triangleRanges.clear();
    m_lineRanges.clear();
    m_pointRanges.clear();

    m_initialized = false;
    m_selectionPassReady = false;
    m_geometryDataVersion = 0;
    m_meshDataVersion = 0;
    LOG_DEBUG("RenderSceneImpl: Cleaned up (multi-pass pipeline)");
}

} // namespace OpenGeoLab::Render
