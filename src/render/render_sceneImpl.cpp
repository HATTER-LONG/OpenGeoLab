/**
 * @file render_sceneImpl.cpp
 * @brief RenderSceneImpl — orchestrates per-frame rendering, GPU picking, and
 *        hover detection by composing GeometryPass, MeshPass, and PickPass.
 *
 * After synchronize(), all rendering and picking uses cached SceneFrameState —
 * no GUI-thread singletons (RenderSceneController) are accessed during
 * render(), processHover(), or processPicking().
 */

#include "render_sceneImpl.hpp"

#include "render/render_select_manager.hpp"
#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

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

    m_geometryPass.initialize();
    m_meshPass.initialize();
    // PickPass is deferred until viewport size is known

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

    if(!m_pickPassInitialized) {
        m_pickPass.initialize(size.width(), size.height());
        m_pickPassInitialized = true;
    } else {
        m_pickPass.resize(size.width(), size.height());
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

    // Update GPU buffers (passes check dirty flags internally)
    m_geometryPass.updateBuffers(renderData);
    m_meshPass.updateBuffers(renderData);

    // Sync mesh display mode
    m_meshPass.setDisplayMode(state.meshDisplayMode);

    // Rebuild pick resolver when geometry changes
    if(renderData.m_geometryDirty) {
        m_pickResolver.rebuild(m_geometryPass.triangleRanges(),
                               m_geometryPass.lineRanges(),
                               m_geometryPass.pointRanges(),
                               renderData.m_pickData);
    }
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

    // Execute render passes using cached frame state
    m_geometryPass.render(m_frameState.viewMatrix, m_frameState.projMatrix,
                          m_frameState.cameraPos, m_frameState.xRayMode);
    m_meshPass.render(m_frameState.viewMatrix, m_frameState.projMatrix,
                      m_frameState.cameraPos, m_frameState.xRayMode);
}

// =============================================================================
// Hover processing
// =============================================================================

void RenderSceneImpl::processHover(int pixel_x, int pixel_y) {
    if(!m_initialized || !m_pickPassInitialized) {
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

    // For Wire/Part mode, we need to render edges/faces to the pick FBO
    // so we can resolve them to their parent Wire/Part
    RenderEntityTypeMask effectiveMask = pickMask;
    if(selectMgr.isTypePickable(RenderEntityType::Wire)) {
        // When in Wire mode, render edges and faces so we can resolve
        // shared edges to the wire belonging to the face under cursor
        effectiveMask = effectiveMask | RenderEntityTypeMask::Edge | RenderEntityTypeMask::Face;
    }
    if(selectMgr.isTypePickable(RenderEntityType::Part)) {
        // When in Part mode, render faces/edges so we can resolve to parent Part
        effectiveMask = effectiveMask | RenderEntityTypeMask::Face | RenderEntityTypeMask::Edge;
    }

    // Render to pick FBO using cached matrices
    m_pickPass.renderToFbo(m_frameState.viewMatrix, m_frameState.projMatrix,
                           m_geometryPass.gpuBuffer(),
                           m_geometryPass.triangleRanges(), m_geometryPass.lineRanges(),
                           m_geometryPass.pointRanges(), m_meshPass.gpuBuffer(),
                           m_meshPass.surfaceVertexCount(), m_meshPass.wireframeVertexCount(),
                           m_meshPass.nodeVertexCount(), effectiveMask);

    // Restore the main framebuffer viewport
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

    // Read a 7x7 region around cursor for priority-based picking
    const auto pickIds = m_pickPass.readPickRegion(pixel_x, pixel_y, 3);

    if(pickIds.empty()) {
        selectMgr.clearHover();
        return;
    }

    // Resolve via PickResolver (priority selection + hierarchy lookup)
    const auto resolved = m_pickResolver.resolve(pickIds);
    if(!resolved.isValid()) {
        selectMgr.clearHover();
        return;
    }

    // Filter hover entity: expanded types (Face, Edge) rendered for Wire/Part
    // disambiguation should not trigger direct face highlighting in those modes.
    const bool wireMode = selectMgr.isTypePickable(RenderEntityType::Wire);
    const bool partMode = selectMgr.isTypePickable(RenderEntityType::Part);

    if(wireMode && resolved.type == RenderEntityType::Face) {
        // In Wire mode, hovering a face region (between edges) should not
        // highlight the face. Clear hover and return.
        selectMgr.clearHover();
        return;
    }
    if(partMode && resolved.type == RenderEntityType::Face) {
        // In Part mode, face hover should resolve to the parent Part.
        // The Part-level hover is handled via partUid below.
    }

    selectMgr.setHoverEntity(resolved.uid, resolved.type, resolved.partUid, resolved.wireUid);

    // For wire mode: pass the complete set of edge UIDs so the render pass
    // can highlight the entire wire loop, including shared edges.
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
    if(!m_initialized || !m_pickPassInitialized) {
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

    // For Wire/Part mode, expand the mask to include sub-entities for resolution
    RenderEntityTypeMask effectiveMask = pickMask;
    if(selectMgr.isTypePickable(RenderEntityType::Wire)) {
        // Wire mode: render edges and faces for shared-edge disambiguation
        effectiveMask = effectiveMask | RenderEntityTypeMask::Edge | RenderEntityTypeMask::Face;
    }
    if(selectMgr.isTypePickable(RenderEntityType::Part)) {
        effectiveMask = effectiveMask | RenderEntityTypeMask::Face | RenderEntityTypeMask::Edge;
    }

    // Render to pick FBO using cached matrices
    m_pickPass.renderToFbo(m_frameState.viewMatrix, m_frameState.projMatrix,
                           m_geometryPass.gpuBuffer(),
                           m_geometryPass.triangleRanges(), m_geometryPass.lineRanges(),
                           m_geometryPass.pointRanges(), m_meshPass.gpuBuffer(),
                           m_meshPass.surfaceVertexCount(), m_meshPass.wireframeVertexCount(),
                           m_meshPass.nodeVertexCount(), effectiveMask);

    // Restore the main framebuffer viewport
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

    // Read a small region for priority-based picking
    const auto pickIds = m_pickPass.readPickRegion(px, py, 3);

    if(pickIds.empty()) {
        if(input.m_action == PickAction::Add) {
            selectMgr.clearSelection();
        }
        return;
    }

    // Resolve via PickResolver (priority selection + hierarchy lookup)
    auto resolved = m_pickResolver.resolve(pickIds);
    if(!resolved.isValid()) {
        if(input.m_action == PickAction::Add) {
            selectMgr.clearSelection();
        }
        return;
    }

    // Wire mode reverse lookup: if we picked an edge but Wire is the pick type,
    // resolve to parent Wire using face context for shared-edge disambiguation.
    // If we picked a face in Wire mode, discard the pick (faces are only
    // rendered for edge/wire disambiguation, not as direct pick targets).
    if(selectMgr.isTypePickable(RenderEntityType::Wire)) {
        if(resolved.type == RenderEntityType::Edge) {
            if(resolved.wireUid != 0) {
                resolved.uid = resolved.wireUid;
                resolved.type = RenderEntityType::Wire;
            }
        } else if(resolved.type == RenderEntityType::Face) {
            // Face picked in Wire mode — not a valid target, ignore
            return;
        }
    }

    // Part mode reverse lookup: if we picked a sub-entity but Part is the pick type,
    // resolve to parent Part
    if(selectMgr.isTypePickable(RenderEntityType::Part) &&
       resolved.type != RenderEntityType::Part) {
        if(resolved.partUid != 0) {
            resolved.uid = resolved.partUid;
            resolved.type = RenderEntityType::Part;
        }
    }

    if(input.m_action == PickAction::Add) {
        selectMgr.addSelection(resolved.uid, resolved.type);
        // Store wire edge UIDs for complete wire highlighting
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
    m_geometryPass.cleanup();
    m_meshPass.cleanup();
    m_pickPass.cleanup();
    m_pickResolver.clear();
    m_frameState = {};
    m_initialized = false;
    m_pickPassInitialized = false;
    LOG_DEBUG("RenderSceneImpl: Cleaned up");
}

} // namespace OpenGeoLab::Render
