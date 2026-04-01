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

#include <opengeolab/scene/pick_id.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/topology_index.hpp>

#include <glad/gl.h>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <vector>

namespace OpenGeoLab::Render {

namespace {

/// Remove raw pick IDs whose entity type is not allowed by @p mask.
/// Only applied in VEF mode where multiple entity types coexist.
void filterByMask(std::vector<uint64_t>& ids, PickMask mask) {
    std::erase_if(ids, [mask](uint64_t id) {
        if(!Scene::PickId::isValid(id)) {
            return false;
        }
        const auto type = Scene::PickId::decodeType(id);
        return (Core::maskForEntityType(type) & mask) == PickMask::None;
    });
}

} // namespace

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

    // Log GL line width capability for diagnostics.
    {
        GLfloat range[2] = {0.F, 0.F};
        glGetFloatv(GL_LINE_WIDTH_RANGE, range);
        std::fprintf(stderr, "[RenderPipeline] GL_LINE_WIDTH_RANGE: [%.1f, %.1f]\n",
                     static_cast<double>(range[0]), static_cast<double>(range[1]));
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

    // Read 13×13 neighborhood sorted by distance from center.
    // PickResolver applies Vertex > Edge > Face priority in VEF mode.
    constexpr int PICK_NEIGHBORHOOD_RADIUS = 6;
    auto raw_pick_ids =
        m_impl->selectionPass.pickFbo().readPickRegion(x, y, PICK_NEIGHBORHOOD_RADIUS);

    const auto mode = Detail::pickModeFromMask(mask);
    if(mode == PickMode::VEF) {
        filterByMask(raw_pick_ids, mask);
    }

    return m_impl->pickResolver->resolve(raw_pick_ids, mode);
}

std::vector<PickResult>
RenderPipeline::pickRegion(int cx, int cy, int radius, PickMask mask) const {
    if(!m_impl->pickResolver) {
        return {};
    }

    auto raw_pick_ids = m_impl->selectionPass.pickFbo().readPickRegion(cx, cy, radius);
    const auto mode = Detail::pickModeFromMask(mask);
    if(mode == PickMode::VEF) {
        filterByMask(raw_pick_ids, mask);
    }
    return m_impl->pickResolver->resolveAll(raw_pick_ids, mode);
}

std::vector<PickResult>
RenderPipeline::pickRect(int x0, int y0, int x1, int y1, PickMask mask) const {
    if(!m_impl->pickResolver) {
        return {};
    }
    auto raw_pick_ids = m_impl->selectionPass.pickFbo().readPickRect(x0, y0, x1, y1);
    const auto mode = Detail::pickModeFromMask(mask);
    if(mode == PickMode::VEF) {
        filterByMask(raw_pick_ids, mask);
    }
    return m_impl->pickResolver->resolveAll(raw_pick_ids, mode);
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

std::span<const Scene::DrawRange> RenderPipeline::resolveEntityDrawRanges(
    uint32_t shape_id, Core::EntityType entity_type, uint32_t local_id) const {
    return m_impl->bufferManager.lookupEntity(shape_id, entity_type, local_id);
}

std::vector<Scene::DrawRange> RenderPipeline::resolveShapeDrawRanges(uint32_t shape_id) const {
    std::vector<Scene::DrawRange> result;
    for(const auto& r : m_impl->bufferManager.triangleRanges()) {
        if(r.shapeId == shape_id) {
            result.push_back(r);
        }
    }
    for(const auto& r : m_impl->bufferManager.lineRanges()) {
        if(r.shapeId == shape_id) {
            result.push_back(r);
        }
    }
    for(const auto& r : m_impl->bufferManager.pointRanges()) {
        if(r.shapeId == shape_id) {
            result.push_back(r);
        }
    }
    return result;
}

} // namespace OpenGeoLab::Render
