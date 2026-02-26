/**
 * @file color_map.hpp
 * @brief Color palette for geometry and mesh rendering
 */

#pragma once
#include "render/render_data.hpp"

#include <kangaroo/util/noncopyable.hpp>

#include <cstdint>

namespace OpenGeoLab::Util {

/**
 * @brief Singleton providing colors for all render entities.
 *
 * Separates hover/selection colors by entity category:
 * edges and vertices share one set of highlight colors,
 * faces use a different set. Part colors are assigned from a
 * fixed palette indexed by entity UID.
 */
class ColorMap : public Kangaroo::Util::NonCopyMoveable {
    ColorMap() = default;

public:
    static const ColorMap& instance();
    ~ColorMap() = default;

    /** @brief Get a part display color from the cyclic palette.
     *  @param part_uid Part entity UID used to index the palette.
     */
    const Render::RenderColor& getColorForPartId(Geometry::EntityUID part_uid) const noexcept;

    /** @brief Darken a color by a multiplicative factor.
     *  @param color Source color.
     *  @param factor Darkening factor (0..1, e.g. 0.7 = 30% darker).
     *  @return Darkened color with original alpha preserved.
     */
    static Render::RenderColor darkenColor(const Render::RenderColor& color, float factor) noexcept;

    /** @brief Get a mesh element display color from the cyclic palette.
     *  @param element_uid Mesh element UID used to index the palette.
     */
    const Render::RenderColor&
    getColorForMeshElementId(Mesh::MeshElementUID element_uid) const noexcept;

    /// @brief Hover color for edges and vertices (#ff7f00)
    const Render::RenderColor& getEdgeVertexHoverColor() const noexcept;
    /// @brief Selection color for edges and vertices (#ff165d)
    const Render::RenderColor& getEdgeVertexSelectionColor() const noexcept;
    /// @brief Hover color for faces (#4b55e9)
    const Render::RenderColor& getFaceHoverColor() const noexcept;
    /// @brief Selection color for faces (#4116ff)
    const Render::RenderColor& getFaceSelectionColor() const noexcept;

    /// @brief Default edge/boundary line color
    const Render::RenderColor& getEdgeColor() const noexcept;
    /// @brief Default vertex point color
    const Render::RenderColor& getVertexColor() const noexcept;
    /// @brief Default mesh node color
    const Render::RenderColor& getMeshNodeColor() const noexcept;
    /// @brief Default mesh line color
    const Render::RenderColor& getMeshLineColor() const noexcept;
    /// @brief Viewport background color
    const Render::RenderColor& getBackgroundColor() const noexcept;
};

/**
 * @brief Compile-time render style constants (line widths, point sizes).
 *
 * Centralises style values that were previously in ColorMap.
 */
struct RenderStyle {
    /// @name Edge line widths
    /// @{
    static constexpr float EDGE_LINE_WIDTH = 1.0f;
    static constexpr float EDGE_LINE_WIDTH_HOVER = 2.0f;
    static constexpr float EDGE_LINE_WIDTH_SELECTED = 1.5f;
    /// @}

    /// @name Vertex point sizes
    /// @{
    static constexpr float VERTEX_POINT_SIZE = 5.0f;
    static constexpr float VERTEX_SCALE_HOVER = 2.0f;
    static constexpr float VERTEX_SCALE_SELECTED = 1.5f;
    /// @}
};
} // namespace OpenGeoLab::Util
