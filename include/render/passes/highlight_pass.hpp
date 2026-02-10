/**
 * @file highlight_pass.hpp
 * @brief Pluggable highlight rendering pass
 *
 * Delegates to an IHighlightStrategy to render selection/hover highlights.
 * Supports strategy swapping at runtime.
 */

#pragma once

#include "render/highlight/highlight_strategy.hpp"
#include "render/render_pass.hpp"
#include "render/renderable.hpp"

#include <memory>

namespace OpenGeoLab::Render {

/**
 * @brief Render pass that delegates highlight drawing to a strategy.
 */
class HighlightPass : public RenderPass {
public:
    HighlightPass() = default;
    ~HighlightPass() override = default;

    [[nodiscard]] const char* name() const override { return "HighlightPass"; }

    void initialize(QOpenGLFunctions& gl) override;
    void resize(QOpenGLFunctions& gl, const QSize& size) override;
    void execute(QOpenGLFunctions& gl, const RenderPassContext& ctx) override;
    void cleanup(QOpenGLFunctions& gl) override;

    /**
     * @brief Set the highlight strategy. Ownership transferred.
     */
    void setStrategy(std::unique_ptr<IHighlightStrategy> strategy);

    [[nodiscard]] IHighlightStrategy* strategy() const { return m_strategy.get(); }

private:
    std::unique_ptr<IHighlightStrategy> m_strategy;
};

} // namespace OpenGeoLab::Render
