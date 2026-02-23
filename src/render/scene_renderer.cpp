/**
 * @file scene_renderer.cpp
 * @brief SceneRendererImpl - concrete implementation of ISceneRenderer
 *
 * Contains the full rendering pipeline implementation including:
 * - GeometryPass for main scene rendering (faces, edges, vertices)
 * - MeshPass for FEM mesh element and node rendering
 * - PickingPass for entity picking (with pixel-level hit detection)
 * - HighlightPass for selection/hover highlighting
 *
 * All internal details are hidden behind the ISceneRenderer interface.
 */

#include "render/scene_renderer.hpp"
#include "render/highlight/outline_highlight.hpp"
#include "render/passes/geometry_pass.hpp"
#include "render/passes/highlight_pass.hpp"
#include "render/passes/mesh_pass.hpp"
#include "render/passes/picking_pass.hpp"
#include "render/render_scene_controller.hpp"
#include "render/render_types.hpp"
#include "render/renderer_core.hpp"
#include "render/select_manager.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace OpenGeoLab::Render {

// =============================================================================
// Picking helpers
// =============================================================================
namespace {

[[nodiscard]] int pickPriority(RenderEntityType type) {
    switch(type) {
    case RenderEntityType::Vertex:
        return 0;
    case RenderEntityType::MeshNode:
        return 1;
    case RenderEntityType::Edge:
        return 2;
    case RenderEntityType::MeshElement:
        return 3;
    case RenderEntityType::Face:
        return 4;
    case RenderEntityType::Solid:
        return 5;
    case RenderEntityType::Part:
        return 6;
    default:
        return 99;
    }
}

struct PickHit {
    RenderEntityType m_type{RenderEntityType::None};
    uint64_t m_uid56{0};
    int m_priority{99};
    int m_dist2{std::numeric_limits<int>::max()};
};

[[nodiscard]] bool isBetterHit(const PickHit& a, const PickHit& b) {
    if(a.m_priority != b.m_priority) {
        return a.m_priority < b.m_priority;
    }
    return a.m_dist2 < b.m_dist2;
}

[[nodiscard]] QPoint cursorToFboPixel(const QPointF& cursor_pos,
                                      const QSize& viewport_size,
                                      const QSizeF& item_size,
                                      qreal device_pixel_ratio) {
    const bool has_item_size = (item_size.width() > 0.0) && (item_size.height() > 0.0);
    if(has_item_size) {
        const double fx = cursor_pos.x() / item_size.width();
        const double fy = cursor_pos.y() / item_size.height();
        return QPoint(
            static_cast<int>(std::lround(fx * static_cast<double>(viewport_size.width()))),
            static_cast<int>(std::lround(fy * static_cast<double>(viewport_size.height()))));
    }

    const qreal dpr = (device_pixel_ratio > 0.0) ? device_pixel_ratio : 1.0;
    return QPoint(static_cast<int>(std::lround(cursor_pos.x() * dpr)),
                  static_cast<int>(std::lround(cursor_pos.y() * dpr)));
}

[[nodiscard]] QPoint clampToViewport(QPoint p, const QSize& viewport_size) {
    if(viewport_size.width() <= 0 || viewport_size.height() <= 0) {
        return QPoint(0, 0);
    }
    p.setX(std::clamp(p.x(), 0, viewport_size.width() - 1));
    p.setY(std::clamp(p.y(), 0, viewport_size.height() - 1));
    return p;
}

struct PickRegion {
    int m_px{0};
    int m_py{0};
    int m_x0{0};
    int m_y0Gl{0};
    int m_readW{0};
    int m_readH{0};
};

[[nodiscard]] PickRegion computePickRegion(const QSize& viewport_size,
                                           const QSizeF& item_size,
                                           const QPointF& cursor_pos,
                                           qreal device_pixel_ratio) {
    PickRegion region;
    const QPoint pxy = clampToViewport(
        cursorToFboPixel(cursor_pos, viewport_size, item_size, device_pixel_ratio), viewport_size);
    region.m_px = pxy.x();
    region.m_py = pxy.y();

    const int gl_y = viewport_size.height() - 1 - region.m_py;

    constexpr int pick_radius = 8;
    const int x1 = std::min(viewport_size.width() - 1, region.m_px + pick_radius);
    const int y1_gl = std::min(viewport_size.height() - 1, gl_y + pick_radius);
    region.m_x0 = std::max(0, region.m_px - pick_radius);
    region.m_y0Gl = std::max(0, gl_y - pick_radius);
    region.m_readW = x1 - region.m_x0 + 1;
    region.m_readH = y1_gl - region.m_y0Gl + 1;
    return region;
}

[[nodiscard]] PickHit findBestPickHit(const QSize& viewport_size,
                                      const PickRegion& region,
                                      const std::vector<uint64_t>& pixels,
                                      SelectManager& select_manager,
                                      SelectManager::PickTypes pick_types) {
    PickHit best;

    const bool wants_part = pick_types == SelectManager::PickTypes::Part;
    const bool wants_solid = pick_types == SelectManager::PickTypes::Solid;
    const bool wants_wire = pick_types == SelectManager::PickTypes::Wire;
    const bool wants_owner_mapping = wants_part || wants_solid || wants_wire;

    for(int iy = 0; iy < region.m_readH; ++iy) {
        const int sample_gl_y = region.m_y0Gl + iy;
        const int sample_top_y = viewport_size.height() - 1 - sample_gl_y;
        const int dy = sample_top_y - region.m_py;

        for(int ix = 0; ix < region.m_readW; ++ix) {
            const int sample_x = region.m_x0 + ix;
            const int dx = sample_x - region.m_px;
            const int dist2 = dx * dx + dy * dy;

            const size_t off = static_cast<size_t>(iy * region.m_readW + ix);
            const uint64_t packed_value = pixels[off];

            // Decode pick pixel via RenderUID: bits[63..8]=uid56, bits[7..0]=type
            const RenderUID render_uid(packed_value);
            const uint64_t uid56 = render_uid.uid56();
            const auto type = render_uid.type();
            if(type == RenderEntityType::None || uid56 == 0) {
                continue;
            }

            if(!wants_owner_mapping) {
                if(!select_manager.isTypePickable(type)) {
                    continue;
                }
            } else {
                if((wants_part || wants_solid) && type != RenderEntityType::Face) {
                    continue;
                }
                if(wants_wire && (type != RenderEntityType::Edge) &&
                   (type != RenderEntityType::Face)) {
                    continue;
                }
            }

            PickHit candidate;
            candidate.m_type = type;
            candidate.m_uid56 = uid56;
            candidate.m_priority = pickPriority(type);
            candidate.m_dist2 = dist2;

            if(isBetterHit(candidate, best)) {
                best = candidate;
            }
        }
    }

    return best;
}

void applyPendingPickAction(SelectManager& select_manager,
                            PickAction pending_pick_action,
                            uint64_t uid56,
                            RenderEntityType type) {
    if(pending_pick_action == PickAction::None || type == RenderEntityType::None || uid56 == 0) {
        return;
    }

    if(pending_pick_action == PickAction::Add) {
        select_manager.addSelection(uid56, type);
        return;
    }

    if(pending_pick_action == PickAction::Remove) {
        if(select_manager.containsSelection(uid56, type)) {
            select_manager.removeSelection(uid56, type);
        }
    }
}

struct ResolvedEntity {
    RenderEntityType m_type{RenderEntityType::None};
    uint64_t m_uid56{0};

    [[nodiscard]] bool isValid() const { return m_type != RenderEntityType::None && m_uid56 != 0; }
};

[[nodiscard]] ResolvedEntity resolveOwnerEntity(RenderEntityType raw_type,
                                                uint64_t raw_uid56,
                                                SelectManager::PickTypes pick_types,
                                                const Geometry::GeometryDocumentPtr& document) {
    if(!document || raw_type == RenderEntityType::None || raw_uid56 == 0) {
        return {raw_type, raw_uid56};
    }

    // Mesh types don't have owner mapping
    if(isMeshType(raw_type)) {
        return {raw_type, raw_uid56};
    }

    const bool wants_part = pick_types == SelectManager::PickTypes::Part;
    const bool wants_solid = pick_types == SelectManager::PickTypes::Solid;
    const bool wants_wire = pick_types == SelectManager::PickTypes::Wire;

    // Convert to geometry-layer types for document API calls
    const auto geo_type = static_cast<Geometry::EntityType>(toGeometryEntityTypeValue(raw_type));
    const auto geo_uid = static_cast<Geometry::EntityUID>(raw_uid56);
    const Geometry::EntityRef geo_ref{geo_uid, geo_type};

    // Wire mode: map Edge -> owning Wire
    if(wants_wire && raw_type == RenderEntityType::Edge) {
        const auto owning_wires =
            document->findRelatedEntities(geo_ref, Geometry::EntityType::Wire);
        if(!owning_wires.empty() && owning_wires.front().m_uid != Geometry::INVALID_ENTITY_UID) {
            return {RenderEntityType::Wire,
                    static_cast<uint64_t>(owning_wires.front().m_uid & 0x00FFFFFFFFFFFFFFu)};
        }
        return {};
    }

    // Part/Solid/Wire mode: map Face -> owning Part/Solid/Wire
    if((wants_part || wants_solid || wants_wire) && raw_type == RenderEntityType::Face) {
        Geometry::EntityType target_type = Geometry::EntityType::None;
        RenderEntityType result_render_type = RenderEntityType::None;

        if(wants_part) {
            target_type = Geometry::EntityType::Part;
            result_render_type = RenderEntityType::Part;
        } else if(wants_solid) {
            target_type = Geometry::EntityType::Solid;
            result_render_type = RenderEntityType::Solid;
        } else if(wants_wire) {
            target_type = Geometry::EntityType::Wire;
            result_render_type = RenderEntityType::Wire;
        }

        const auto owning = document->findRelatedEntities(geo_ref, target_type);
        if(!owning.empty() && owning.front().m_uid != Geometry::INVALID_ENTITY_UID) {
            return {result_render_type,
                    static_cast<uint64_t>(owning.front().m_uid & 0x00FFFFFFFFFFFFFFu)};
        }

        if(wants_wire) {
            return {};
        }
    }

    return {raw_type, raw_uid56};
}

} // namespace

// =============================================================================
// SceneRendererImpl
// =============================================================================

class SceneRendererImpl : public ISceneRenderer {
public:
    SceneRendererImpl();
    ~SceneRendererImpl() override;

    void initialize() override;
    [[nodiscard]] bool isInitialized() const override;
    void setViewportSize(const QSize& size) override;
    void uploadMeshData(const DocumentRenderData& render_data) override;
    void processPicking(const PickingInput& input) override;
    void render(const QVector3D& camera_pos,
                const QMatrix4x4& view_matrix,
                const QMatrix4x4& projection_matrix) override;
    void cleanup() override;

private:
    void setHighlightedEntity(uint64_t uid56, RenderEntityType type);

    bool renderPickPass(const PickingInput& input,
                        const PickRegion& region,
                        std::vector<uint64_t>& out_pixels);

private:
    RendererCore m_core;

    GeometryPass* m_geometryPass{nullptr};
    MeshPass* m_meshPass{nullptr};
    HighlightPass* m_highlightPass{nullptr};

    std::unique_ptr<PickingPass> m_pickingPass;

    RenderEntityType m_lastHoverType{RenderEntityType::None};
    uint64_t m_lastHoverUid56{0};
};

SceneRendererImpl::SceneRendererImpl() { LOG_TRACE("SceneRendererImpl created"); }

SceneRendererImpl::~SceneRendererImpl() {
    cleanup();
    LOG_TRACE("SceneRendererImpl destroyed");
}

void SceneRendererImpl::initialize() {
    if(m_core.isInitialized()) {
        return;
    }

    auto geometry_pass = std::make_unique<GeometryPass>();
    m_geometryPass = geometry_pass.get();
    m_core.registerPass(std::move(geometry_pass));

    auto mesh_pass = std::make_unique<MeshPass>();
    m_meshPass = mesh_pass.get();
    m_core.registerPass(std::move(mesh_pass));

    auto highlight_pass = std::make_unique<HighlightPass>();
    highlight_pass->setStrategy(std::make_unique<OutlineHighlight>());
    m_highlightPass = highlight_pass.get();
    m_core.registerPass(std::move(highlight_pass));

    m_core.initialize();

    m_pickingPass = std::make_unique<PickingPass>();
    QOpenGLFunctions gl_funcs;
    gl_funcs.initializeOpenGLFunctions();
    m_pickingPass->initialize(gl_funcs);

    LOG_DEBUG("SceneRendererImpl: Initialized with RendererCore pipeline");
}

bool SceneRendererImpl::isInitialized() const { return m_core.isInitialized(); }

void SceneRendererImpl::setViewportSize(const QSize& size) {
    m_core.setViewportSize(size);
    if(m_pickingPass && m_core.isInitialized()) {
        QOpenGLFunctions gl_funcs;
        gl_funcs.initializeOpenGLFunctions();
        m_pickingPass->resize(gl_funcs, size);
    }
}

void SceneRendererImpl::uploadMeshData(const DocumentRenderData& render_data) {
    m_core.uploadMeshData(render_data);
}

bool SceneRendererImpl::renderPickPass(const PickingInput& input,
                                       const PickRegion& region,
                                       std::vector<uint64_t>& out_pixels) {
    auto* gl_ctx = QOpenGLContext::currentContext();
    auto* f = gl_ctx ? gl_ctx->functions() : nullptr;
    if(!f || !m_pickingPass) {
        return false;
    }

    RenderPassContext pass_ctx;
    pass_ctx.m_core = &m_core;
    pass_ctx.m_viewportSize = m_core.viewportSize();
    pass_ctx.m_aspectRatio =
        pass_ctx.m_viewportSize.width() / static_cast<float>(pass_ctx.m_viewportSize.height());
    pass_ctx.m_matrices.m_view = input.m_viewMatrix;
    pass_ctx.m_matrices.m_projection = input.m_projectionMatrix;
    pass_ctx.m_matrices.m_mvp = input.m_projectionMatrix * input.m_viewMatrix;
    pass_ctx.m_cameraPos = QVector3D(0, 0, 0);

    m_pickingPass->execute(*f, pass_ctx);

    m_pickingPass->readRegion(*f, region.m_x0, region.m_y0Gl, region.m_readW, region.m_readH,
                              out_pixels);

    return true;
}

void SceneRendererImpl::processPicking(const PickingInput& input) {
    auto& select_manager = SelectManager::instance();
    if(!select_manager.isPickEnabled()) {
        return;
    }

    if(!m_core.isInitialized() || !m_pickingPass) {
        return;
    }

    const auto& viewport_size = m_core.viewportSize();
    if(viewport_size.width() <= 0 || viewport_size.height() <= 0) {
        return;
    }

    const PickRegion region = computePickRegion(viewport_size, input.m_itemSize, input.m_cursorPos,
                                                input.m_devicePixelRatio);

    std::vector<uint64_t> pixels;
    if(!renderPickPass(input, region, pixels)) {
        return;
    }

    const auto pick_types = select_manager.pickTypes();
    const PickHit best = findBestPickHit(viewport_size, region, pixels, select_manager, pick_types);

    const auto document = RenderSceneController::instance().currentDocument();
    const auto resolved = resolveOwnerEntity(best.m_type, best.m_uid56, pick_types, document);

    if(resolved.m_type != m_lastHoverType || resolved.m_uid56 != m_lastHoverUid56) {
        m_lastHoverType = resolved.m_type;
        m_lastHoverUid56 = resolved.m_uid56;
        setHighlightedEntity(resolved.m_uid56, resolved.m_type);
    }

    applyPendingPickAction(select_manager, input.m_action, resolved.m_uid56, resolved.m_type);
}

void SceneRendererImpl::setHighlightedEntity(uint64_t uid56, RenderEntityType type) {
    if(m_geometryPass) {
        m_geometryPass->setHighlightedEntity(uid56, type);
    }
    if(m_meshPass) {
        m_meshPass->setHighlightedEntity(uid56, type);
    }
}

void SceneRendererImpl::render(const QVector3D& camera_pos,
                               const QMatrix4x4& view_matrix,
                               const QMatrix4x4& projection_matrix) {
    m_core.render(camera_pos, view_matrix, projection_matrix);
}

void SceneRendererImpl::cleanup() {
    if(m_pickingPass) {
        QOpenGLFunctions gl_funcs;
        gl_funcs.initializeOpenGLFunctions();
        m_pickingPass->cleanup(gl_funcs);
        m_pickingPass.reset();
    }
    m_core.cleanup();
    m_geometryPass = nullptr;
    m_meshPass = nullptr;
    m_highlightPass = nullptr;
    LOG_DEBUG("SceneRendererImpl: Cleanup complete");
}

// =============================================================================
// SceneRendererFactory implementation
// =============================================================================

class SceneRendererFactoryImpl : public SceneRendererFactory {
public:
    SceneRendererFactoryImpl() = default;
    ~SceneRendererFactoryImpl() = default;

    tObjectPtr create() override { return std::make_unique<SceneRendererImpl>(); }
};

void registerSceneRendererFactory() {
    g_ComponentFactory.registFactoryWithID<SceneRendererFactoryImpl>("SceneRenderer");
}

} // namespace OpenGeoLab::Render
