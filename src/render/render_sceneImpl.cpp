/**
 * @file render_sceneImpl.cpp
 * @brief RenderSceneImpl — orchestrates per-frame rendering, GPU picking, and
 *        hover detection using modular render passes.
 *
 * Render pipeline order:
 *   1. OpaquePass (or TransparentPass in X-ray mode) — surfaces
 *   2. WireframePass — edges and points
 *   3. HighlightPass — selected/hovered entity overdraw
 *   4. PostProcessPass — future post-processing (stub)
 *   5. UIPass — future UI overlay (stub)
 *
 * SelectionPass is invoked on demand from processHover/processPicking.
 */

#include "render_sceneImpl.hpp"

#include "render/render_select_manager.hpp"
#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {

namespace {

constexpr int kPickRegionRadius = 4;

template <typename... TPasses> void initializePasses(TPasses&... passes) {
    (passes.initialize(), ...);
}

template <typename... TPasses> void cleanupPasses(TPasses&... passes) { (passes.cleanup(), ...); }

} // anonymous namespace

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
    m_meshBuffer.initialize();
    initializePasses(m_opaquePass, m_transparentPass, m_wireframePass, m_highlightPass,
                     m_postProcessPass, m_uiPass);
    // SelectionPass is deferred until viewport size is known

    m_initialized = true;
    LOG_DEBUG("RenderSceneImpl: Initialized");
}

bool RenderSceneImpl::isInitialized() const { return m_initialized; }

void RenderSceneImpl::setViewportSize(const QSize& size) {
    if(size == m_viewportSize) {
        return;
    }

    m_viewportSize = size;
    LOG_DEBUG("RenderSceneImpl: Viewport size {}x{}", size.width(), size.height());

    if(!m_initialized) {
        return;
    }

    if(!m_selectionPassInitialized) {
        m_selectionPass.initialize(size.width(), size.height());
        m_selectionPassInitialized = true;
    } else {
        m_selectionPass.resize(size.width(), size.height());
    }
}

// =============================================================================
// Synchronize — cache GUI-thread state and update GPU buffers
// =============================================================================

void RenderSceneImpl::synchronize(const SceneFrameState& state) {
    m_frameState = state;

    if(!m_initialized || !state.renderData) {
        return;
    }

    const auto& renderData = *state.renderData;

    // Upload geometry GPU buffer (version-checked internally)
    {
        auto passIt = renderData.m_passData.find(RenderPassType::Geometry);
        if(passIt != renderData.m_passData.end()) {
            if(!m_geometryBuffer.upload(passIt->second)) {
                LOG_ERROR("RenderSceneImpl: Failed to upload geometry GPU buffer");
            }
        }

        // Copy pre-built draw ranges
        m_geometryTriangleRanges = renderData.m_geometryTriangleRanges;
        m_geometryLineRanges = renderData.m_geometryLineRanges;
        m_geometryPointRanges = renderData.m_geometryPointRanges;
    }

    // Upload mesh GPU buffer (version-checked internally)
    {
        auto passIt = renderData.m_passData.find(RenderPassType::Mesh);
        if(passIt != renderData.m_passData.end()) {
            if(!m_meshBuffer.upload(passIt->second)) {
                LOG_ERROR("RenderSceneImpl: Failed to upload mesh GPU buffer");
            }
        }

        m_meshTriangleRanges = renderData.m_meshTriangleRanges;
        m_meshLineRanges = renderData.m_meshLineRanges;
        m_meshPointRanges = renderData.m_meshPointRanges;
    }

    // Sync mesh display mode
    m_meshDisplayMode = state.meshDisplayMode;

    // Update pick resolver data reference
    m_pickResolver.setPickData(renderData.m_pickData);
}

// =============================================================================
// Rendering — uses cached m_frameState, no singleton access
// =============================================================================

void RenderSceneImpl::render() {
    if(!m_initialized) {
        return;
    }

    // Reset GL state — Qt's scene-graph renderer may leave colour-mask,
    // blending, or depth state in an arbitrary configuration.
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    f->glDepthMask(GL_TRUE);
    f->glDisable(GL_BLEND);
    f->glDisable(GL_CULL_FACE);
    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LESS);

    // Clear viewport
    const auto& bg = Util::ColorMap::instance().getBackgroundColor();
    f->glClearColor(bg.m_r, bg.m_g, bg.m_b, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Build unified render context
    RenderPassContext pass_ctx;
    pass_ctx.m_params.viewMatrix = m_frameState.viewMatrix;
    pass_ctx.m_params.projMatrix = m_frameState.projMatrix;
    pass_ctx.m_params.cameraPos = m_frameState.cameraPos;
    pass_ctx.m_params.xRayMode = m_frameState.xRayMode;

    pass_ctx.m_geometry.m_buffer = &m_geometryBuffer;
    pass_ctx.m_geometry.m_triangleRanges = &m_geometryTriangleRanges;
    pass_ctx.m_geometry.m_lineRanges = &m_geometryLineRanges;
    pass_ctx.m_geometry.m_pointRanges = &m_geometryPointRanges;

    pass_ctx.m_mesh.m_buffer = &m_meshBuffer;
    pass_ctx.m_mesh.m_triangleRanges = &m_meshTriangleRanges;
    pass_ctx.m_mesh.m_lineRanges = &m_meshLineRanges;
    pass_ctx.m_mesh.m_pointRanges = &m_meshPointRanges;
    pass_ctx.m_mesh.m_displayMode = m_meshDisplayMode;

    // --- Pass 1: Surfaces ---
    if(m_frameState.xRayMode) {
        m_transparentPass.render(pass_ctx);
    } else {
        m_opaquePass.render(pass_ctx);
    }

    // --- Pass 2: Wireframe ---
    m_wireframePass.render(pass_ctx);

    // --- Pass 3: Highlight (selected/hovered entity overdraw) ---
    auto& selectMgr = RenderSelectManager::instance();
    const bool has_hover = selectMgr.hoveredEntity().m_type != RenderEntityType::None;
    const bool has_selection = !selectMgr.selections().empty();
    if(has_hover || has_selection) {
        m_highlightPass.render(pass_ctx);
    }

    // --- Pass 4: Post-processing (stub) ---
    m_postProcessPass.render();

    // --- Pass 5: UI (stub) ---
    m_uiPass.render();
}

// =============================================================================
// Hover processing
// =============================================================================

void RenderSceneImpl::processHover(int pixel_x, int pixel_y) {
    if(!m_initialized || !m_selectionPassInitialized) {
        return;
    }

    auto& selectMgr = RenderSelectManager::instance();
    if(!selectMgr.isPickEnabled()) {
        selectMgr.clearHover();
        return;
    }

    const RenderEntityTypeMask pickMask = selectMgr.getPickTypes();
    if(pickMask == RenderEntityTypeMask::None) {
        selectMgr.clearHover();
        return;
    }

    // For Wire/Part mode, render sub-entities so we can resolve to parent
    RenderEntityTypeMask effectiveMask = pickMask;
    if(selectMgr.isTypePickable(RenderEntityType::Wire)) {
        effectiveMask = effectiveMask | RenderEntityTypeMask::Edge | RenderEntityTypeMask::Face;
    }
    if(selectMgr.isTypePickable(RenderEntityType::Part)) {
        effectiveMask = effectiveMask | RenderEntityTypeMask::Face | RenderEntityTypeMask::Edge;
    }

    RenderPassContext pick_ctx;
    pick_ctx.m_params.viewMatrix = m_frameState.viewMatrix;
    pick_ctx.m_params.projMatrix = m_frameState.projMatrix;
    pick_ctx.m_geometry.m_buffer = &m_geometryBuffer;
    pick_ctx.m_geometry.m_triangleRanges = &m_geometryTriangleRanges;
    pick_ctx.m_geometry.m_lineRanges = &m_geometryLineRanges;
    pick_ctx.m_geometry.m_pointRanges = &m_geometryPointRanges;
    pick_ctx.m_mesh.m_buffer = &m_meshBuffer;
    pick_ctx.m_mesh.m_triangleRanges = &m_meshTriangleRanges;
    pick_ctx.m_mesh.m_lineRanges = &m_meshLineRanges;
    pick_ctx.m_mesh.m_pointRanges = &m_meshPointRanges;

    // Render to pick FBO
    m_selectionPass.renderToFbo(pick_ctx, effectiveMask);

    // Restore main framebuffer viewport
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

    // Read 7x7 region around cursor for priority-based picking
    const auto pickIds = m_selectionPass.readPickRegion(pixel_x, pixel_y, kPickRegionRadius);

    if(pickIds.empty()) {
        selectMgr.clearHover();
        return;
    }

    // Resolve via PickResolver
    const auto resolved = m_pickResolver.resolve(pickIds);
    if(!resolved.isValid()) {
        selectMgr.clearHover();
        return;
    }

    // Filter hover entity for Wire/Part modes
    const bool wireMode = selectMgr.isTypePickable(RenderEntityType::Wire);
    const bool partMode = selectMgr.isTypePickable(RenderEntityType::Part);

    if(wireMode && resolved.type == RenderEntityType::Face) {
        selectMgr.clearHover();
        return;
    }

    selectMgr.setHoverEntity(resolved.uid, resolved.type, resolved.partUid, resolved.wireUid);

    // For wire mode: pass complete set of edge UIDs for wire loop highlighting
    if(resolved.wireUid != 0) {
        const auto& wireEdges = m_pickResolver.wireEdges(resolved.wireUid);
        selectMgr.setHoveredWireEdges(wireEdges);
    } else {
        selectMgr.setHoveredWireEdges({});
    }
}

// =============================================================================
// Picking (click selection)
// =============================================================================

void RenderSceneImpl::processPicking(const PickingInput& input) {
    if(!m_initialized || !m_selectionPassInitialized) {
        return;
    }
    if(input.m_action == PickAction::None) {
        return;
    }

    auto& selectMgr = RenderSelectManager::instance();

    // Convert cursor position to framebuffer pixel coordinates (flip Y for OpenGL)
    const int px = static_cast<int>(input.m_cursorPos.x() * input.m_devicePixelRatio);
    const int py = static_cast<int>((input.m_itemSize.height() - input.m_cursorPos.y()) *
                                    input.m_devicePixelRatio);

    const RenderEntityTypeMask pickMask = selectMgr.getPickTypes();

    // Expand mask for Wire/Part mode
    RenderEntityTypeMask effectiveMask = pickMask;
    if(selectMgr.isTypePickable(RenderEntityType::Wire)) {
        effectiveMask = effectiveMask | RenderEntityTypeMask::Edge | RenderEntityTypeMask::Face;
    }
    if(selectMgr.isTypePickable(RenderEntityType::Part)) {
        effectiveMask = effectiveMask | RenderEntityTypeMask::Face | RenderEntityTypeMask::Edge;
    }

    RenderPassContext pick_ctx;
    pick_ctx.m_params.viewMatrix = m_frameState.viewMatrix;
    pick_ctx.m_params.projMatrix = m_frameState.projMatrix;
    pick_ctx.m_geometry.m_buffer = &m_geometryBuffer;
    pick_ctx.m_geometry.m_triangleRanges = &m_geometryTriangleRanges;
    pick_ctx.m_geometry.m_lineRanges = &m_geometryLineRanges;
    pick_ctx.m_geometry.m_pointRanges = &m_geometryPointRanges;
    pick_ctx.m_mesh.m_buffer = &m_meshBuffer;
    pick_ctx.m_mesh.m_triangleRanges = &m_meshTriangleRanges;
    pick_ctx.m_mesh.m_lineRanges = &m_meshLineRanges;
    pick_ctx.m_mesh.m_pointRanges = &m_meshPointRanges;

    // Render to pick FBO
    m_selectionPass.renderToFbo(pick_ctx, effectiveMask);

    // Restore main framebuffer viewport
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

    // Read a small region for priority-based picking
    const auto pickIds = m_selectionPass.readPickRegion(px, py, kPickRegionRadius);

    if(pickIds.empty()) {
        if(input.m_action == PickAction::Add) {
            selectMgr.clearSelection();
        }
        return;
    }

    // Resolve via PickResolver
    auto resolved = m_pickResolver.resolve(pickIds);
    if(!resolved.isValid()) {
        if(input.m_action == PickAction::Add) {
            selectMgr.clearSelection();
        }
        return;
    }

    // Wire mode reverse lookup
    if(selectMgr.isTypePickable(RenderEntityType::Wire)) {
        if(resolved.type == RenderEntityType::Edge) {
            if(resolved.wireUid != 0) {
                resolved.uid = resolved.wireUid;
                resolved.type = RenderEntityType::Wire;
            }
        } else if(resolved.type == RenderEntityType::Face) {
            return;
        }
    }

    // Part mode reverse lookup
    if(selectMgr.isTypePickable(RenderEntityType::Part) &&
       resolved.type != RenderEntityType::Part) {
        if(resolved.partUid != 0) {
            resolved.uid = resolved.partUid;
            resolved.type = RenderEntityType::Part;
        }
    }

    if(input.m_action == PickAction::Add) {
        selectMgr.addSelection(resolved.uid, resolved.type);
        if(resolved.type == RenderEntityType::Wire) {
            const auto& wireEdges = m_pickResolver.wireEdges(resolved.uid);
            selectMgr.addSelectedWireEdges(resolved.uid, wireEdges);
        }
    } else if(input.m_action == PickAction::Remove) {
        selectMgr.removeSelection(resolved.uid, resolved.type);
        if(resolved.type == RenderEntityType::Wire) {
            selectMgr.removeSelectedWireEdges(resolved.uid);
        }
    }

    LOG_DEBUG("RenderSceneImpl: Pick result type={}, uid={}", static_cast<int>(resolved.type),
              resolved.uid);
}

// =============================================================================
// Cleanup
// =============================================================================

void RenderSceneImpl::cleanup() {
    m_geometryBuffer.cleanup();
    m_meshBuffer.cleanup();
    cleanupPasses(m_opaquePass, m_transparentPass, m_wireframePass, m_highlightPass,
                  m_postProcessPass, m_uiPass);
    m_selectionPass.cleanup();
    m_pickResolver.clear();
    m_geometryTriangleRanges.clear();
    m_geometryLineRanges.clear();
    m_geometryPointRanges.clear();
    m_meshTriangleRanges.clear();
    m_meshLineRanges.clear();
    m_meshPointRanges.clear();
    m_frameState = {};
    m_initialized = false;
    m_selectionPassInitialized = false;
    LOG_DEBUG("RenderSceneImpl: Cleaned up");
}

} // namespace OpenGeoLab::Render
