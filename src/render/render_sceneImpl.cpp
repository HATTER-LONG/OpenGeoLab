#include "render_sceneImpl.hpp"

#include "render/render_data.hpp"
#include "render/render_scene_controller.hpp"
#include "render/render_select_manager.hpp"
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

    // Clear viewport
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    f->glEnable(GL_DEPTH_TEST);

    // Execute render passes
    m_geometryPass.render(view_matrix, projection_matrix, camera_pos);
    m_meshPass.render(view_matrix, projection_matrix, camera_pos);
}

// =============================================================================
// Picking
// =============================================================================

void RenderSceneImpl::processPicking(const PickingInput& input) {
    if(!m_initialized || !m_pickPassInitialized) {
        return;
    }
    if(input.m_action == PickAction::None) {
        return;
    }

    // Convert cursor position to framebuffer pixel coordinates (flip Y for OpenGL)
    const int px = static_cast<int>(input.m_cursorPos.x() * input.m_devicePixelRatio);
    const int py = static_cast<int>((input.m_itemSize.height() - input.m_cursorPos.y()) *
                                    input.m_devicePixelRatio);

    const uint64_t encoded =
        m_pickPass.execute(px, py, input.m_viewMatrix, input.m_projectionMatrix,
                           m_geometryPass.gpuBuffer(), m_meshPass.gpuBuffer());

    auto& selectMgr = RenderSelectManager::instance();

    if(encoded == 0) {
        // Clicked background
        if(input.m_action == PickAction::Add) {
            selectMgr.clearSelection();
        }
        return;
    }

    const RenderEntityType type = PickId::decodeType(encoded);
    const uint64_t uid = PickId::decodeUID(encoded);

    if(!selectMgr.isTypePickable(type)) {
        return;
    }

    if(input.m_action == PickAction::Add) {
        selectMgr.addSelection(uid, type);
    } else if(input.m_action == PickAction::Remove) {
        selectMgr.removeSelection(uid, type);
    }

    LOG_DEBUG("RenderSceneImpl: Pick result type={}, uid={}", static_cast<int>(type), uid);
}

// =============================================================================
// Cleanup
// =============================================================================

void RenderSceneImpl::cleanup() {
    m_geometryPass.cleanup();
    m_meshPass.cleanup();
    m_pickPass.cleanup();
    m_initialized = false;
    m_pickPassInitialized = false;
    LOG_DEBUG("RenderSceneImpl: Cleaned up");
}

} // namespace OpenGeoLab::Render
