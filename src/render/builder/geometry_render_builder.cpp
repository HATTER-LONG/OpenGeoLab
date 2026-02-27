/**
 * @file geometry_render_builder.cpp
 * @brief GeometryRenderBuilder — tessellates OCCT BRep shapes (faces, edges,
 *        vertices) into RenderData for the Geometry render pass, with per-entity
 *        pick IDs and part/wire hierarchy lookups.
 */

#include "geometry_render_builder.hpp"

#include "geometry/entity/entity_index.hpp"
#include "geometry/entity/geometry_entityImpl.hpp"
#include "geometry/entity/relationship_index.hpp"
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

namespace OpenGeoLab::Render {

namespace {

bool isIdentityTrsf(const gp_Trsf& trsf) {
    return trsf.IsNegative() == Standard_False && trsf.ScaleFactor() == 1.0 &&
           trsf.TranslationPart().SquareModulus() == 0.0;
}

} // namespace

bool GeometryRenderBuilder::build(RenderData& render_data, const GeometryRenderInput& input) {
    render_data.clearGeometry();

    const auto parts = input.m_entityIndex.entitiesByType(Geometry::EntityType::Part);
    if(parts.empty()) {
        LOG_DEBUG("GeometryRenderBuilder::build: No parts to render");
        return true;
    }

    // Run BRepMesh on each Part's shape for tessellation
    for(const auto& part : parts) {
        if(!part || !part->hasShape()) {
            continue;
        }
        BRepMesh_IncrementalMesh mesher(part->shape(), input.m_options.m_linearDeflection,
                                        Standard_False, input.m_options.m_angularDeflection,
                                        Standard_True);
        mesher.Perform();
    }

    const auto& color_map = Util::ColorMap::instance();

    for(const auto& part : parts) {
        if(!part || !part->hasShape()) {
            continue;
        }

        const Geometry::EntityUID part_uid = part->entityUID();
        const RenderColor part_color = color_map.getColorForPartId(part_uid);

        RenderNode part_node;
        part_node.m_key = {RenderEntityType::Part, part_uid};
        part_node.m_color = part_color;

        // Build edge-to-wire lookup for this Part
        std::unordered_map<Geometry::EntityUID, Geometry::EntityUID> edgeToWire;
        const auto wire_keys = input.m_relationshipIndex.findRelatedEntities(
            part->entityId(), Geometry::EntityType::Wire);
        for(const auto& wk : wire_keys) {
            auto wire_entity = input.m_entityIndex.findByKey(wk);
            if(!wire_entity) {
                continue;
            }
            const Geometry::EntityUID wire_uid = wire_entity->entityUID();
            // Find edges belonging to this wire
            const auto wire_edge_keys = input.m_relationshipIndex.findRelatedEntities(
                wire_entity->entityId(), Geometry::EntityType::Edge);
            for(const auto& ek : wire_edge_keys) {
                auto edge_entity = input.m_entityIndex.findByKey(ek);
                if(edge_entity) {
                    edgeToWire[edge_entity->entityUID()] = wire_uid;
                    // Edge may belong to multiple wires (shared edges between faces)
                    auto& wires = render_data.m_edgeToWireUids[edge_entity->entityUID()];
                    if(std::find(wires.begin(), wires.end(), wire_uid) == wires.end()) {
                        wires.push_back(wire_uid);
                    }
                    // Build reverse lookup: wire → all its edges
                    render_data.m_wireToEdgeUids[wire_uid].push_back(edge_entity->entityUID());
                }
            }
        }

        // --- Faces (triangles for Geometry pass) ---
        const auto face_keys = input.m_relationshipIndex.findRelatedEntities(
            part->entityId(), Geometry::EntityType::Face);
        for(const auto& fk : face_keys) {
            auto face_entity = input.m_entityIndex.findByKey(fk);
            if(!face_entity || !face_entity->hasShape()) {
                continue;
            }

            // Build wire → face mapping: find wires that bound this face
            const auto face_wire_keys = input.m_relationshipIndex.findRelatedEntities(
                face_entity->entityId(), Geometry::EntityType::Wire);
            for(const auto& wk : face_wire_keys) {
                auto wire_entity = input.m_entityIndex.findByKey(wk);
                if(wire_entity) {
                    render_data.m_wireToFaceUid[wire_entity->entityUID()] =
                        face_entity->entityUID();
                }
            }

            DrawRange range = generateFaceMesh(render_data, face_entity, part_uid, input.m_options);
            if(range.m_indexCount == 0 && range.m_vertexCount == 0) {
                continue;
            }

            RenderNode face_node;
            face_node.m_key = {RenderEntityType::Face, face_entity->entityUID()};
            face_node.m_color = part_color;
            face_node.m_bbox = face_entity->boundingBox();
            face_node.m_drawRanges[RenderPassType::Geometry].push_back(range);

            part_node.m_bbox.expand(face_node.m_bbox);
            part_node.m_children.push_back(std::move(face_node));
        }

        // --- Edges (lines for Geometry pass) ---
        const auto edge_keys = input.m_relationshipIndex.findRelatedEntities(
            part->entityId(), Geometry::EntityType::Edge);
        for(const auto& ek : edge_keys) {
            auto edge_entity = input.m_entityIndex.findByKey(ek);
            if(!edge_entity || !edge_entity->hasShape()) {
                continue;
            }

            DrawRange range = generateEdgeMesh(render_data, edge_entity, input.m_options);
            if(range.m_vertexCount == 0) {
                continue;
            }

            RenderNode edge_node;
            edge_node.m_key = {RenderEntityType::Edge, edge_entity->entityUID()};
            edge_node.m_color = color_map.getEdgeColor();
            edge_node.m_bbox = edge_entity->boundingBox();
            edge_node.m_drawRanges[RenderPassType::Geometry].push_back(range);

            part_node.m_bbox.expand(edge_node.m_bbox);
            part_node.m_children.push_back(std::move(edge_node));
        }

        // --- Vertices (points for Geometry pass) ---
        const auto vertex_keys = input.m_relationshipIndex.findRelatedEntities(
            part->entityId(), Geometry::EntityType::Vertex);
        for(const auto& vk : vertex_keys) {
            auto vertex_entity = input.m_entityIndex.findByKey(vk);
            if(!vertex_entity || !vertex_entity->hasShape()) {
                continue;
            }

            DrawRange range = generateVertexMesh(render_data, vertex_entity);
            if(range.m_vertexCount == 0) {
                continue;
            }

            RenderNode vertex_node;
            vertex_node.m_key = {RenderEntityType::Vertex, vertex_entity->entityUID()};
            vertex_node.m_color = color_map.getVertexColor();
            vertex_node.m_bbox = vertex_entity->boundingBox();
            vertex_node.m_drawRanges[RenderPassType::Geometry].push_back(range);

            part_node.m_bbox.expand(vertex_node.m_bbox);
            part_node.m_children.push_back(std::move(vertex_node));
        }

        render_data.m_sceneBBox.expand(part_node.m_bbox);
        render_data.m_roots.push_back(std::move(part_node));
    }

    render_data.m_geometryDirty = true;

    LOG_DEBUG("GeometryRenderBuilder::build: {} roots, geom vertices={}, indices={}",
              render_data.m_roots.size(),
              render_data.m_passData.count(RenderPassType::Geometry)
                  ? render_data.m_passData[RenderPassType::Geometry].m_vertices.size()
                  : 0,
              render_data.m_passData.count(RenderPassType::Geometry)
                  ? render_data.m_passData[RenderPassType::Geometry].m_indices.size()
                  : 0);
    return true;
}

DrawRange GeometryRenderBuilder::generateFaceMesh(RenderData& render_data,
                                                  const Geometry::GeometryEntityImplPtr& entity,
                                                  Geometry::EntityUID owner_part_uid,
                                                  const TessellationOptions& options) {
    DrawRange result;
    if(!entity || !entity->hasShape()) {
        return result;
    }

    const auto face = TopoDS::Face(entity->shape());
    TopLoc_Location location;
    const auto triangulation = BRep_Tool::Triangulation(face, location);
    if(triangulation.IsNull() || triangulation->NbTriangles() == 0) {
        return result;
    }

    auto& pass_data = render_data.m_passData[RenderPassType::Geometry];
    const auto base_vertex = static_cast<uint32_t>(pass_data.m_vertices.size());
    const auto base_index = static_cast<uint32_t>(pass_data.m_indices.size());

    const auto& color_map = Util::ColorMap::instance();
    const RenderColor face_color = color_map.getColorForPartId(owner_part_uid);
    const uint64_t pick_id = PickId::encode(RenderEntityType::Face, entity->entityUID());

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

        RenderVertex v{};
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
    result.m_topology = PrimitiveTopology::Triangles;

    pass_data.m_dirty = true;
    return result;
}

DrawRange GeometryRenderBuilder::generateEdgeMesh(RenderData& render_data,
                                                  const Geometry::GeometryEntityImplPtr& entity,
                                                  const TessellationOptions& options) {
    DrawRange result;
    if(!entity || !entity->hasShape()) {
        return result;
    }

    const auto edge = TopoDS::Edge(entity->shape());
    if(BRep_Tool::Degenerated(edge)) {
        return result;
    }

    auto& pass_data = render_data.m_passData[RenderPassType::Geometry];
    const auto base_vertex = static_cast<uint32_t>(pass_data.m_vertices.size());

    const auto& color_map = Util::ColorMap::instance();
    const RenderColor edge_color = color_map.getEdgeColor();
    const uint64_t pick_id = PickId::encode(RenderEntityType::Edge, entity->entityUID());

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

            RenderVertex v{};
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

            RenderVertex v{};
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
    result.m_topology = PrimitiveTopology::Lines;

    pass_data.m_dirty = true;
    return result;
}

DrawRange GeometryRenderBuilder::generateVertexMesh(RenderData& render_data,
                                                    const Geometry::GeometryEntityImplPtr& entity) {
    DrawRange result;
    if(!entity || !entity->hasShape()) {
        return result;
    }

    const auto vertex = TopoDS::Vertex(entity->shape());
    const gp_Pnt p = BRep_Tool::Pnt(vertex);

    auto& pass_data = render_data.m_passData[RenderPassType::Geometry];
    const auto base_vertex = static_cast<uint32_t>(pass_data.m_vertices.size());

    const auto& color_map = Util::ColorMap::instance();
    const RenderColor vtx_color = color_map.getVertexColor();
    const uint64_t pick_id = PickId::encode(RenderEntityType::Vertex, entity->entityUID());

    RenderVertex v{};
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
    result.m_topology = PrimitiveTopology::Points;

    pass_data.m_dirty = true;
    return result;
}

} // namespace OpenGeoLab::Render
