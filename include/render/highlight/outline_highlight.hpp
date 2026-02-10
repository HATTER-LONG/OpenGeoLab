/**
 * @file outline_highlight.hpp
 * @brief Stencil-based outline highlight strategy
 */

#pragma once

#include "render/highlight/highlight_strategy.hpp"
#include <QOpenGLShaderProgram>
#include <memory>

namespace OpenGeoLab::Render {

/**
 * @brief Highlight strategy that draws a solid-color outline around
 *        selected/hovered entities using stencil buffer techniques.
 */
class OutlineHighlight : public IHighlightStrategy {
public:
    OutlineHighlight() = default;
    ~OutlineHighlight() override = default;

    [[nodiscard]] const char* name() const override { return "OutlineHighlight"; }

    void initialize(QOpenGLFunctions& gl) override;
    void resize(QOpenGLFunctions& gl, const QSize& size) override;
    void cleanup(QOpenGLFunctions& gl) override;

    void render(QOpenGLFunctions& gl,
                const RenderPassContext& ctx,
                RenderBatch& batch,
                const HighlightSet& highlights) override;

    void setOutlineWidth(float width) { m_outlineWidth = width; }
    void setHoverColor(const QVector4D& color) { m_hoverColor = color; }
    void setSelectionColor(const QVector4D& color) { m_selectionColor = color; }

private:
    void renderOutline(QOpenGLFunctions& gl,
                       const RenderPassContext& ctx,
                       RenderBatch& batch,
                       const HighlightSet::Entry& entry,
                       const QVector4D& color);

    std::unique_ptr<QOpenGLShaderProgram> m_outlineShader;
    int m_outlineMvpLoc{-1};
    int m_outlineColorLoc{-1};
    int m_outlineScaleLoc{-1};
    int m_outlineCenterLoc{-1};

    float m_outlineWidth{2.0f};
    QVector4D m_hoverColor{0.31f, 0.77f, 0.97f, 1.0f};
    QVector4D m_selectionColor{0.12f, 0.53f, 0.90f, 1.0f};
};

} // namespace OpenGeoLab::Render
