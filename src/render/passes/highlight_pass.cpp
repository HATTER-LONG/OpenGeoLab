/**
 * @file highlight_pass.cpp
 * @brief HighlightPass implementation - delegates to IHighlightStrategy
 */

#include "render/passes/highlight_pass.hpp"
#include "render/renderer_core.hpp"
#include "render/select_manager.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Render {

void HighlightPass::initialize(QOpenGLFunctions& gl) {
    if(m_strategy) {
        m_strategy->initialize(gl);
    }
    LOG_DEBUG("HighlightPass: Initialized");
}

void HighlightPass::resize(QOpenGLFunctions& gl, const QSize& size) {
    if(m_strategy) {
        m_strategy->resize(gl, size);
    }
}

void HighlightPass::execute(QOpenGLFunctions& gl, const RenderPassContext& ctx) {
    if(!m_strategy || !ctx.m_core) {
        return;
    }

    // Build the highlight set from SelectManager state
    HighlightSet highlights;

    const auto& sm = SelectManager::instance();
    const auto& selections = sm.selections();
    for(const auto& sel : selections) {
        HighlightSet::Entry entry;
        entry.m_type = sel.m_type;
        entry.m_uid56 = sel.m_uid56;
        highlights.m_selected.push_back(entry);
    }

    if(highlights.empty()) {
        return;
    }

    m_strategy->render(gl, ctx, ctx.m_core->batch(), highlights);
}

void HighlightPass::cleanup(QOpenGLFunctions& gl) {
    if(m_strategy) {
        m_strategy->cleanup(gl);
    }
    LOG_DEBUG("HighlightPass: Cleanup");
}

void HighlightPass::setStrategy(std::unique_ptr<IHighlightStrategy> strategy) {
    m_strategy = std::move(strategy);
}

} // namespace OpenGeoLab::Render
