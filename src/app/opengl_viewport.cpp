/**
 * @file opengl_viewport.cpp
 * @brief Implementation of GLViewport and GLViewportRenderer
 */

#include "app/opengl_viewport.hpp"
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

    LOG_TRACE("GLViewport created");
}

GLViewport::~GLViewport() { LOG_TRACE("GLViewport destroyed"); }

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
    if(window()) {
        m_devicePixelRatio = window()->devicePixelRatio();
    }
    m_pressedButtons = event->buttons();

    m_trackballController.setViewportSize(size());
    const auto mode = pickMode(m_pressedButtons, m_pressedModifiers);
    if(mode != Render::TrackballController::Mode::None) {
        m_trackballController.begin(event->position(), mode, m_cameraState);
    }
    event->accept();
}

void GLViewport::mouseMoveEvent(QMouseEvent* event) {
    m_cursorPos = event->position();
    if(window()) {
        m_devicePixelRatio = window()->devicePixelRatio();
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
    m_pressedButtons = event->buttons();

    if(m_trackballController.isActive() && m_pressedButtons == Qt::NoButton) {
        m_trackballController.end();
        Render::RenderSceneController::instance().setCamera(m_cameraState, true);
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
        m_trackballController.wheelZoom(steps * 5.0f, m_cameraState);
        Render::RenderSceneController::instance().setCamera(m_cameraState, true);
        update();
    }
    event->accept();
}

void GLViewport::hoverMoveEvent(QHoverEvent* event) {
    m_cursorPos = event->position();
    if(window()) {
        m_devicePixelRatio = window()->devicePixelRatio();
    }
    update();
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
    // Picking FBO (no MSAA, easier to read back)
    QOpenGLFramebufferObjectFormat pick_format;
    pick_format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    pick_format.setSamples(0);
    m_pickFbo = std::make_unique<QOpenGLFramebufferObject>(size, pick_format);

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

    if(m_pickFbo && m_renderData.meshCount() > 0 && m_viewportSize.width() > 0 &&
       m_viewportSize.height() > 0) {
        const bool has_item_size = (m_itemSize.width() > 0.0) && (m_itemSize.height() > 0.0);
        int px = 0;
        int py = 0;
        if(has_item_size) {
            const double fx = m_cursorPos.x() / m_itemSize.width();
            const double fy = m_cursorPos.y() / m_itemSize.height();
            px = static_cast<int>(std::lround(fx * static_cast<double>(m_viewportSize.width())));
            py = static_cast<int>(std::lround(fy * static_cast<double>(m_viewportSize.height())));
        } else {
            const qreal dpr = (m_devicePixelRatio > 0.0) ? m_devicePixelRatio : 1.0;
            px = static_cast<int>(std::lround(m_cursorPos.x() * dpr));
            py = static_cast<int>(std::lround(m_cursorPos.y() * dpr));
        }
        px = std::clamp(px, 0, m_viewportSize.width() - 1);
        py = std::clamp(py, 0, m_viewportSize.height() - 1);
        const int gl_y = m_viewportSize.height() - 1 - py;

        // Scan radius in FBO pixels. Larger radius makes thin edges/points easier to pick.
        constexpr int pick_radius = 8; // 17x17
        const int x0 = std::max(0, px - pick_radius);
        const int x1 = std::min(m_viewportSize.width() - 1, px + pick_radius);
        const int y0_gl = std::max(0, gl_y - pick_radius);
        const int y1_gl = std::min(m_viewportSize.height() - 1, gl_y + pick_radius);
        const int read_w = x1 - x0 + 1;
        const int read_h = y1_gl - y0_gl + 1;

        auto* ctx = QOpenGLContext::currentContext();
        auto* f = ctx ? ctx->functions() : nullptr;
        if(f) {
            GLint prev_fbo = 0;
            f->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);

            m_pickFbo->bind();
            m_sceneRenderer->renderPicking(view, projection);

            std::vector<uint8_t> pixels(static_cast<size_t>(read_w * read_h * 4), 0);
            f->glReadPixels(x0, y0_gl, read_w, read_h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

            m_pickFbo->release();
            f->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prev_fbo));

            PickHit best;

            // Evaluate all pixels in the neighborhood.
            // Readback is in GL coordinate space (origin at bottom-left).
            // Compute distance in "top-left" pixel space to match px/py.
            for(int iy = 0; iy < read_h; ++iy) {
                const int sample_gl_y = y0_gl + iy;
                const int sample_top_y = m_viewportSize.height() - 1 - sample_gl_y;
                const int dy = sample_top_y - py;

                for(int ix = 0; ix < read_w; ++ix) {
                    const int sample_x = x0 + ix;
                    const int dx = sample_x - px;
                    const int dist2 = dx * dx + dy * dy;

                    const size_t off = static_cast<size_t>((iy * read_w + ix) * 4);
                    const uint8_t r = pixels[off + 0];
                    const uint8_t g = pixels[off + 1];
                    const uint8_t b = pixels[off + 2];
                    const uint8_t a = pixels[off + 3];

                    const auto uid = decodeUid24(r, g, b);
                    const auto type = static_cast<Geometry::EntityType>(a);
                    if(type == Geometry::EntityType::None || uid == Geometry::INVALID_ENTITY_UID) {
                        continue;
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

            const auto type = best.m_type;
            const auto uid = best.m_uid;

            if(type != m_lastHoverType || uid != m_lastHoverUid) {
                m_lastHoverType = type;
                m_lastHoverUid = uid;
                if(type == Geometry::EntityType::None || uid == Geometry::INVALID_ENTITY_UID) {
                    m_sceneRenderer->setHighlightedEntity(Geometry::EntityType::None,
                                                          Geometry::INVALID_ENTITY_UID);
                } else {
                    m_sceneRenderer->setHighlightedEntity(type, uid);
                }
            }
        }
    }
    // Delegate rendering to SceneRenderer
    m_sceneRenderer->render(m_cameraState.m_position, view, projection);
}

} // namespace OpenGeoLab::App
