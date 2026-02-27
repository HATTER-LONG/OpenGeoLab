/**
 * @file render_sceneImpl.cpp
 * @brief RenderSceneImpl — orchestrates per-frame rendering, GPU picking, and
 *        hover detection by composing GeometryPass, MeshPass, and PickPass.
 */

#include "render_sceneImpl.hpp"

#include "render/render_data.hpp"
#include "render/render_scene_controller.hpp"
#include "render/render_select_manager.hpp"
#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <algorithm>

namespace OpenGeoLab::Render {

namespace {

/// Priority order for pick type resolution (lower index = higher priority)
constexpr RenderEntityType PICK_PRIORITY[] = {
    RenderEntityType::Vertex,       RenderEntityType::MeshNode,   RenderEntityType::Edge,
    RenderEntityType::MeshLine,     RenderEntityType::Face,       RenderEntityType::Shell,
    RenderEntityType::Wire,         RenderEntityType::Solid,      RenderEntityType::Part,
    RenderEntityType::MeshTriangle, RenderEntityType::MeshQuad4,  RenderEntityType::MeshTetra4,
    RenderEntityType::MeshHexa8,    RenderEntityType::MeshPrism6, RenderEntityType::MeshPyramid5,
};

constexpr int PICK_PRIORITY_COUNT = sizeof(PICK_PRIORITY) / sizeof(PICK_PRIORITY[0]);

/// Returns priority index for a type (lower = higher priority). Returns INT_MAX if not found.
int typePriority(RenderEntityType type) {
    for(int i = 0; i < PICK_PRIORITY_COUNT; ++i) {
        if(PICK_PRIORITY[i] == type) {
            return i;
        }
    }
    return INT_MAX;
}

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
// Entity lookup tables
// =============================================================================

void RenderSceneImpl::rebuildEntityLookups() {
    m_entityToPartUid.clear();

    auto addRanges = [this](const std::vector<DrawRangeEx>& ranges) {
        for(const auto& rangeEx : ranges) {
            if(rangeEx.m_partUid != 0 && rangeEx.m_entityKey.m_type != RenderEntityType::Part) {
                m_entityToPartUid[rangeEx.m_entityKey.m_uid] = rangeEx.m_partUid;
            }
        }
    };

    addRanges(m_geometryPass.triangleRanges());
    addRanges(m_geometryPass.lineRanges());
    addRanges(m_geometryPass.pointRanges());

    // Copy authoritative mappings from RenderData (built by GeometryRenderBuilder)
    const auto& renderData = RenderSceneController::instance().renderData();
    m_edgeToWireUids = renderData.m_edgeToWireUids;
    m_wireToEdgeUids = renderData.m_wireToEdgeUids;
    m_wireToFaceUid = renderData.m_wireToFaceUid;
}

uint64_t RenderSceneImpl::resolvePartUid(uint64_t entity_uid, RenderEntityType type) const {
    if(type == RenderEntityType::Part) {
        return entity_uid;
    }
    auto it = m_entityToPartUid.find(entity_uid);
    return it != m_entityToPartUid.end() ? it->second : 0;
}

uint64_t RenderSceneImpl::resolveWireUid(uint64_t edge_uid, uint64_t face_uid) const {
    auto it = m_edgeToWireUids.find(edge_uid);
    if(it == m_edgeToWireUids.end() || it->second.empty()) {
        return 0;
    }

    // If there's only one wire, return it directly
    if(it->second.size() == 1) {
        return it->second[0];
    }

    // Multiple wires share this edge — prefer the wire belonging to the given face
    if(face_uid != 0) {
        for(const auto wire_uid : it->second) {
            auto fit = m_wireToFaceUid.find(wire_uid);
            if(fit != m_wireToFaceUid.end() && fit->second == face_uid) {
                return wire_uid;
            }
        }
    }

    // Fallback: return the first wire
    return it->second[0];
}

const std::vector<uint64_t>& RenderSceneImpl::resolveWireEdges(uint64_t wire_uid) const {
    static const std::vector<uint64_t> empty;
    auto it = m_wireToEdgeUids.find(wire_uid);
    return it != m_wireToEdgeUids.end() ? it->second : empty;
}

// =============================================================================
// Rendering
// =============================================================================

void RenderSceneImpl::render(const QVector3D& camera_pos,
                             const QMatrix4x4& view_matrix,
                             const QMatrix4x4& projection_matrix) {
    if(!m_initialized) {
        return;
    }

    const auto& renderData = RenderSceneController::instance().renderData();

    // Update GPU buffers from current render data
    m_geometryPass.updateBuffers(renderData);
    m_meshPass.updateBuffers(renderData);

    // Sync mesh display mode from controller each frame
    m_meshPass.setDisplayMode(RenderSceneController::instance().meshDisplayMode());

    // Rebuild entity lookup tables when geometry changes
    if(renderData.m_geometryDirty) {
        rebuildEntityLookups();
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

    // Execute render passes
    const bool xRay = RenderSceneController::instance().isXRayMode();
    m_geometryPass.render(view_matrix, projection_matrix, camera_pos, xRay);
    m_meshPass.render(view_matrix, projection_matrix, camera_pos, xRay);
}

// =============================================================================
// Hover processing
// =============================================================================

void RenderSceneImpl::processHover(int pixel_x,
                                   int pixel_y,
                                   const QMatrix4x4& view,
                                   const QMatrix4x4& projection) {
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

    // Render to pick FBO with selective type rendering.
    // Pass separate vertex counts so pick pass can draw each mesh topology independently.
    m_pickPass.renderToFbo(view, projection, m_geometryPass.gpuBuffer(),
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

    // Find the highest-priority entity and also the best face in the region
    // (face context is used to disambiguate shared edges for Wire selection)
    uint64_t bestEncoded = 0;
    int bestPriority = INT_MAX;
    uint64_t bestFaceEncoded = 0;

    for(const auto encoded : pickIds) {
        const RenderEntityType type = PickId::decodeType(encoded);
        const int priority = typePriority(type);
        if(priority < bestPriority) {
            bestPriority = priority;
            bestEncoded = encoded;
        }
        // Track the first Face as context for wire resolution
        if(type == RenderEntityType::Face && bestFaceEncoded == 0) {
            bestFaceEncoded = encoded;
        }
    }

    if(bestEncoded == 0) {
        selectMgr.clearHover();
        return;
    }

    const RenderEntityType hoverType = PickId::decodeType(bestEncoded);
    const uint64_t hoverUid = PickId::decodeUID(bestEncoded);

    // Filter hover entity: expanded types (Face, Edge) rendered for Wire/Part
    // disambiguation should not trigger direct face highlighting in those modes.
    const bool wireMode = selectMgr.isTypePickable(RenderEntityType::Wire);
    const bool partMode = selectMgr.isTypePickable(RenderEntityType::Part);

    if(wireMode && hoverType == RenderEntityType::Face) {
        // In Wire mode, hovering a face region (between edges) should not
        // highlight the face. Clear hover and return.
        selectMgr.clearHover();
        return;
    }
    if(partMode && hoverType == RenderEntityType::Face) {
        // In Part mode, face hover should resolve to the parent Part.
        // The Part-level hover is handled via partUid below.
    }

    // Resolve parent part uid for part-mode highlighting
    const uint64_t partUid = resolvePartUid(hoverUid, hoverType);

    // Resolve parent wire uid for wire-mode highlighting.
    // Use face context from the pick region to prefer the wire on the face
    // under the cursor (handles shared edges between two faces correctly).
    uint64_t wireUid = 0;
    if(hoverType == RenderEntityType::Edge) {
        const uint64_t faceContext = bestFaceEncoded != 0 ? PickId::decodeUID(bestFaceEncoded) : 0;
        wireUid = resolveWireUid(hoverUid, faceContext);
    }

    selectMgr.setHoverEntity(hoverUid, hoverType, partUid, wireUid);

    // For wire mode: pass the complete set of edge UIDs so the render pass
    // can highlight the entire wire loop, including shared edges.
    if(wireUid != 0) {
        const auto& wireEdges = resolveWireEdges(wireUid);
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

    // Render to pick FBO with selective type rendering
    m_pickPass.renderToFbo(input.m_viewMatrix, input.m_projectionMatrix, m_geometryPass.gpuBuffer(),
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

    // Find highest-priority entity and face context for wire resolution
    uint64_t bestEncoded = 0;
    int bestPriority = INT_MAX;
    uint64_t bestFaceEncoded = 0;

    for(const auto encoded : pickIds) {
        const RenderEntityType type = PickId::decodeType(encoded);
        const int priority = typePriority(type);
        if(priority < bestPriority) {
            bestPriority = priority;
            bestEncoded = encoded;
        }
        if(type == RenderEntityType::Face && bestFaceEncoded == 0) {
            bestFaceEncoded = encoded;
        }
    }

    if(bestEncoded == 0) {
        if(input.m_action == PickAction::Add) {
            selectMgr.clearSelection();
        }
        return;
    }

    RenderEntityType pickedType = PickId::decodeType(bestEncoded);
    uint64_t pickedUid = PickId::decodeUID(bestEncoded);

    // Wire mode reverse lookup: if we picked an edge but Wire is the pick type,
    // resolve to parent Wire using face context for shared-edge disambiguation.
    // If we picked a face in Wire mode, discard the pick (faces are only
    // rendered for edge/wire disambiguation, not as direct pick targets).
    if(selectMgr.isTypePickable(RenderEntityType::Wire)) {
        if(pickedType == RenderEntityType::Edge) {
            const uint64_t faceContext =
                bestFaceEncoded != 0 ? PickId::decodeUID(bestFaceEncoded) : 0;
            const uint64_t wireUid = resolveWireUid(pickedUid, faceContext);
            if(wireUid != 0) {
                pickedUid = wireUid;
                pickedType = RenderEntityType::Wire;
            }
        } else if(pickedType == RenderEntityType::Face) {
            // Face picked in Wire mode — not a valid target, ignore
            return;
        }
    }

    // Part mode reverse lookup: if we picked a sub-entity but Part is the pick type,
    // resolve to parent Part
    if(selectMgr.isTypePickable(RenderEntityType::Part) && pickedType != RenderEntityType::Part) {
        const uint64_t partUid = resolvePartUid(pickedUid, pickedType);
        if(partUid != 0) {
            pickedUid = partUid;
            pickedType = RenderEntityType::Part;
        }
    }

    if(input.m_action == PickAction::Add) {
        selectMgr.addSelection(pickedUid, pickedType);
        // Store wire edge UIDs for complete wire highlighting
        if(pickedType == RenderEntityType::Wire) {
            const auto& wireEdges = resolveWireEdges(pickedUid);
            selectMgr.addSelectedWireEdges(pickedUid, wireEdges);
        }
    } else if(input.m_action == PickAction::Remove) {
        selectMgr.removeSelection(pickedUid, pickedType);
        if(pickedType == RenderEntityType::Wire) {
            selectMgr.removeSelectedWireEdges(pickedUid);
        }
    }

    LOG_DEBUG("RenderSceneImpl: Pick result type={}, uid={}", static_cast<int>(pickedType),
              pickedUid);
}

// =============================================================================
// Cleanup
// =============================================================================

void RenderSceneImpl::cleanup() {
    m_geometryPass.cleanup();
    m_meshPass.cleanup();
    m_pickPass.cleanup();
    m_entityToPartUid.clear();
    m_edgeToWireUids.clear();
    m_wireToEdgeUids.clear();
    m_wireToFaceUid.clear();
    m_initialized = false;
    m_pickPassInitialized = false;
    LOG_DEBUG("RenderSceneImpl: Cleaned up");
}

} // namespace OpenGeoLab::Render
