#include "geometry_render_builder.hpp"
#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_UniformDeflection.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_Triangle.hxx>
#include <Poly_Triangulation.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>

#include <algorithm>

namespace OpenGeoLab::Geometry {
namespace {
void appendRange(std::vector<Render::DrawRange>& ranges, Render::DrawRange&& range) {
    ranges.push_back(std::move(range));
}

bool isIdentityTrsf(const gp_Trsf& trsf) {
    return trsf.IsNegative() == Standard_False && trsf.ScaleFactor() == 1.0 &&
           trsf.TranslationPart().SquareModulus() == 0.0;
}

bool isRenderableEntity(const Geometry::GeometryEntityImplPtr& entity) {
    return entity && entity->hasShape();
}

void tessellatePartShape(const Geometry::GeometryEntityImplPtr& part,
                         const Render::TessellationOptions& options) {
    if(!isRenderableEntity(part)) {
        return;
    }

    BRepMesh_IncrementalMesh mesher(part->shape(), options.m_linearDeflection, Standard_False,
                                    options.m_angularDeflection, Standard_True);
    mesher.Perform();
}

void buildWireEdgeLookupsForPart(Render::RenderData& render_data,
                                 const GeometryRenderInput& input,
                                 const Geometry::GeometryEntityImplPtr& part) {
    const auto wire_keys =
        input.m_relationshipIndex.findRelatedEntities(part->entityId(), Geometry::EntityType::Wire);
    for(const auto& wk : wire_keys) {
        auto wire_entity = input.m_entityIndex.findByKey(wk);
        if(!wire_entity) {
            continue;
        }

        const EntityUID wire_uid = wire_entity->entityUID();
        const auto wire_edge_keys = input.m_relationshipIndex.findRelatedEntities(
            wire_entity->entityId(), Geometry::EntityType::Edge);
        for(const auto& ek : wire_edge_keys) {
            auto edge_entity = input.m_entityIndex.findByKey(ek);
            if(!edge_entity) {
                continue;
            }
            const EntityUID edge_uid = edge_entity->entityUID();
            auto& edge_wires = render_data.m_geometry.m_pickData.m_edgeToWireUids[edge_uid];
            if(std::find(edge_wires.begin(), edge_wires.end(), wire_uid) == edge_wires.end()) {
                edge_wires.push_back(wire_uid);
            }
            render_data.m_geometry.m_pickData.m_wireToEdgeUids[wire_uid].push_back(edge_uid);
        }
    }
}

std::vector<EntityUID> findOwningSolidUids(const GeometryRenderInput& input,
                                           const Geometry::GeometryEntityImplPtr& entity) {
    std::vector<EntityUID> solid_uids;
    if(!entity) {
        return solid_uids;
    }

    const auto solid_keys = input.m_relationshipIndex.findRelatedEntities(
        entity->entityId(), Geometry::EntityType::Solid);
    solid_uids.reserve(solid_keys.size());
    for(const auto& solid_key : solid_keys) {
        if(solid_key.m_uid == Geometry::INVALID_ENTITY_UID) {
            continue;
        }
        if(std::find(solid_uids.begin(), solid_uids.end(), solid_key.m_uid) == solid_uids.end()) {
            solid_uids.push_back(solid_key.m_uid);
        }
    }
    return solid_uids;
}

EntityUID firstOwningSolidUid(const GeometryRenderInput& input,
                              const Geometry::GeometryEntityImplPtr& entity) {
    const auto solid_uids = findOwningSolidUids(input, entity);
    return solid_uids.empty() ? Geometry::INVALID_ENTITY_UID : solid_uids.front();
}

} // namespace

bool GeometryRenderBuilder::build(Render::RenderData& render_data,
                                  const GeometryRenderInput& input) {
    render_data.clearGeometry();
    auto& geometry = render_data.m_geometry;
    const auto parts = input.m_entityIndex.entitiesByType(Geometry::EntityType::Part);
    if(parts.empty()) {
        LOG_DEBUG("GeometryRenderBuilder::build: No parts to render");
        geometry.markUpdated();
        return true;
    }

    for(const auto& part : parts) {
        tessellatePartShape(part, input.m_options);
    }

    for(const auto& part : parts) {
        if(!isRenderableEntity(part)) {
            continue;
        }
        const EntityUID part_uid = part->entityUID();

        PartBuildContext context(render_data, input, part, part_uid);
        buildWireEdgeLookupsForPart(render_data, input, part);
        appendFaceNodes(context);
        appendEdgeNodes(context);
        appendVertexNodes(context);

        geometry.m_bbox.expand(part->boundingBox());
    }
    geometry.markUpdated();

    LOG_DEBUG("GeometryRenderBuilder::build: geom tris={}, lines={}, points={}, vertices={}, "
              "indices={}",
              geometry.m_triangleRanges.size(), geometry.m_lineRanges.size(),
              geometry.m_pointRanges.size(), geometry.m_passData.m_vertices.size(),
              geometry.m_passData.m_indices.size());
    return true;
}
void GeometryRenderBuilder::appendFaceNodes(const PartBuildContext& context) {
    const auto face_keys = context.m_input.m_relationshipIndex.findRelatedEntities(
        context.m_part->entityId(), Geometry::EntityType::Face);
    for(const auto& fk : face_keys) {
        auto face_entity = context.m_input.m_entityIndex.findByKey(fk);
        if(!isRenderableEntity(face_entity)) {
            continue;
        }
        processFaceEntity(context, face_entity);
    }
}

void GeometryRenderBuilder::appendEdgeNodes(const PartBuildContext& context) {
    const auto edge_keys = context.m_input.m_relationshipIndex.findRelatedEntities(
        context.m_part->entityId(), Geometry::EntityType::Edge);
    for(const auto& ek : edge_keys) {
        auto edge_entity = context.m_input.m_entityIndex.findByKey(ek);
        if(!isRenderableEntity(edge_entity)) {
            continue;
        }

        Render::DrawRange range = generateEdgeMesh(context.m_renderData, edge_entity,
                                                   context.m_partUid, context.m_input.m_options);
        if(range.m_vertexCount == 0) {
            continue;
        }
        const auto solid_uids = findOwningSolidUids(context.m_input, edge_entity);
        range.m_solidUid = solid_uids.empty() ? 0 : solid_uids.front();
        context.m_renderData.m_geometry.m_pickData.m_entityToPartUid[Render::PickId::encode(
            Render::RenderEntityType::Edge, edge_entity->entityUID())] = context.m_partUid;
        if(!solid_uids.empty()) {
            auto& edge_solids = context.m_renderData.m_geometry.m_pickData
                                    .m_edgeToSolidUids[edge_entity->entityUID()];
            for(const auto solid_uid : solid_uids) {
                if(std::find(edge_solids.begin(), edge_solids.end(), solid_uid) ==
                   edge_solids.end()) {
                    edge_solids.push_back(solid_uid);
                }
            }
        }
        appendRange(context.m_renderData.m_geometry.m_lineRanges, std::move(range));
    }
}

void GeometryRenderBuilder::appendVertexNodes(const PartBuildContext& context) {
    const auto vertex_keys = context.m_input.m_relationshipIndex.findRelatedEntities(
        context.m_part->entityId(), Geometry::EntityType::Vertex);
    for(const auto& vk : vertex_keys) {
        auto vertex_entity = context.m_input.m_entityIndex.findByKey(vk);
        if(!isRenderableEntity(vertex_entity)) {
            continue;
        }

        Render::DrawRange range =
            generateVertexMesh(context.m_renderData, vertex_entity, context.m_partUid);
        if(range.m_vertexCount == 0) {
            continue;
        }
        // TODO(layton) - combine with edge/face part lookup or unify into single entity-level
        // lookup

        // context.m_renderData.m_pickData.m_entityToPartUid[vertex_entity->entityUID()] =
        //     context.m_partUid;
        appendRange(context.m_renderData.m_geometry.m_pointRanges, std::move(range));
    }
}

void GeometryRenderBuilder::processFaceEntity(const PartBuildContext& context,
                                              const Geometry::GeometryEntityImplPtr& face_entity) {
    mapFaceWireRelations(context, face_entity);
    tryAppendFaceNode(context, face_entity);
}

void GeometryRenderBuilder::mapFaceWireRelations(
    const PartBuildContext& context, const Geometry::GeometryEntityImplPtr& face_entity) {
    const auto face_wire_keys = context.m_input.m_relationshipIndex.findRelatedEntities(
        face_entity->entityId(), Geometry::EntityType::Wire);
    for(const auto& wk : face_wire_keys) {
        auto wire_entity = context.m_input.m_entityIndex.findByKey(wk);
        if(wire_entity) {
            context.m_renderData.m_geometry.m_pickData.m_wireToFaceUid[wire_entity->entityUID()] =
                face_entity->entityUID();
        }
    }
}

bool GeometryRenderBuilder::tryAppendFaceNode(const PartBuildContext& context,
                                              const Geometry::GeometryEntityImplPtr& face_entity) {
    auto range = generateFaceMesh(context.m_renderData, face_entity, context.m_partUid,
                                  context.m_input.m_options);
    if(range.m_indexCount == 0 && range.m_vertexCount == 0) {
        return false;
    }
    const auto solid_uid = firstOwningSolidUid(context.m_input, face_entity);
    if(solid_uid != Geometry::INVALID_ENTITY_UID) {
        context.m_renderData.m_geometry.m_pickData.m_faceToSolidUid[face_entity->entityUID()] =
            solid_uid;
        range.m_solidUid = solid_uid;
    }
    context.m_renderData.m_geometry.m_pickData.m_entityToPartUid[Render::PickId::encode(
        Render::RenderEntityType::Face, face_entity->entityUID())] = context.m_partUid;
    appendRange(context.m_renderData.m_geometry.m_triangleRanges, std::move(range));
    return true;
}

Render::DrawRange
GeometryRenderBuilder::generateFaceMesh(Render::RenderData& render_data,
                                        const Geometry::GeometryEntityImplPtr& entity,
                                        Geometry::EntityUID owner_part_uid,
                                        const Render::TessellationOptions& options) {
    Render::DrawRange result;
    if(!entity || !entity->hasShape()) {
        return result;
    }

    const auto face = TopoDS::Face(entity->shape());
    TopLoc_Location location;
    const auto triangulation = BRep_Tool::Triangulation(face, location);
    if(triangulation.IsNull() || triangulation->NbTriangles() == 0) {
        return result;
    }

    auto& pass_data = render_data.m_geometry.m_passData;
    const auto base_vertex = static_cast<uint32_t>(pass_data.m_vertices.size());
    const auto base_index = static_cast<uint32_t>(pass_data.m_indices.size());

    const auto& color_map = Util::ColorMap::instance();
    const Render::RenderColor face_color = color_map.getColorForPartId(owner_part_uid);
    const uint64_t pick_id =
        Render::PickId::encode(Render::RenderEntityType::Face, entity->entityUID());

    const bool reversed = (face.Orientation() == TopAbs_REVERSED);
    const gp_Trsf trsf = location.Transformation();
    const bool has_trsf = !isIdentityTrsf(trsf);

    const int nb_nodes = triangulation->NbNodes();
    pass_data.m_vertices.reserve(pass_data.m_vertices.size() + static_cast<size_t>(nb_nodes));

    for(int i = 1; i <= nb_nodes; ++i) {
        gp_Pnt p = triangulation->Node(i);
        if(has_trsf) {
            p.Transform(trsf);
        }

        Render::RenderVertex v{};
        v.m_position[0] = static_cast<float>(p.X());
        v.m_position[1] = static_cast<float>(p.Y());
        v.m_position[2] = static_cast<float>(p.Z());

        if(options.m_computeNormals && triangulation->HasNormals()) {
            gp_Dir n = triangulation->Normal(i);
            if(has_trsf) {
                n.Transform(trsf);
            }
            if(reversed) {
                n.Reverse();
            }
            v.m_normal[0] = static_cast<float>(n.X());
            v.m_normal[1] = static_cast<float>(n.Y());
            v.m_normal[2] = static_cast<float>(n.Z());
        }

        v.m_color[0] = face_color.m_r;
        v.m_color[1] = face_color.m_g;
        v.m_color[2] = face_color.m_b;
        v.m_color[3] = face_color.m_a;
        v.m_pickId = pick_id;

        pass_data.m_vertices.push_back(v);
    }

    const int nb_triangles = triangulation->NbTriangles();
    pass_data.m_indices.reserve(pass_data.m_indices.size() + static_cast<size_t>(nb_triangles) * 3);

    for(int i = 1; i <= nb_triangles; ++i) {
        const Poly_Triangle& tri = triangulation->Triangle(i);
        int n1 = 0;
        int n2 = 0;
        int n3 = 0;
        tri.Get(n1, n2, n3);
        if(reversed) {
            std::swap(n2, n3);
        }
        pass_data.m_indices.push_back(base_vertex + static_cast<uint32_t>(n1 - 1));
        pass_data.m_indices.push_back(base_vertex + static_cast<uint32_t>(n2 - 1));
        pass_data.m_indices.push_back(base_vertex + static_cast<uint32_t>(n3 - 1));
    }

    result.m_vertexOffset = base_vertex;
    result.m_vertexCount = static_cast<uint32_t>(nb_nodes);
    result.m_indexOffset = base_index;
    result.m_indexCount = static_cast<uint32_t>(nb_triangles) * 3;
    result.m_topology = Render::PrimitiveTopology::Triangles;
    result.m_entityKey = {Render::RenderEntityType::Face, entity->entityUID()};
    result.m_partUid = owner_part_uid;

    pass_data.markDataUpdated();
    return result;
}

Render::DrawRange
GeometryRenderBuilder::generateEdgeMesh(Render::RenderData& render_data,
                                        const Geometry::GeometryEntityImplPtr& entity,
                                        Geometry::EntityUID owner_part_uid,
                                        const Render::TessellationOptions& options) {
    Render::DrawRange result;
    if(!entity || !entity->hasShape()) {
        return result;
    }

    const auto edge = TopoDS::Edge(entity->shape());
    if(BRep_Tool::Degenerated(edge)) {
        return result;
    }

    auto& pass_data = render_data.m_geometry.m_passData;
    const auto base_vertex = static_cast<uint32_t>(pass_data.m_vertices.size());

    const auto& color_map = Util::ColorMap::instance();
    const Render::RenderColor edge_color = color_map.getEdgeColor();
    const uint64_t pick_id =
        Render::PickId::encode(Render::RenderEntityType::Edge, entity->entityUID());

    TopLoc_Location location;
    const auto polygon = BRep_Tool::Polygon3D(edge, location);

    // Prefer the pre-computed polygon3D from BRepMesh; fall back to
    // adaptive curve discretization when polygon data is unavailable.
    if(!polygon.IsNull() && polygon->NbNodes() >= 2) {
        const auto& nodes = polygon->Nodes();
        const gp_Trsf trsf = location.Transformation();
        const bool has_trsf = !location.IsIdentity();

        for(int i = 1; i <= nodes.Length(); ++i) {
            gp_Pnt p = nodes.Value(i);
            if(has_trsf) {
                p.Transform(trsf);
            }

            Render::RenderVertex v{};
            v.m_position[0] = static_cast<float>(p.X());
            v.m_position[1] = static_cast<float>(p.Y());
            v.m_position[2] = static_cast<float>(p.Z());
            v.m_color[0] = edge_color.m_r;
            v.m_color[1] = edge_color.m_g;
            v.m_color[2] = edge_color.m_b;
            v.m_color[3] = edge_color.m_a;
            v.m_pickId = pick_id;
            pass_data.m_vertices.push_back(v);
        }
    } else {
        double first = 0.0;
        double last = 0.0;
        const auto curve = BRep_Tool::Curve(edge, first, last);
        if(curve.IsNull()) {
            return result;
        }

        GeomAdaptor_Curve adaptor(curve, first, last);
        GCPnts_UniformDeflection discretizer(adaptor, options.m_linearDeflection);
        if(!discretizer.IsDone() || discretizer.NbPoints() < 2) {
            return result;
        }

        for(int i = 1; i <= discretizer.NbPoints(); ++i) {
            const gp_Pnt p = discretizer.Value(i);

            Render::RenderVertex v{};
            v.m_position[0] = static_cast<float>(p.X());
            v.m_position[1] = static_cast<float>(p.Y());
            v.m_position[2] = static_cast<float>(p.Z());
            v.m_color[0] = edge_color.m_r;
            v.m_color[1] = edge_color.m_g;
            v.m_color[2] = edge_color.m_b;
            v.m_color[3] = edge_color.m_a;
            v.m_pickId = pick_id;
            pass_data.m_vertices.push_back(v);
        }
    }

    const auto vertex_count = static_cast<uint32_t>(pass_data.m_vertices.size()) - base_vertex;
    if(vertex_count < 2) {
        return result;
    }

    const auto base_index = static_cast<uint32_t>(pass_data.m_indices.size());
    pass_data.m_indices.reserve(pass_data.m_indices.size() +
                                static_cast<size_t>(vertex_count - 1) * 2);

    for(uint32_t i = 0; i + 1 < vertex_count; ++i) {
        pass_data.m_indices.push_back(base_vertex + i);
        pass_data.m_indices.push_back(base_vertex + i + 1);
    }

    result.m_vertexOffset = base_vertex;
    result.m_vertexCount = vertex_count;
    result.m_indexOffset = base_index;
    result.m_indexCount = (vertex_count - 1) * 2;
    result.m_topology = Render::PrimitiveTopology::Lines;
    result.m_entityKey = {Render::RenderEntityType::Edge, entity->entityUID()};
    result.m_partUid = owner_part_uid;

    auto wire_it = render_data.m_geometry.m_pickData.m_edgeToWireUids.find(entity->entityUID());
    if(wire_it != render_data.m_geometry.m_pickData.m_edgeToWireUids.end() &&
       !wire_it->second.empty()) {
        result.m_wireUid = wire_it->second.front();
    }

    pass_data.markDataUpdated();
    return result;
}

Render::DrawRange
GeometryRenderBuilder::generateVertexMesh(Render::RenderData& render_data,
                                          const Geometry::GeometryEntityImplPtr& entity,
                                          Geometry::EntityUID owner_part_uid) {
    Render::DrawRange result;
    if(!entity || !entity->hasShape()) {
        return result;
    }
    const auto vertex = TopoDS::Vertex(entity->shape());
    const gp_Pnt p = BRep_Tool::Pnt(vertex);

    auto& pass_data = render_data.m_geometry.m_passData;
    const auto base_vertex = static_cast<uint32_t>(pass_data.m_vertices.size());

    const auto& color_map = Util::ColorMap::instance();
    const Render::RenderColor vtx_color = color_map.getVertexColor();
    const uint64_t pick_id =
        Render::PickId::encode(Render::RenderEntityType::Vertex, entity->entityUID());
    Render::RenderVertex v{};
    v.m_position[0] = static_cast<float>(p.X());
    v.m_position[1] = static_cast<float>(p.Y());
    v.m_position[2] = static_cast<float>(p.Z());
    v.m_color[0] = vtx_color.m_r;
    v.m_color[1] = vtx_color.m_g;
    v.m_color[2] = vtx_color.m_b;
    v.m_color[3] = vtx_color.m_a;
    v.m_pickId = pick_id;

    pass_data.m_vertices.push_back(v);

    result.m_vertexOffset = base_vertex;
    result.m_vertexCount = 1;
    result.m_indexOffset = 0;
    result.m_indexCount = 0;
    result.m_topology = Render::PrimitiveTopology::Points;
    result.m_entityKey = {Render::RenderEntityType::Vertex, entity->entityUID()};
    result.m_partUid = owner_part_uid;

    pass_data.markDataUpdated();
    return result;
}
} // namespace OpenGeoLab::Geometry