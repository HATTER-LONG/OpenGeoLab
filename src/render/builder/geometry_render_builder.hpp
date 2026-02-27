/**
 * @file geometry_render_builder.hpp
 * @brief Converts OCC geometry entities into GPU-ready render data (vertices, draw ranges).
 */

#pragma once

#include "render/render_data.hpp"

#include <memory>
#include <vector>

namespace OpenGeoLab::Geometry {
class EntityIndex;
class EntityRelationshipIndex;
class GeometryEntityImpl;
using GeometryEntityImplPtr = std::shared_ptr<GeometryEntityImpl>;
} // namespace OpenGeoLab::Geometry

namespace OpenGeoLab::Render {

/** @brief Input parameters for geometry render data generation. */
struct GeometryRenderInput {
    const Geometry::EntityIndex& m_entityIndex;
    const Geometry::EntityRelationshipIndex& m_relationshipIndex;
    TessellationOptions m_options;
};

/**
 * @brief Builds GPU render data from OCC geometry topology.
 *
 * Tessellates faces, discretizes edges, and generates vertex data for
 * the geometry render pass. Each entity gets a unique pick ID for GPU picking.
 */
class GeometryRenderBuilder {
public:
    /**
     * @brief Build render data from geometry entities.
     * @param render_data Output container for vertices and draw ranges.
     * @param input Geometry entities and tessellation options.
     * @return true on success.
     */
    static bool build(RenderData& render_data, const GeometryRenderInput& input);

private:
    static DrawRange generateFaceMesh(RenderData& rd,
                                      const Geometry::GeometryEntityImplPtr& entity,
                                      Geometry::EntityUID owner_part_uid,
                                      const TessellationOptions& opts);

    static DrawRange generateEdgeMesh(RenderData& rd,
                                      const Geometry::GeometryEntityImplPtr& entity,
                                      const TessellationOptions& opts);

    static DrawRange generateVertexMesh(RenderData& rd,
                                        const Geometry::GeometryEntityImplPtr& entity);
};

} // namespace OpenGeoLab::Render
