/**
 * @file render_data_provider.hpp
 * @brief Interface for providing render data from geometry entities
 *
 * Defines the interface that geometry entities implement to provide
 * render data for OpenGL visualization.
 */

#pragma once

#include "render/render_data.hpp"

namespace OpenGeoLab::Render {

/**
 * @brief Interface for objects that can provide render data
 *
 * This interface is implemented by geometry entities to generate
 * render-ready mesh data for OpenGL visualization.
 */
class IRenderDataProvider {
public:
    virtual ~IRenderDataProvider() = default;

    /**
     * @brief Generate render mesh data for this entity
     * @return RenderMesh containing vertices and indices
     * @note Implementation should tessellate the geometry appropriately
     */
    [[nodiscard]] virtual RenderMesh generateRenderMesh() const = 0;

    /**
     * @brief Check if entity has renderable geometry
     * @return true if entity can be rendered
     */
    [[nodiscard]] virtual bool isRenderable() const = 0;
};

} // namespace OpenGeoLab::Render
