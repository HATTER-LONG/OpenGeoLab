/**
 * @file gl_viewport_renderer.cpp
 * @brief GLViewportRenderer implementation
 */

#include <opengeolab/app/gl_viewport_renderer.hpp>

#include <opengeolab/app/gl_viewport.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <glad/gl.h>

#include <QMetaObject>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QQuickOpenGLUtils>
#include <QQuickWindow>

#include <algorithm>

namespace OpenGeoLab::App {

namespace {

[[nodiscard]] Render::PickMask pickMaskFromMode(Render::PickMode mode) noexcept {
    switch(mode) {
    case Render::PickMode::VEF:
        return Render::PickMask::Vertex | Render::PickMask::Edge | Render::PickMask::Face;
    case Render::PickMode::Wire:
        return Render::PickMask::Wire;
    case Render::PickMode::Solid:
        return Render::PickMask::Solid;
    case Render::PickMode::Part:
        return Render::PickMask::Part;
    }

    return Render::PickMask::Vertex | Render::PickMask::Edge | Render::PickMask::Face;
}

} // namespace

GLViewportRenderer::GLViewportRenderer() = default;

GLViewportRenderer::~GLViewportRenderer() {
    if(m_pipelineInitialized && QOpenGLContext::currentContext() != nullptr) {
        m_pipeline.cleanup();
    }
}

QOpenGLFramebufferObject* GLViewportRenderer::createFramebufferObject(const QSize& size) {
    static_cast<void>(ensureGladInitialized());

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setInternalTextureFormat(GL_RGBA8);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}

void GLViewportRenderer::synchronize(QQuickFramebufferObject* item) {
    auto* viewport = static_cast<GLViewport*>(item);
    m_viewport = viewport;

    if(!ensureGladInitialized()) {
        return;
    }

    if(!m_pipelineInitialized) {
        auto gl_loader = +[](const char* name) -> void* {
            if(const auto* const ctx = QOpenGLContext::currentContext(); ctx != nullptr) {
                return reinterpret_cast<void*>(ctx->getProcAddress(name));
            }
            return nullptr;
        };
        m_pipeline.initialize(gl_loader);
        m_pipelineInitialized = true;
    }

    if(const auto* scene = viewport->sceneGraph(); scene != nullptr) {
        [[maybe_unused]] auto lock = scene->readLock();
        m_pipeline.synchronize(*scene);
    }

    const auto* const window = viewport->window();
    const float device_pixel_ratio =
        window != nullptr ? static_cast<float>(window->devicePixelRatio()) : 1.0F;
    const float width_px =
        static_cast<float>(std::max(viewport->width(), 1.0)) * device_pixel_ratio;
    const float height_px =
        static_cast<float>(std::max(viewport->height(), 1.0)) * device_pixel_ratio;
    const float aspect = width_px / std::max(height_px, 1.0F);

    const auto& camera = viewport->cameraState();
    m_frameState.viewMatrix = camera.viewMatrix();
    m_frameState.projMatrix = camera.projMatrix(aspect);
    m_frameState.cameraPos = camera.eyePosition();
    m_frameState.viewportWidth = static_cast<int>(width_px);
    m_frameState.viewportHeight = static_cast<int>(height_px);
    m_frameState.devicePixelRatio = device_pixel_ratio;
    m_frameState.xRayMode = viewport->xRayMode();
    m_frameState.selectedDrawRanges.clear();
    m_frameState.hoveredDrawRanges.clear();

    m_pickingEnabled = viewport->pickingEnabled();
    m_pickMode = static_cast<Render::PickMode>(viewport->pickMode());
    m_pendingPick = viewport->consumePendingPick();
    m_hoverPick = {m_pickingEnabled && viewport->m_hoverActive,
                   static_cast<float>(viewport->m_hoverPos.x()),
                   static_cast<float>(viewport->m_hoverPos.y())};
}

void GLViewportRenderer::render() {
    if(!m_gladInitialized || !m_pipelineInitialized) {
        return;
    }

    m_pipeline.render(m_frameState);

    if(m_pickingEnabled) {
        if(m_hoverPick.active) {
            dispatchHoverResult(pickAtItemPosition(m_hoverPick.x, m_hoverPick.y));
        }

        if(m_pendingPick.active) {
            dispatchPickResult(pickAtItemPosition(m_pendingPick.x, m_pendingPick.y));
            m_pendingPick = {};
        }
    }

    QQuickOpenGLUtils::resetOpenGLState();
}

bool GLViewportRenderer::ensureGladInitialized() {
    if(m_gladInitialized) {
        return true;
    }

    if(QOpenGLContext::currentContext() == nullptr) {
        return false;
    }

    const int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(+[](const char* name) -> void* {
        if(const auto* const context = QOpenGLContext::currentContext(); context != nullptr) {
            return reinterpret_cast<void*>(context->getProcAddress(name));
        }
        return nullptr;
    }));
    m_gladInitialized = version != 0;
    return m_gladInitialized;
}

Render::PickMask GLViewportRenderer::pickMask() const { return pickMaskFromMode(m_pickMode); }

Render::PickResult GLViewportRenderer::pickAtItemPosition(float x, float y) const {
    if(m_frameState.viewportWidth <= 0 || m_frameState.viewportHeight <= 0) {
        return {};
    }

    const float device_x = x * m_frameState.devicePixelRatio;
    const float device_y = y * m_frameState.devicePixelRatio;
    const int pixel_x =
        std::clamp(static_cast<int>(device_x), 0, std::max(m_frameState.viewportWidth - 1, 0));
    const int pixel_y =
        std::clamp(static_cast<int>(device_y), 0, std::max(m_frameState.viewportHeight - 1, 0));
    return m_pipeline.pickAt(pixel_x, pixel_y, pickMask());
}

void GLViewportRenderer::dispatchPickResult(const Render::PickResult& result) const {
    if(m_viewport.isNull()) {
        return;
    }

    const QPointer<GLViewport> viewport = m_viewport;
    QMetaObject::invokeMethod(
        viewport.data(),
        [viewport, result]() {
            if(!viewport.isNull()) {
                viewport->notifyPickResult(result);
            }
        },
        Qt::QueuedConnection);
}

void GLViewportRenderer::dispatchHoverResult(const Render::PickResult& result) const {
    if(m_viewport.isNull()) {
        return;
    }

    const QPointer<GLViewport> viewport = m_viewport;
    QMetaObject::invokeMethod(
        viewport.data(),
        [viewport, result]() {
            if(!viewport.isNull()) {
                viewport->notifyHoverResult(result);
            }
        },
        Qt::QueuedConnection);
}

} // namespace OpenGeoLab::App
