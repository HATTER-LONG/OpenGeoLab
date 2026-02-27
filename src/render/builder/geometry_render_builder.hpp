/**
 * @file geometry_render_builder.hpp
 * @brief Converts OCC geometry entities into GPU-ready render data (vertices, draw ranges).
 */

#pragma once

#include "render/render_data.hpp"

#include <memory>

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
    struct PartBuildContext {
        RenderData& m_renderData;
        const GeometryRenderInput& m_input;
        const Geometry::GeometryEntityImplPtr& m_part;
        Geometry::EntityUID m_partUid;
        const RenderColor& m_partColor;
        RenderNode& m_partNode;

        PartBuildContext(RenderData& render_data,
                         const GeometryRenderInput& input,
                         const Geometry::GeometryEntityImplPtr& part,
                         Geometry::EntityUID part_uid,
                         const RenderColor& part_color,
                         RenderNode& part_node)
            : m_renderData(render_data), m_input(input), m_part(part), m_partUid(part_uid),
              m_partColor(part_color), m_partNode(part_node) {}
    };

    static void appendFaceNodes(const PartBuildContext& context);

    static void mapFaceWireRelations(const PartBuildContext& context,
                                     const Geometry::GeometryEntityImplPtr& face_entity);

    static bool tryAppendFaceNode(const PartBuildContext& context,
                                  const Geometry::GeometryEntityImplPtr& face_entity);

    static void processFaceEntity(const PartBuildContext& context,
                                  const Geometry::GeometryEntityImplPtr& face_entity);

    static void appendFaceRenderNode(const PartBuildContext& context,
                                     const Geometry::GeometryEntityImplPtr& face_entity,
                                     const DrawRange& range);

    static void appendEdgeNodes(const PartBuildContext& context);

    static void appendVertexNodes(const PartBuildContext& context);

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
