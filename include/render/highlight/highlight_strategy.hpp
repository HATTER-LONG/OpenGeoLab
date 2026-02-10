/**
 * @file highlight_strategy.hpp
 * @brief Pluggable highlight strategy interface
 *
 * Strategies can render highlights using different techniques:
 * - OutlineHighlight: stencil-buffer based edge outline
 * - InstanceHighlight: re-render selected geometry with color tint
 */

#pragma once

#include "geometry/geometry_types.hpp"
#include "render/render_pass.hpp"
#include "render/renderable.hpp"

#include <QOpenGLFunctions>
#include <unordered_set>

namespace OpenGeoLab::Render {

/**
 * @brief Set of entities to highlight (uid+type pairs)
 */
struct HighlightSet {
    struct Entry {
        Geometry::EntityType m_type{Geometry::EntityType::None};
        Geometry::EntityUID m_uid{Geometry::INVALID_ENTITY_UID};
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
