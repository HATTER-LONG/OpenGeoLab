/**
 * @file opengl_viewport.cpp
 * @brief Implementation of GLViewport and GLViewportRenderer
 */

#include "app/opengl_viewport.hpp"
#include "render/passes/picking_pass.hpp"
#include "render/select_manager.hpp"
#include "util/logger.hpp"

#include <QMouseEvent>
#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QWheelEvent>
#include <QtCore/QMetaObject>
#include <QtMath>

namespace OpenGeoLab::App {
namespace {
[[nodiscard]] Render::TrackballController::Mode pickMode(Qt::MouseButtons buttons,
                                                         Qt::KeyboardModifiers modifiers) {
    if((buttons & Qt::LeftButton) && (modifiers & Qt::ControlModifier)) {
        return Render::TrackballController::Mode::Orbit;
    }

    if(((buttons & Qt::LeftButton) && (modifiers & Qt::ShiftModifier)) ||
       (buttons & Qt::MiddleButton)) {
        return Render::TrackballController::Mode::Pan;
    }

    if(buttons & Qt::RightButton) {
        return Render::TrackballController::Mode::Zoom;
    }

    return Render::TrackballController::Mode::None;
}
} // namespace
// =============================================================================
// GLViewport Implementation
// =============================================================================

GLViewport::GLViewport(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemHasContents, true);
    setMirrorVertically(true);
    setAcceptHoverEvents(true);

    auto& scene_controller = Render::RenderSceneController::instance();
    m_cameraState = scene_controller.camera();
    m_hasGeometry = scene_controller.hasGeometry();
    m_trackballController.syncFromCamera(m_cameraState);

    // Bridge service events (Util::Signal) onto the Qt/GUI thread
    m_sceneNeedsUpdateConn = scene_controller.subscribeSceneNeedsUpdate([this]() {
        QMetaObject::invokeMethod(this, &GLViewport::onSceneNeedsUpdate, Qt::QueuedConnection);
    });
    m_cameraChangedConn = scene_controller.subscribeCameraChanged([this]() {
        QMetaObject::invokeMethod(this, &GLViewport::onSceneNeedsUpdate, Qt::QueuedConnection);
    });
    m_geometryChangedConn = scene_controller.subscribeGeometryChanged([this]() {
        QMetaObject::invokeMethod(this, &GLViewport::onServiceGeometryChanged,
                                  Qt::QueuedConnection);
    });

    auto& select_manager = Render::SelectManager::instance();
    m_pickSettingsChangedConn =
        select_manager.subscribePickSettingsChanged([this](Render::SelectManager::PickTypes) {
            QMetaObject::invokeMethod(this, &GLViewport::onSceneNeedsUpdate, Qt::QueuedConnection);
        });
    m_selectionChangedConn = select_manager.subscribeSelectionChanged(
        [this](Render::SelectManager::PickResult, Render::SelectManager::SelectionChangeAction) {
            QMetaObject::invokeMethod(this, &GLViewport::onSceneNeedsUpdate, Qt::QueuedConnection);
        });

    LOG_TRACE("GLViewport created");
}

GLViewport::~GLViewport() { LOG_TRACE("GLViewport destroyed"); }

GLViewport::PickAction GLViewport::consumePickAction() {
    const auto action = m_pendingPickAction;
    m_pendingPickAction = PickAction::None;
    return action;
}

QQuickFramebufferObject::Renderer* GLViewport::createRenderer() const {
    return new GLViewportRenderer(this);
}

bool GLViewport::hasGeometry() const { return m_hasGeometry; }

const Render::CameraState& GLViewport::cameraState() const { return m_cameraState; }

const Render::DocumentRenderData& GLViewport::renderData() const {
    static Render::DocumentRenderData empty;
    return Render::RenderSceneController::instance().renderData();
}

void GLViewport::onSceneNeedsUpdate() {
    m_cameraState = Render::RenderSceneController::instance().camera();
    if(!m_trackballController.isActive()) {
        m_trackballController.syncFromCamera(m_cameraState);
    }
    update();
}

void GLViewport::onServiceGeometryChanged() {
    const bool new_has_geometry = Render::RenderSceneController::instance().hasGeometry();
    if(new_has_geometry != m_hasGeometry) {
        m_hasGeometry = new_has_geometry;
        emit hasGeometryChanged();
    }

    emit geometryChanged();
    update();
}

void GLViewport::keyPressEvent(QKeyEvent* event) {
    m_pressedModifiers = event->modifiers();
    event->accept();
}
void GLViewport::mousePressEvent(QMouseEvent* event) {
    if(!hasFocus()) {
        forceActiveFocus();
    }
    m_cursorPos = event->position();
    m_pressPos = event->position();
    m_movedSincePress = false;
    if(window()) {
        m_devicePixelRatio = window()->devicePixelRatio();
    }
    m_pressedButtons = event->buttons();

    // const bool pick_enabled = Render::SelectManager::instance().isPickEnabled();

    // // In picking mode, use right-click for removing picked entities.
    // // Keep camera zoom on right button only when not in picking mode.
    // if(!(pick_enabled && (event->buttons() & Qt::RightButton))) {
    //     m_trackballController.setViewportSize(size());
    //     const auto mode = pickMode(m_pressedButtons, m_pressedModifiers);
    //     if(mode != Render::TrackballController::Mode::None) {
    //         m_trackballController.begin(event->position(), mode, m_cameraState);
    //     }
    // }
    event->accept();
}

void GLViewport::mouseMoveEvent(QMouseEvent* event) {
    if(!hasFocus()) {
        forceActiveFocus();
    }
    m_cursorPos = event->position();
    if(window()) {
        m_devicePixelRatio = window()->devicePixelRatio();
    }

    constexpr qreal drag_threshold = 4.0;
    if((m_pressedButtons != Qt::NoButton) && !m_movedSincePress) {
        const auto delta = m_cursorPos - m_pressPos;
        if((delta.x() * delta.x() + delta.y() * delta.y()) > (drag_threshold * drag_threshold)) {
            m_movedSincePress = true;
        }
    }

    // If modifiers/buttons changed mid-drag, switch mode seamlessly.
    const auto mode = pickMode(m_pressedButtons, m_pressedModifiers);
    if(mode != m_trackballController.mode()) {
        m_trackballController.end();
        m_trackballController.setViewportSize(size());
        if(mode != Render::TrackballController::Mode::None) {
            m_trackballController.begin(event->position(), mode, m_cameraState);
        }
    }

    if(mode != Render::TrackballController::Mode::None) {
        m_trackballController.setViewportSize(size());
        m_trackballController.update(event->position(), m_cameraState);
        Render::RenderSceneController::instance().setCamera(m_cameraState, false);
    }

    update();
    event->accept();
}

void GLViewport::mouseReleaseEvent(QMouseEvent* event) {
    m_cursorPos = event->position();
    m_pressedButtons = event->buttons();

    if(m_trackballController.isActive() && m_pressedButtons == Qt::NoButton) {
        m_trackballController.end();
        Render::RenderSceneController::instance().setCamera(m_cameraState, true);
    }

    const bool pick_enabled = Render::SelectManager::instance().isPickEnabled();
    if(pick_enabled && !m_trackballController.isActive() && !m_movedSincePress) {
        if(event->button() == Qt::LeftButton) {
            m_pendingPickAction = PickAction::Add;
            update();
        } else if(event->button() == Qt::RightButton) {
            m_pendingPickAction = PickAction::Remove;
            update();
        }
    }

    event->accept();
}
void GLViewport::keyReleaseEvent(QKeyEvent* event) {
    m_pressedModifiers = event->modifiers();
    event->accept();
}

void GLViewport::wheelEvent(QWheelEvent* event) {
    // Default behavior: Ctrl + wheel zoom (keeps app-level scroll gestures safe).
    if((m_pressedModifiers & Qt::ControlModifier)) {
        m_trackballController.setViewportSize(size());
        const float steps = event->angleDelta().y() / 120.0f;
        m_trackballController.wheelZoom(steps * 2.0f, m_cameraState);
        Render::RenderSceneController::instance().setCamera(m_cameraState, true);
        update();
    }
    event->accept();
}

void GLViewport::hoverMoveEvent(QHoverEvent* event) {
    if(Render::SelectManager::instance().isPickEnabled()) {
        m_cursorPos = event->position();
        if(window()) {
            m_devicePixelRatio = window()->devicePixelRatio();
        }
        update();
    }
    event->accept();
}
// =============================================================================
// GLViewportRenderer Implementation
// =============================================================================

GLViewportRenderer::GLViewportRenderer(const GLViewport* viewport)
    : m_viewport(viewport), m_sceneRenderer(std::make_unique<Render::SceneRenderer>()) {
    LOG_TRACE("GLViewportRenderer created");
}

GLViewportRenderer::~GLViewportRenderer() {
    m_sceneRenderer->cleanup();
    LOG_TRACE("GLViewportRenderer destroyed");
}

QOpenGLFramebufferObject* GLViewportRenderer::createFramebufferObject(const QSize& size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4); // Enable MSAA
    m_viewportSize = size;
    m_sceneRenderer->setViewportSize(size);

    return new QOpenGLFramebufferObject(size, format);
}

void GLViewportRenderer::synchronize(QQuickFramebufferObject* item) {
    auto* viewport = qobject_cast<GLViewport*>(item);
    if(!viewport) {
        return;
    }

    m_cameraState = viewport->cameraState();
    m_cursorPos = viewport->cursorPos();
    m_devicePixelRatio = viewport->devicePixelRatio();
    m_itemSize = viewport->itemSize();

    m_pendingPickAction = viewport->consumePickAction();

    // Check if render data changed using version number
    const auto& new_render_data = viewport->renderData();
    if(new_render_data.m_version != m_renderData.m_version) {
        LOG_DEBUG("GLViewportRenderer: Render data changed, version {} -> {}, uploading {} meshes",
                  m_renderData.m_version, new_render_data.m_version, new_render_data.meshCount());
        m_renderData = new_render_data;
        m_needsDataUpload = true;
    }
}
namespace {
[[nodiscard]] OpenGeoLab::Geometry::EntityUID decodeUid24(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<OpenGeoLab::Geometry::EntityUID>(static_cast<uint32_t>(r) |
                                                        (static_cast<uint32_t>(g) << 8u) |
                                                        (static_cast<uint32_t>(b) << 16u));
}

[[nodiscard]] int pickPriority(OpenGeoLab::Geometry::EntityType type) {
    // Smaller is better.
    switch(type) {
    case OpenGeoLab::Geometry::EntityType::Vertex:
        return 0;
    case OpenGeoLab::Geometry::EntityType::Edge:
        return 1;
    case OpenGeoLab::Geometry::EntityType::Face:
        return 2;
    case OpenGeoLab::Geometry::EntityType::Solid:
        return 3;
    case OpenGeoLab::Geometry::EntityType::Part:
        return 4;
    default:
        return 99;
    }
}

struct PickHit {
    OpenGeoLab::Geometry::EntityType m_type{OpenGeoLab::Geometry::EntityType::None};
    OpenGeoLab::Geometry::EntityUID m_uid{OpenGeoLab::Geometry::INVALID_ENTITY_UID};
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

struct PickingContext {
    Render::SceneRenderer& m_sceneRenderer;
    const Render::DocumentRenderData& m_renderData;
    const QSize& m_viewportSize;
    const QSizeF& m_itemSize;
    const QPointF& m_cursorPos;
    qreal m_devicePixelRatio{1.0};
    const QMatrix4x4& m_view;
    const QMatrix4x4& m_projection;
    Render::SelectManager& m_selectManager;
    GLViewport::PickAction m_pendingPickAction{GLViewport::PickAction::None};
    Geometry::EntityType& m_lastHoverType;
    Geometry::EntityUID& m_lastHoverUid;
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

    // Scan radius in FBO pixels. Larger radius makes thin edges/points easier to pick.
    constexpr int pick_radius = 8; // 17x17
    const int x1 = std::min(viewport_size.width() - 1, region.m_px + pick_radius);
    const int y1_gl = std::min(viewport_size.height() - 1, gl_y + pick_radius);
    region.m_x0 = std::max(0, region.m_px - pick_radius);
    region.m_y0Gl = std::max(0, gl_y - pick_radius);
    region.m_readW = x1 - region.m_x0 + 1;
    region.m_readH = y1_gl - region.m_y0Gl + 1;
    return region;
}

[[nodiscard]] bool
readPickPixels(PickingContext& ctx, const PickRegion& region, std::vector<uint8_t>& out_pixels) {
    auto* gl_ctx = QOpenGLContext::currentContext();
    auto* f = gl_ctx ? gl_ctx->functions() : nullptr;
    if(!f) {
        return false;
    }

    // Use PickingPass to render to its internal FBO
    OpenGeoLab::Render::RenderPassContext pass_ctx;
    pass_ctx.m_core = &ctx.m_sceneRenderer.core();
    pass_ctx.m_viewportSize = ctx.m_viewportSize;
    pass_ctx.m_aspectRatio =
        ctx.m_viewportSize.width() / static_cast<float>(ctx.m_viewportSize.height());
    pass_ctx.m_matrices.m_view = ctx.m_view;
    pass_ctx.m_matrices.m_projection = ctx.m_projection;
    pass_ctx.m_matrices.m_mvp = ctx.m_projection * ctx.m_view;
    pass_ctx.m_cameraPos = QVector3D(0, 0, 0);

    auto* picking_pass = ctx.m_sceneRenderer.pickingPass();
    if(!picking_pass) {
        return false;
    }

    picking_pass->execute(*f, pass_ctx);

    // Read from the pick FBO
    GLint prev_fbo = 0;
    f->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);

    picking_pass->bindFbo(*f);

    out_pixels.assign(static_cast<size_t>(region.m_readW * region.m_readH * 4), 0);
    f->glReadPixels(region.m_x0, region.m_y0Gl, region.m_readW, region.m_readH, GL_RGBA,
                    GL_UNSIGNED_BYTE, out_pixels.data());

    picking_pass->unbindFbo(*f);

    return true;
}

[[nodiscard]] PickHit findBestPickHit(const QSize& viewport_size,
                                      const PickRegion& region,
                                      const std::vector<uint8_t>& pixels,
                                      Render::SelectManager& select_manager,
                                      Render::SelectManager::PickTypes pick_types) {
    PickHit best;

    const bool wants_part = pick_types == Render::SelectManager::PickTypes::Part;
    const bool wants_solid = pick_types == Render::SelectManager::PickTypes::Solid;
    const bool wants_wire = pick_types == Render::SelectManager::PickTypes::Wire;
    const bool wants_owner_mapping = wants_part || wants_solid || wants_wire;

    for(int iy = 0; iy < region.m_readH; ++iy) {
        const int sample_gl_y = region.m_y0Gl + iy;
        const int sample_top_y = viewport_size.height() - 1 - sample_gl_y;
        const int dy = sample_top_y - region.m_py;

        for(int ix = 0; ix < region.m_readW; ++ix) {
            const int sample_x = region.m_x0 + ix;
            const int dx = sample_x - region.m_px;
            const int dist2 = dx * dx + dy * dy;

            const size_t off = static_cast<size_t>((iy * region.m_readW + ix) * 4);
            const uint8_t r = pixels[off + 0];
            const uint8_t g = pixels[off + 1];
            const uint8_t b = pixels[off + 2];
            const uint8_t a = pixels[off + 3];

            const auto uid = decodeUid24(r, g, b);
            const auto type = static_cast<Geometry::EntityType>(a);
            if(type == Geometry::EntityType::None || uid == Geometry::INVALID_ENTITY_UID) {
                continue;
            }

            if(!wants_owner_mapping) {
                if(!select_manager.isTypePickable(type)) {
                    continue;
                }
            } else {
                // Owner-mapping pick modes:
                // - Part/Solid: pick via Face, then map to owning Part/Solid.
                // - Wire: prefer picking Edge (thin features), fallback to Face->owning Wire.
                if((wants_part || wants_solid) && type != Geometry::EntityType::Face) {
                    continue;
                }
                if(wants_wire && (type != Geometry::EntityType::Edge) &&
                   (type != Geometry::EntityType::Face)) {
                    continue;
                }
            }

            PickHit candidate;
            candidate.m_type = type;
            candidate.m_uid = uid;
            candidate.m_priority = pickPriority(type);
            candidate.m_dist2 = dist2;

            if(isBetterHit(candidate, best)) {
                best = candidate;
            }
        }
    }

    return best;
}

void applyPendingPickAction(Render::SelectManager& select_manager,
                            GLViewport::PickAction pending_pick_action,
                            const Geometry::EntityRef& ref) {
    if(pending_pick_action == GLViewport::PickAction::None || !ref.isValid()) {
        return;
    }

    if(pending_pick_action == GLViewport::PickAction::Add) {
        select_manager.addSelection(ref);
        return;
    }

    if(pending_pick_action == GLViewport::PickAction::Remove) {
        if(select_manager.containsSelection(ref)) {
            select_manager.removeSelection(ref);
        }
    }
}

void processPicking(PickingContext& ctx) {
    if(ctx.m_renderData.meshCount() <= 0 || ctx.m_viewportSize.width() <= 0 ||
       ctx.m_viewportSize.height() <= 0) {
        return;
    }

    const PickRegion region = computePickRegion(ctx.m_viewportSize, ctx.m_itemSize, ctx.m_cursorPos,
                                                ctx.m_devicePixelRatio);

    std::vector<uint8_t> pixels;
    if(!readPickPixels(ctx, region, pixels)) {
        return;
    }

    const auto pick_types = ctx.m_selectManager.pickTypes();
    const PickHit best =
        findBestPickHit(ctx.m_viewportSize, region, pixels, ctx.m_selectManager, pick_types);

    Geometry::EntityRef ref{best.m_uid, best.m_type};

    // Map sub-entity -> owning Part/Solid when pick mode requests it.
    const bool wants_part = pick_types == Render::SelectManager::PickTypes::Part;
    const bool wants_solid = pick_types == Render::SelectManager::PickTypes::Solid;
    const bool wants_wire = pick_types == Render::SelectManager::PickTypes::Wire;

    const auto document = Render::RenderSceneController::instance().currentDocument();
    if(document) {
        if(wants_wire && (ref.m_type == Geometry::EntityType::Edge) && ref.isValid()) {
            const auto owning_wires =
                document->findRelatedEntities(ref, Geometry::EntityType::Wire);
            if(!owning_wires.empty() &&
               owning_wires.front().m_uid != Geometry::INVALID_ENTITY_UID) {
                ref = Geometry::EntityRef{owning_wires.front().m_uid, Geometry::EntityType::Wire};
            } else {
                ref = Geometry::EntityRef{};
            }
        }
    }

    if((wants_part || wants_solid || wants_wire) && (ref.m_type == Geometry::EntityType::Face) &&
       ref.isValid()) {
        if(document) {
            if(wants_part) {
                const auto owning_parts =
                    document->findRelatedEntities(ref, Geometry::EntityType::Part);
                if(!owning_parts.empty() &&
                   owning_parts.front().m_uid != Geometry::INVALID_ENTITY_UID) {
                    ref =
                        Geometry::EntityRef{owning_parts.front().m_uid, Geometry::EntityType::Part};
                }
            } else if(wants_solid) {
                const auto owning_solids =
                    document->findRelatedEntities(ref, Geometry::EntityType::Solid);
                if(!owning_solids.empty() &&
                   owning_solids.front().m_uid != Geometry::INVALID_ENTITY_UID) {
                    ref = Geometry::EntityRef{owning_solids.front().m_uid,
                                              Geometry::EntityType::Solid};
                }
            } else if(wants_wire) {
                const auto owning_wires =
                    document->findRelatedEntities(ref, Geometry::EntityType::Wire);
                if(!owning_wires.empty() &&
                   owning_wires.front().m_uid != Geometry::INVALID_ENTITY_UID) {
                    ref =
                        Geometry::EntityRef{owning_wires.front().m_uid, Geometry::EntityType::Wire};
                } else {
                    ref = Geometry::EntityRef{};
                }
            }
        }
    }

    if(ref.m_type != ctx.m_lastHoverType || ref.m_uid != ctx.m_lastHoverUid) {
        ctx.m_lastHoverType = ref.m_type;
        ctx.m_lastHoverUid = ref.m_uid;
        ctx.m_sceneRenderer.setHighlightedEntity(ref);
    }

    applyPendingPickAction(ctx.m_selectManager, ctx.m_pendingPickAction, ref);
}
} // namespace
void GLViewportRenderer::render() {
    if(!m_sceneRenderer->isInitialized()) {
        m_sceneRenderer->initialize();
    }

    if(m_needsDataUpload) {
        m_sceneRenderer->uploadMeshData(m_renderData);
        m_needsDataUpload = false;
    }

    // Calculate matrices
    const float aspect_ratio = m_viewportSize.width() / static_cast<float>(m_viewportSize.height());
    const QMatrix4x4 projection = m_cameraState.projectionMatrix(aspect_ratio);
    const QMatrix4x4 view = m_cameraState.viewMatrix();

    auto& select_manager = Render::SelectManager::instance();
    const bool pick_enabled = select_manager.isPickEnabled();

    if(pick_enabled && m_sceneRenderer->pickingPass()) {
        PickingContext ctx{
            *m_sceneRenderer,   m_renderData,  m_viewportSize, m_itemSize,     m_cursorPos,
            m_devicePixelRatio, view,          projection,     select_manager, m_pendingPickAction,
            m_lastHoverType,    m_lastHoverUid};
        processPicking(ctx);
    }

    // Clear request after processing (or ignoring).
    m_pendingPickAction = GLViewport::PickAction::None;
    // Delegate rendering to SceneRenderer
    m_sceneRenderer->render(m_cameraState.m_position, view, projection);
}

} // namespace OpenGeoLab::App
