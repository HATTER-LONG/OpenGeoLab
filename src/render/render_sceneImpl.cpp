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

void RenderSceneImpl::synchronizeGeometry(const GeometryRenderDomain& geometry) {
    if(!m_geometryBuffer.upload(geometry.m_passData)) {
        LOG_ERROR("RenderSceneImpl: Failed to upload geometry GPU buffer");
    }
    m_geometryDataVersion = geometry.m_version;
}

void RenderSceneImpl::synchronizeMesh(const MeshRenderDomain& mesh) {
    if(!m_meshBuffer.upload(mesh.m_passData)) {
        LOG_ERROR("RenderSceneImpl: Failed to upload mesh GPU buffer");
    }
    m_meshDataVersion = mesh.m_version;
}

RenderPassContext RenderSceneImpl::buildRenderPassContext(const RenderData& render_data) {
    return {{m_frameState.m_viewMatrix, m_frameState.m_projMatrix, m_frameState.m_cameraPos,
             m_frameState.m_xRayMode},
            {m_geometryBuffer, render_data.m_geometry.m_triangleRanges,
             render_data.m_geometry.m_lineRanges, render_data.m_geometry.m_pointRanges,
             render_data.m_geometry.m_batches},
            {m_meshBuffer, render_data.m_mesh.m_triangleRanges, render_data.m_mesh.m_lineRanges,
             render_data.m_mesh.m_pointRanges, render_data.m_mesh.m_batches,
             m_frameState.m_meshDisplayMode}};
}

RenderPassContext RenderSceneImpl::buildPickPassContext(const RenderData& render_data,
                                                        RenderEntityTypeMask pick_mask) {
    return {{m_frameState.m_viewMatrix, m_frameState.m_projMatrix, m_frameState.m_cameraPos,
             m_frameState.m_xRayMode, pick_mask},
            {m_geometryBuffer, render_data.m_geometry.m_triangleRanges,
             render_data.m_geometry.m_lineRanges, render_data.m_geometry.m_pointRanges,
             render_data.m_geometry.m_batches},
            {m_meshBuffer, render_data.m_mesh.m_triangleRanges, render_data.m_mesh.m_lineRanges,
             render_data.m_mesh.m_pointRanges, render_data.m_mesh.m_batches,
             m_frameState.m_meshDisplayMode}};
}

void RenderSceneImpl::initialize() {
    if(m_initialized) {
        return;
    }

    m_geometryBuffer.initialize();
    m_meshBuffer.initialize();
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
    if(render_data.m_geometry.m_version != m_geometryDataVersion ||
       render_data.m_mesh.m_version != m_meshDataVersion) {
        // Update pick resolver reference data for hierarchy lookups
        m_pickResolver.setPickData(render_data.m_geometry.m_pickData,
                                   render_data.m_mesh.m_pickData);
    }
    if(render_data.m_geometry.m_version != m_geometryDataVersion) {
        synchronizeGeometry(render_data.m_geometry);
    }

    if(render_data.m_mesh.m_version != m_meshDataVersion) {
        synchronizeMesh(render_data.m_mesh);
    }
}

void RenderSceneImpl::render() {
    if(!m_initialized) {
        return;
    }

    auto* f = QOpenGLContext::currentContext()->functions();

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

    if(!m_frameState.m_renderData) {
        return;
    }

    RenderPassContext pass_context = buildRenderPassContext(*m_frameState.m_renderData);

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

    if(!m_frameState.m_renderData) {
        return;
    }

    RenderPassContext pass_context =
        buildPickPassContext(*m_frameState.m_renderData, effective_mask);
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
                              resolved.m_solidUid, resolved.m_wireUid);

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

    if(!m_frameState.m_renderData) {
        return;
    }

    RenderPassContext pass_context =
        buildPickPassContext(*m_frameState.m_renderData, effective_mask);

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
    cleanupPasses(m_opaquePass, m_wireframePass, m_highlightPass);
    m_initialized = false;
    m_selectionPass.cleanup();
    m_geometryBuffer.cleanup();
    m_meshBuffer.cleanup();
    m_pickResolver.clear();
    m_geometryDataVersion = 0;
    m_meshDataVersion = 0;
    LOG_DEBUG("RenderSceneImpl: Cleaning up render scene");
}

} // namespace OpenGeoLab::Render
