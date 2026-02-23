/**
 * @file highlight_strategy.hpp
 * @brief Pluggable highlight strategy interface
 *
 * Strategies can render highlights using different techniques:
 * - OutlineHighlight: stencil-buffer based edge outline
 * - InstanceHighlight: re-render selected geometry with color tint
 */

#pragma once

#include "render/render_pass.hpp"
#include "render/render_types.hpp"
#include "render/renderable.hpp"

#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {

/**
 * @brief Set of entities to highlight (RenderEntityType + uid56 pairs)
 */
struct HighlightSet {
    struct Entry {
        RenderEntityType m_type{RenderEntityType::None};
        uint64_t m_uid56{0};

        [[nodiscard]] bool isValid() const {
            return m_type != RenderEntityType::None && m_uid56 != 0;
        }

        [[nodiscard]] uint64_t packed() const {
            return RenderUID::encode(m_type, m_uid56).m_packed;
        }
    };

    std::vector<Entry> m_hover;    ///< Currently hovered entities
    std::vector<Entry> m_selected; ///< Currently selected entities

    [[nodiscard]] bool empty() const { return m_hover.empty() && m_selected.empty(); }
};

/**
 * @brief Interface for highlight rendering strategies.
 */
class IHighlightStrategy {
public:
    virtual ~IHighlightStrategy() = default;

    [[nodiscard]] virtual const char* name() const = 0;

    virtual void initialize(QOpenGLFunctions& gl) = 0;
    virtual void resize(QOpenGLFunctions& gl, const QSize& size) = 0;
    virtual void cleanup(QOpenGLFunctions& gl) = 0;

    /**
     * @brief Render highlight effects for the given entities.
     * @param gl OpenGL functions
     * @param ctx Render pass context
     * @param batch Current render batch with uploaded buffers
     * @param highlights Set of entities to highlight
     */
    virtual void render(QOpenGLFunctions& gl,
                        const RenderPassContext& ctx,
                        RenderBatch& batch,
                        const HighlightSet& highlights) = 0;

protected:
    IHighlightStrategy() = default;
};

} // namespace OpenGeoLab::Render
