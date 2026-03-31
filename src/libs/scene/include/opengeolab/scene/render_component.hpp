/**
 * @file render_component.hpp
 * @brief IRenderComponent — abstract interface for renderable scene nodes
 */

#pragma once

#include <opengeolab/scene/render_mesh_data.hpp>
#include <opengeolab/scene/scene_export.hpp>

#include <cstdint>

namespace OpenGeoLab::Scene {

/**
 * @brief Interface for scene nodes that provide render data.
 *
 * Concrete implementations hold RenderMeshData and expose it to
 * the render pipeline via meshData(). dataVersion() enables
 * dirty-check synchronization.
 */
class OPENGEOLAB_SCENE_EXPORT IRenderComponent {
public:
    virtual ~IRenderComponent() = default;

    /** @brief Access the render mesh data owned by this component. */
    [[nodiscard]] virtual const RenderMeshData& meshData() const = 0;

    /** @brief Current data version for dirty-check synchronization. */
    [[nodiscard]] virtual uint64_t dataVersion() const = 0;
};

} // namespace OpenGeoLab::Scene
