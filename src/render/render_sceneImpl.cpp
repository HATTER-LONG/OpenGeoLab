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

        // Extract mesh topology counts from mesh root's DrawRanges
        m_meshSurfaceCount = 0;
        m_meshWireframeCount = 0;
        m_meshNodeCount = 0;

        for(const auto& root : renderData.m_roots) {
            if(!isMeshDomain(root.m_key.m_type)) {
                continue;
            }
            auto it = root.m_drawRanges.find(RenderPassType::Mesh);
            if(it == root.m_drawRanges.end()) {
                continue;
            }
            for(const auto& range : it->second) {
                switch(range.m_topology) {
                case PrimitiveTopology::Triangles:
                    m_meshSurfaceCount += range.m_vertexCount;
                    break;
                case PrimitiveTopology::Lines:
                    m_meshWireframeCount += range.m_vertexCount;
                    break;
                case PrimitiveTopology::Points:
                    m_meshNodeCount += range.m_vertexCount;
                    break;
                }
            }
        }
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

    // Build common render parameters
    PassRenderParams params;
    params.viewMatrix = m_frameState.viewMatrix;
    params.projMatrix = m_frameState.projMatrix;
    params.cameraPos = m_frameState.cameraPos;
    params.xRayMode = m_frameState.xRayMode;

    // --- Pass 1: Surfaces ---
    m_opaquePass.render(params, m_geometryBuffer, m_geometryTriangleRanges, m_meshBuffer,
                        m_meshSurfaceCount, m_meshDisplayMode);
    m_transparentPass.render(params, m_geometryBuffer, m_geometryTriangleRanges, m_meshBuffer,
                             m_meshSurfaceCount, m_meshDisplayMode);

    // --- Pass 2: Wireframe ---
    m_wireframePass.render(params, m_geometryBuffer, m_geometryLineRanges, m_geometryPointRanges,
                           m_meshBuffer, m_meshSurfaceCount, m_meshWireframeCount, m_meshNodeCount,
                           m_meshDisplayMode);

    // --- Pass 3: Highlight (selected/hovered entity overdraw) ---
    m_highlightPass.renderGeometry(params, m_geometryBuffer, m_geometryTriangleRanges,
                                   m_geometryLineRanges, m_geometryPointRanges);
    m_highlightPass.renderMesh(params, m_meshBuffer, m_meshSurfaceCount, m_meshWireframeCount,
                               m_meshNodeCount, m_meshDisplayMode);

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

    // Render to pick FBO
    m_selectionPass.renderToFbo(m_frameState.viewMatrix, m_frameState.projMatrix, m_geometryBuffer,
                                m_geometryTriangleRanges, m_geometryLineRanges,
                                m_geometryPointRanges, m_meshBuffer, m_meshSurfaceCount,
                                m_meshWireframeCount, m_meshNodeCount, effectiveMask);

    // Restore main framebuffer viewport
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

    // Read 7x7 region around cursor for priority-based picking
    const auto pickIds = m_selectionPass.readPickRegion(pixel_x, pixel_y, 3);

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

    // Render to pick FBO
    m_selectionPass.renderToFbo(m_frameState.viewMatrix, m_frameState.projMatrix, m_geometryBuffer,
                                m_geometryTriangleRanges, m_geometryLineRanges,
                                m_geometryPointRanges, m_meshBuffer, m_meshSurfaceCount,
                                m_meshWireframeCount, m_meshNodeCount, effectiveMask);

    // Restore main framebuffer viewport
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

    // Read a small region for priority-based picking
    const auto pickIds = m_selectionPass.readPickRegion(px, py, 3);

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
    m_meshSurfaceCount = 0;
    m_meshWireframeCount = 0;
    m_meshNodeCount = 0;
    m_frameState = {};
    m_initialized = false;
    m_selectionPassInitialized = false;
    LOG_DEBUG("RenderSceneImpl: Cleaned up");
}

} // namespace OpenGeoLab::Render
