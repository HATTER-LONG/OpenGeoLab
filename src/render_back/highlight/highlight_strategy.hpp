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
#include <kangaroo/util/noncopyable.hpp>
namespace OpenGeoLab::Render {

struct HighlightParams {
    std::vector<Geometry::EntityRef> m_hover;
    std::vector<Geometry::EntityRef> m_selection;

    [[nodiscard]] bool empty() const { return m_hover.empty() && m_selection.empty(); }
};

class IHighlightStrategy : public Kangaroo::Util::NonCopyMoveable {
public:
    IHighlightStrategy() = default;
    virtual ~IHighlightStrategy() = default;

    /**
     * @brief Apply highlights based on the given parameters
     * @param params Highlight parameters (hovered/selected entities)
     */
    virtual void applyHighlights(const HighlightParams& params) = 0;
};

} // namespace OpenGeoLab::Render