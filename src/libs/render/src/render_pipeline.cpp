/**
 * @file render_pipeline.cpp
 * @brief RenderPipeline implementation
 */

#include <opengeolab/render/render_pipeline.hpp>

#include "core/gpu_buffer_manager.hpp"
#include "pass/highlight_pass.hpp"
#include "pass/opaque_pass.hpp"
#include "pass/selection_pass.hpp"
#include "pass/wireframe_pass.hpp"
#include "pick_resolver.hpp"
#include "render_pipeline_detail.hpp"

#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/topology_index.hpp>

#include <glad/gl.h>

#include <memory>
#include <vector>

namespace OpenGeoLab::Render {

struct RenderPipeline::Impl {
    GpuBufferManager bufferManager;
    OpaquePass opaquePass;
    WireframePass wireframePass;
    HighlightPass highlightPass;
    SelectionPass selectionPass;
    std::unique_ptr<Scene::TopologyIndex> topologyIndex;
    std::unique_ptr<PickResolver> pickResolver;
    bool initialized{false};
};

RenderPipeline::RenderPipeline() : m_impl(std::make_unique<Impl>()) {}

RenderPipeline::~RenderPipeline() = default;

void RenderPipeline::initialize(GlLoaderFunc gl_loader) {
    if(gl_loader != nullptr) {
        gladLoadGL(reinterpret_cast<GLADloadfunc>(gl_loader));
    }

    m_impl->bufferManager.initialize();
    m_impl->opaquePass.initialize();
    m_impl->wireframePass.initialize();
    m_impl->highlightPass.initialize();
    m_impl->selectionPass.initialize();
    m_impl->initialized = true;
}

void RenderPipeline::synchronize(const Scene::SceneGraph& scene) {
    m_impl->bufferManager.synchronize(scene);

    // SceneGraph does not currently expose the GeometrySceneBridge TopologyIndex.
    // Keep a resolver available for VEF and Part picking while remaining ready
    // to switch to scene-provided topology once that accessor exists.
    m_impl->topologyIndex = std::make_unique<Scene::TopologyIndex>();
    m_impl->pickResolver = std::make_unique<PickResolver>(*m_impl->topologyIndex);
}

void RenderPipeline::render(const FrameState& state) {
    if(!m_impl->initialized) {
        return;
    }

    glViewport(0, 0, state.viewportWidth, state.viewportHeight);
    glClearColor(0.149F, 0.149F, 0.169F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    m_impl->opaquePass.render(state, m_impl->bufferManager);
    m_impl->wireframePass.render(state, m_impl->bufferManager);
    m_impl->highlightPass.render(state, m_impl->bufferManager);
    m_impl->selectionPass.render(state, m_impl->bufferManager);
}

PickResult RenderPipeline::pickAt(int x, int y, PickMask mask) const {
    if(!m_impl->pickResolver) {
        return {};
    }

    const uint64_t raw_pick_id = m_impl->selectionPass.pickFbo().readPickId(x, y);
    return m_impl->pickResolver->resolve({raw_pick_id}, Detail::pickModeFromMask(mask));
}

std::vector<PickResult>
RenderPipeline::pickRegion(int cx, int cy, int radius, PickMask mask) const {
    if(!m_impl->pickResolver) {
        return {};
    }

    const auto raw_pick_ids = m_impl->selectionPass.pickFbo().readPickRegion(cx, cy, radius);
    return m_impl->pickResolver->resolveAll(raw_pick_ids, Detail::pickModeFromMask(mask));
}

void RenderPipeline::cleanup() {
    m_impl->selectionPass.cleanup();
    m_impl->highlightPass.cleanup();
    m_impl->wireframePass.cleanup();
    m_impl->opaquePass.cleanup();
    m_impl->bufferManager.cleanup();
    m_impl->pickResolver.reset();
    m_impl->topologyIndex.reset();
    m_impl->initialized = false;
}

} // namespace OpenGeoLab::Render
