#include "render_sceneImpl.hpp"

#include "render/render_scene_controller.hpp"
#include "render_scene_impl_internal.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {

RenderSceneImpl::RenderSceneImpl() {
    LOG_DEBUG("RenderSceneImpl: Created new render scene instance");
}

RenderSceneImpl::~RenderSceneImpl() {
    LOG_DEBUG("RenderSceneImpl: Destroying render scene instance");
}

void RenderSceneImpl::initialize() { LOG_DEBUG("RenderSceneImpl: Initializing render scene"); }

[[nodiscard]] bool RenderSceneImpl::isInitialized() const { return m_initialized; }

void RenderSceneImpl::setViewportSize(const QSize& size) {
    m_viewportSize = size;
    LOG_DEBUG("RenderSceneImpl: Viewport size set to {}x{}", size.width(), size.height());
}

void RenderSceneImpl::render(const QVector3D& camera_pos,
                             const QMatrix4x4& view_matrix,
                             const QMatrix4x4& projection_matrix) {
    auto* context = QOpenGLContext::currentContext();
    if(!context) {
        LOG_WARN("RenderSceneImpl: no active OpenGL context, skip rendering");
        return;
    }

    auto* gl = context->functions();
    if(!gl) {
        LOG_WARN("RenderSceneImpl: no OpenGL functions available");
        return;
    }

    if(!m_initialized) {
        gl->glEnable(GL_DEPTH_TEST);
        gl->glEnable(GL_BLEND);
        gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_initialized = true;
        LOG_DEBUG("RenderSceneImpl: OpenGL state initialized");
    }

    ensureGpuResources();
    if(!m_gpuReady) {
        LOG_WARN("RenderSceneImpl: GPU resources not ready");
        return;
    }

    if(m_viewportSize.width() > 0 && m_viewportSize.height() > 0) {
        gl->glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());
    }

    const auto& bucket = RenderSceneController::instance().renderData();
    const auto display_mode = RenderSceneController::instance().displayMode();
    const auto revision = RenderSceneController::instance().renderRevision();

    if(!m_hasUploadedData || revision != m_lastUploadedRevision ||
       display_mode != m_lastUploadedMode) {
        uploadBuckets(bucket, display_mode);
        m_lastUploadedRevision = revision;
        m_lastUploadedMode = display_mode;
        m_hasUploadedData = true;
    }

    QMatrix4x4 mvp = projection_matrix * view_matrix;

    gl->glClearColor(0.08F, 0.08F, 0.1F, 1.0F);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawBuckets(mvp);

    const auto count_visible = [display_mode](const RenderData& pass_data) {
        size_t visible = 0;
        for(const auto& primitive : pass_data.m_primitives) {
            if(primitive.m_visible && Detail::isModeVisible(display_mode, primitive.m_topology)) {
                ++visible;
            }
        }
        return visible;
    };

    const auto geometry_count = count_visible(bucket.m_geometryPass);
    const auto mesh_count = count_visible(bucket.m_meshPass);
    const auto post_count = count_visible(bucket.m_postPass);

    LOG_DEBUG("RenderSceneImpl: Rendering scene with camera at ({}, {}, {})", camera_pos.x(),
              camera_pos.y(), camera_pos.z());
    LOG_TRACE("RenderSceneImpl: mode={}, pass primitives geometry={}, mesh={}, post={}",
              static_cast<int>(display_mode), geometry_count, mesh_count, post_count);
}

void RenderSceneImpl::cleanup() {
    releaseGpuResources();
    releasePickFramebuffer();
    m_initialized = false;
    m_hasUploadedData = false;
    m_lastUploadedRevision = 0;
    m_lastUploadedMode = RenderDisplayMode::Surface;
    LOG_DEBUG("RenderSceneImpl: Cleaning up render scene");
}

} // namespace OpenGeoLab::Render
