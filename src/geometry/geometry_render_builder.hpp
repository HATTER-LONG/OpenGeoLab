#pragma once

#include "entity/entity_index.hpp"
#include "entity/relationship_index.hpp"
#include "geometry/geometry_types.hpp"

#include "render/render_data.hpp"

namespace OpenGeoLab::Geometry {
struct GeometryRenderInput {
    const Geometry::EntityIndex& m_entityIndex;
    const Geometry::EntityRelationshipIndex& m_relationshipIndex;
    Render::TessellationOptions m_options;
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
    static bool build(Render::RenderData& render_data, const GeometryRenderInput& input);

private:
    struct PartBuildContext {
        Render::RenderData& m_renderData;
        const GeometryRenderInput& m_input;
        const Geometry::GeometryEntityImplPtr& m_part;
        Geometry::EntityUID m_partUid;
        const Render::RenderColor& m_partColor;
        Render::RenderNode& m_partNode;

        PartBuildContext(Render::RenderData& render_data,
                         const GeometryRenderInput& input,
                         const Geometry::GeometryEntityImplPtr& part,
                         Geometry::EntityUID part_uid,
                         const Render::RenderColor& part_color,
                         Render::RenderNode& part_node)
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
                                     const Render::DrawRange& range);

    static void appendEdgeNodes(const PartBuildContext& context);

    static void appendVertexNodes(const PartBuildContext& context);

    static Render::DrawRange generateFaceMesh(Render::RenderData& rd,
                                              const Geometry::GeometryEntityImplPtr& entity,
                                              Geometry::EntityUID owner_part_uid,
                                              const Render::TessellationOptions& opts);

    static Render::DrawRange generateEdgeMesh(Render::RenderData& rd,
                                              const Geometry::GeometryEntityImplPtr& entity,
                                              const Render::TessellationOptions& opts);

    static Render::DrawRange generateVertexMesh(Render::RenderData& rd,
                                                const Geometry::GeometryEntityImplPtr& entity);
};
} // namespace OpenGeoLab::Geometry