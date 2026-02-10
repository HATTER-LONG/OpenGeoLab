/**
 * @file scene_renderer.cpp
 * @brief SceneRenderer facade implementation
 *
 * Delegates to RendererCore + registered passes:
 * - GeometryPass for main scene rendering (faces, edges, vertices)
 * - MeshPass for FEM mesh element and node rendering
 * - PickingPass for entity picking
 * - HighlightPass for selection/hover highlighting
 */

#include "render/scene_renderer.hpp"
#include "render/highlight/outline_highlight.hpp"
#include "render/select_manager.hpp"
#include "util/logger.hpp"

#include <QOpenGLFramebufferObject>

namespace OpenGeoLab::Render {

SceneRenderer::SceneRenderer() { LOG_TRACE("SceneRenderer created (facade)"); }

SceneRenderer::~SceneRenderer() {
    cleanup();
    LOG_TRACE("SceneRenderer destroyed");
}

void SceneRenderer::initialize() {
    if(m_core.isInitialized()) {
        return;
    }

    // Register passes in order: geometry -> mesh -> highlight
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

    // PickingPass is owned directly by SceneRenderer and called on-demand.
    m_pickingPass = std::make_unique<PickingPass>();
    QOpenGLFunctions gl_funcs;
    gl_funcs.initializeOpenGLFunctions();
    m_pickingPass->initialize(gl_funcs);

    LOG_DEBUG("SceneRenderer: Initialized with RendererCore pipeline");
}

bool SceneRenderer::isInitialized() const { return m_core.isInitialized(); }

void SceneRenderer::setViewportSize(const QSize& size) {
    m_core.setViewportSize(size);
    if(m_pickingPass && m_core.isInitialized()) {
        QOpenGLFunctions gl_funcs;
        gl_funcs.initializeOpenGLFunctions();
        m_pickingPass->resize(gl_funcs, size);
    }
}

void SceneRenderer::uploadMeshData(const DocumentRenderData& render_data) {
    m_core.uploadMeshData(render_data);
}

void SceneRenderer::renderPicking(const QMatrix4x4& view_matrix,
                                  const QMatrix4x4& projection_matrix) {
    if(!m_core.isInitialized() || !m_pickingPass) {
        return;
    }

    QOpenGLFunctions gl_funcs;
    gl_funcs.initializeOpenGLFunctions();

    RenderPassContext ctx;
    ctx.m_core = &m_core;
    ctx.m_viewportSize = m_core.viewportSize();
    ctx.m_aspectRatio =
        ctx.m_viewportSize.width() / static_cast<float>(ctx.m_viewportSize.height());
    ctx.m_matrices.m_view = view_matrix;
    ctx.m_matrices.m_projection = projection_matrix;
    ctx.m_matrices.m_mvp = projection_matrix * view_matrix;
    ctx.m_cameraPos = QVector3D(0, 0, 0); // Not needed for picking

    m_pickingPass->execute(gl_funcs, ctx);
}

void SceneRenderer::setHighlightedEntity(Geometry::EntityUID uid, Geometry::EntityType type) {
    if(m_geometryPass) {
        m_geometryPass->setHighlightedEntity(uid, type);
    }
    if(m_meshPass) {
        m_meshPass->setHighlightedEntity(uid, type);
    }
}

void SceneRenderer::render(const QVector3D& camera_pos,
                           const QMatrix4x4& view_matrix,
                           const QMatrix4x4& projection_matrix) {
    m_core.render(camera_pos, view_matrix, projection_matrix);
}

void SceneRenderer::cleanup() {
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
    LOG_DEBUG("SceneRenderer: Cleanup complete");
}

} // namespace OpenGeoLab::Render
