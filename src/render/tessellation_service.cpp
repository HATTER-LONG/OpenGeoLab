/**
 * @file tessellation_service.cpp
 * @brief Implementation of tessellation service for OCC geometry
 */

#include "render/tessellation_service.hpp"
#include "geometry/edge_entity.hpp"
#include "geometry/face_entity.hpp"
#include "geometry/part_entity.hpp"
#include "util/logger.hpp"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_Curve.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>

namespace OpenGeoLab::Render {

DocumentRenderDataPtr
TessellationService::tessellateDocument(const Geometry::GeometryDocumentPtr& document,
                                        const TessellationParams& params) {
    if(!document) {
        LOG_ERROR("TessellationService: Cannot tessellate null document");
        return nullptr;
    }

    auto result = std::make_shared<DocumentRenderData>();

    // Find all part entities in the document
    size_t part_index = 0;

    // Iterate through entities to find parts
    // Since EntityIndex doesn't expose iteration, we'll use a snapshot approach
    // For now, we tessellate the shapes directly from parts

    // Get parts by iterating types - this is a simplified approach
    // In production, we'd want EntityIndex to provide type-filtered iteration
    for(size_t uid = 1; uid <= document->entityCountByType(Geometry::EntityType::Part); ++uid) {
        auto entity = document->findByUIDAndType(uid, Geometry::EntityType::Part);
        if(!entity) {
            continue;
        }

        auto part_entity = std::dynamic_pointer_cast<Geometry::PartEntity>(entity);
        if(!part_entity) {
            continue;
        }

        auto part_render_data = tessellatePart(part_entity, part_index, params);
        if(part_render_data) {
            result->m_parts.push_back(std::move(part_render_data));
            ++part_index;
        }
    }

    result->updateSceneBoundingBox();

    LOG_INFO("TessellationService: Tessellated {} parts with {} total triangles",
             result->partCount(), result->totalTriangleCount());

    return result;
}

PartRenderDataPtr TessellationService::tessellatePart(const Geometry::PartEntityPtr& part_entity,
                                                      size_t part_index,
                                                      const TessellationParams& params) {
    if(!part_entity || !part_entity->hasShape()) {
        LOG_WARN("TessellationService: Cannot tessellate null or empty part");
        return nullptr;
    }

    auto result = std::make_shared<PartRenderData>();
    result->m_partEntityId = part_entity->entityId();
    result->m_partName = part_entity->name();
    result->m_baseColor = RenderColor::fromIndex(part_index);
    result->m_boundingBox = part_entity->boundingBox();

    const TopoDS_Shape& part_shape = part_entity->partShape();

    // Ensure triangulation exists
    ensureTriangulation(part_shape, params);

    // Extract faces
    for(TopExp_Explorer face_exp(part_shape, TopAbs_FACE); face_exp.More(); face_exp.Next()) {
        const TopoDS_Face& face = TopoDS::Face(face_exp.Current());

        RenderFace render_face = extractFaceTriangulation(face, result->m_baseColor);
        if(!render_face.m_vertices.empty()) {
            result->m_faces.push_back(std::move(render_face));
        }
    }

    // Extract edges for wireframe
    for(TopExp_Explorer edge_exp(part_shape, TopAbs_EDGE); edge_exp.More(); edge_exp.Next()) {
        const TopoDS_Edge& edge = TopoDS::Edge(edge_exp.Current());

        RenderEdge render_edge;
        render_edge.m_points = discretizeEdgeCurve(edge, params.m_linearDeflection);
        render_edge.m_color = RenderColor(0.1f, 0.1f, 0.1f); // Dark gray for edges

        if(!render_edge.m_points.empty()) {
            result->m_edges.push_back(std::move(render_edge));
        }
    }

    LOG_DEBUG("TessellationService: Part '{}' has {} faces, {} edges", result->m_partName,
              result->m_faces.size(), result->m_edges.size());

    return result;
}

RenderFace TessellationService::tessellateFace(const Geometry::FaceEntityPtr& face_entity,
                                               const RenderColor& color,
                                               const TessellationParams& params) {
    if(!face_entity || !face_entity->hasShape()) {
        LOG_WARN("TessellationService: Cannot tessellate null or empty face");
        return RenderFace();
    }

    const TopoDS_Face& face = face_entity->face();

    // Ensure triangulation
    BRepMesh_IncrementalMesh mesher(face, params.m_linearDeflection, params.m_relative,
                                    params.m_angularDeflection, Standard_True);

    RenderFace result = extractFaceTriangulation(face, color);
    result.m_entityId = face_entity->entityId();
    return result;
}

RenderEdge TessellationService::discretizeEdge(const Geometry::EdgeEntityPtr& edge_entity,
                                               const TessellationParams& params) {
    RenderEdge result;

    if(!edge_entity || !edge_entity->hasShape()) {
        LOG_WARN("TessellationService: Cannot discretize null or empty edge");
        return result;
    }

    result.m_entityId = edge_entity->entityId();
    result.m_points = discretizeEdgeCurve(edge_entity->edge(), params.m_linearDeflection);
    result.m_color = RenderColor(0.1f, 0.1f, 0.1f);

    return result;
}

void TessellationService::ensureTriangulation(const TopoDS_Shape& shape,
                                              const TessellationParams& params) {
    // Use BRepMesh_IncrementalMesh to ensure all faces have triangulation
    BRepMesh_IncrementalMesh mesher(shape, params.m_linearDeflection, params.m_relative,
                                    params.m_angularDeflection, Standard_True);

    if(!mesher.IsDone()) {
        LOG_WARN("TessellationService: Mesh generation may be incomplete");
    }
}

RenderFace TessellationService::extractFaceTriangulation(const TopoDS_Face& face,
                                                         const RenderColor& color) {
    RenderFace result;

    TopLoc_Location loc;
    Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, loc);

    if(triangulation.IsNull()) {
        LOG_WARN("TessellationService: Face has no triangulation");
        return result;
    }

    const gp_Trsf& trsf = loc.Transformation();
    const bool has_normals = triangulation->HasNormals();
    const Standard_Integer nb_nodes = triangulation->NbNodes();
    const Standard_Integer nb_triangles = triangulation->NbTriangles();

    // Reserve space
    result.m_vertices.reserve(static_cast<size_t>(nb_nodes));
    result.m_indices.reserve(static_cast<size_t>(nb_triangles) * 3);

    // Extract vertices and normals
    for(Standard_Integer i = 1; i <= nb_nodes; ++i) {
        gp_Pnt point = triangulation->Node(i);
        point.Transform(trsf);

        RenderVertex vertex;
        vertex.m_position[0] = static_cast<float>(point.X());
        vertex.m_position[1] = static_cast<float>(point.Y());
        vertex.m_position[2] = static_cast<float>(point.Z());

        if(has_normals) {
            gp_Dir normal = triangulation->Normal(i);
            normal.Transform(trsf);

            // Handle face orientation
            if(face.Orientation() == TopAbs_REVERSED) {
                normal.Reverse();
            }

            vertex.m_normal[0] = static_cast<float>(normal.X());
            vertex.m_normal[1] = static_cast<float>(normal.Y());
            vertex.m_normal[2] = static_cast<float>(normal.Z());
        }

        vertex.setColor(color);
        result.m_vertices.push_back(vertex);
    }

    // Extract triangle indices
    for(Standard_Integer i = 1; i <= nb_triangles; ++i) {
        Standard_Integer n1, n2, n3;
        triangulation->Triangle(i).Get(n1, n2, n3);

        // Handle face orientation for proper winding
        if(face.Orientation() == TopAbs_REVERSED) {
            std::swap(n2, n3);
        }

        // OCC uses 1-based indexing, OpenGL uses 0-based
        result.m_indices.push_back(static_cast<uint32_t>(n1 - 1));
        result.m_indices.push_back(static_cast<uint32_t>(n2 - 1));
        result.m_indices.push_back(static_cast<uint32_t>(n3 - 1));
    }

    return result;
}

std::vector<Geometry::Point3D> TessellationService::discretizeEdgeCurve(const TopoDS_Edge& edge,
                                                                        double deflection) {
    std::vector<Geometry::Point3D> points;

    if(BRep_Tool::Degenerated(edge)) {
        return points;
    }

    Standard_Real first, last;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);

    if(curve.IsNull()) {
        // Try to get points from 3D polygon if no curve
        TopLoc_Location loc;
        Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(edge, loc);
        if(!polygon.IsNull()) {
            const TColgp_Array1OfPnt& nodes = polygon->Nodes();
            const gp_Trsf& trsf = loc.Transformation();

            for(Standard_Integer i = nodes.Lower(); i <= nodes.Upper(); ++i) {
                gp_Pnt pt = nodes(i);
                pt.Transform(trsf);
                points.emplace_back(pt.X(), pt.Y(), pt.Z());
            }
        }
        return points;
    }

    try {
        // Use tangential deflection for adaptive discretization
        GCPnts_TangentialDeflection discretizer(GeomAdaptor_Curve(curve, first, last), deflection,
                                                0.1);

        if(discretizer.NbPoints() > 0) {
            points.reserve(static_cast<size_t>(discretizer.NbPoints()));
            for(Standard_Integer i = 1; i <= discretizer.NbPoints(); ++i) {
                const gp_Pnt& pt = discretizer.Value(i);
                points.emplace_back(pt.X(), pt.Y(), pt.Z());
            }
        }
    } catch(const Standard_Failure& e) {
        LOG_WARN("TessellationService: Edge discretization failed: {}",
                 e.GetMessageString() ? e.GetMessageString() : "Unknown error");
    }

    return points;
}

} // namespace OpenGeoLab::Render
