/**
 * @file occ_converter.cpp
 * @brief Implementation of OpenCASCADE geometry converter.
 */
#include "geometry/occ_converter.hpp"
#include "util/logger.hpp"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>

#include <TopLoc_Location.hxx>

#include <unordered_map>

namespace OpenGeoLab {
namespace Geometry {

GeometryModelPtr OccConverter::convertShape(const TopoDS_Shape& shape,
                                            const std::string& part_name,
                                            const TessellationParams& params) {
    auto model = std::make_shared<GeometryModel>();

    if(!addShapeToModel(shape, part_name, *model, params)) {
        LOG_ERROR("OccConverter: Failed to convert shape");
        return nullptr;
    }

    return model;
}

bool OccConverter::addShapeToModel(const TopoDS_Shape& shape,
                                   const std::string& part_name,
                                   GeometryModel& model,
                                   const TessellationParams& params) {
    if(shape.IsNull()) {
        LOG_ERROR("OccConverter: Input shape is null");
        return false;
    }

    // Tessellate shape for visualization
    if(!tessellateShape(shape, params)) {
        LOG_ERROR("OccConverter: Tessellation failed");
        return false;
    }

    // Extract geometry data
    extractGeometry(shape, part_name, model);

    LOG_INFO("OccConverter: Converted shape '{}' - {}", part_name, model.getSummary());
    return true;
}

bool OccConverter::tessellateShape(const TopoDS_Shape& shape, const TessellationParams& params) {
    try {
        BRepMesh_IncrementalMesh mesher(shape, params.linearDeflection, params.relative,
                                        params.angularDeflection);
        mesher.Perform();

        if(!mesher.IsDone()) {
            LOG_ERROR("OccConverter: BRepMesh_IncrementalMesh failed");
            return false;
        }

        LOG_DEBUG("OccConverter: Tessellation completed");
        return true;
    } catch(const Standard_Failure& e) {
        LOG_ERROR("OccConverter: OCC exception during tessellation: {}", e.GetMessageString());
        return false;
    }
}

void OccConverter::extractGeometry(const TopoDS_Shape& shape, // NOLINT
                                   const std::string& part_name,
                                   GeometryModel& model) {
    // Maps to track OCC shape to our ID mapping
    std::unordered_map<size_t, uint32_t> vertex_map;
    std::unordered_map<size_t, uint32_t> edge_map;
    std::unordered_map<size_t, uint32_t> face_map;
    std::unordered_map<size_t, uint32_t> solid_map;

    // Create the part
    Part part;
    part.m_id = model.generateNextId();
    part.m_name = part_name;

    // Extract solids
    for(TopExp_Explorer solid_exp(shape, TopAbs_SOLID); solid_exp.More(); solid_exp.Next()) {
        const TopoDS_Solid& occ_solid = TopoDS::Solid(solid_exp.Current());
        size_t solid_hash = std::hash<TopoDS_Solid>{}(occ_solid);

        if(solid_map.find(solid_hash) != solid_map.end()) {
            continue; // Already processed
        }

        Solid solid;
        solid.m_id = model.generateNextId();
        solid_map[solid_hash] = solid.m_id;
        part.m_solidIds.push_back(solid.m_id);

        // Extract faces for this solid
        for(TopExp_Explorer face_exp(occ_solid, TopAbs_FACE); face_exp.More(); face_exp.Next()) {
            const TopoDS_Face& occ_face = TopoDS::Face(face_exp.Current());
            size_t face_hash = std::hash<TopoDS_Face>{}(occ_face);

            uint32_t face_id;
            if(face_map.find(face_hash) != face_map.end()) {
                face_id = face_map[face_hash];
            } else {
                Face face;
                face.m_id = model.generateNextId();
                face_id = face.m_id;
                face_map[face_hash] = face_id;

                // Get tessellation
                TopLoc_Location loc;
                Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(occ_face, loc);

                if(!triangulation.IsNull()) {
                    const gp_Trsf& trsf = loc.Transformation();
                    int num_nodes = triangulation->NbNodes();
                    int num_tris = triangulation->NbTriangles();

                    // Extract vertices with normals
                    face.m_meshVertices.reserve(num_nodes);
                    for(int i = 1; i <= num_nodes; ++i) {
                        gp_Pnt pt = triangulation->Node(i).Transformed(trsf);

                        RenderVertex rv;
                        rv.m_position = Point3D(pt.X(), pt.Y(), pt.Z());

                        // Get normal if available
                        if(triangulation->HasNormals()) {
                            gp_Dir normal = triangulation->Normal(i);
                            rv.m_normal = Point3D(normal.X(), normal.Y(), normal.Z());
                        }

                        face.m_meshVertices.push_back(rv);
                    }

                    // Extract triangle indices
                    face.m_meshIndices.reserve(num_tris * 3);
                    for(int i = 1; i <= num_tris; ++i) {
                        const Poly_Triangle& tri = triangulation->Triangle(i);
                        int n1, n2, n3;
                        tri.Get(n1, n2, n3);

                        // Convert to 0-based indices
                        face.m_meshIndices.push_back(static_cast<uint32_t>(n1 - 1));
                        face.m_meshIndices.push_back(static_cast<uint32_t>(n2 - 1));
                        face.m_meshIndices.push_back(static_cast<uint32_t>(n3 - 1));
                    }
                }

                // Extract edges for this face
                for(TopExp_Explorer edge_exp(occ_face, TopAbs_EDGE); edge_exp.More();
                    edge_exp.Next()) {
                    const TopoDS_Edge& occ_edge = TopoDS::Edge(edge_exp.Current());
                    size_t edge_hash = std::hash<TopoDS_Edge>{}(occ_edge);

                    uint32_t edge_id;
                    if(edge_map.find(edge_hash) != edge_map.end()) {
                        edge_id = edge_map[edge_hash];
                    } else {
                        Edge edge;
                        edge.m_id = model.generateNextId();
                        edge_id = edge.m_id;
                        edge_map[edge_hash] = edge_id;

                        // Get edge vertices
                        TopoDS_Vertex v1, v2;
                        TopExp_Explorer vert_exp(occ_edge, TopAbs_VERTEX);
                        if(vert_exp.More()) {
                            v1 = TopoDS::Vertex(vert_exp.Current());
                            vert_exp.Next();
                            if(vert_exp.More()) {
                                v2 = TopoDS::Vertex(vert_exp.Current());
                            }
                        }

                        // Get or create vertex IDs
                        auto get_vertex_id = [&](const TopoDS_Vertex& v) -> uint32_t {
                            if(v.IsNull()) {
                                return 0;
                            }

                            size_t v_hash = std::hash<TopoDS_Vertex>{}(v);
                            auto it = vertex_map.find(v_hash);
                            if(it != vertex_map.end()) {
                                return it->second;
                            }

                            gp_Pnt pt = BRep_Tool::Pnt(v);
                            Vertex vert;
                            vert.m_id = model.generateNextId();
                            vert.m_position = Point3D(pt.X(), pt.Y(), pt.Z());
                            vertex_map[v_hash] = vert.m_id;
                            model.addVertex(vert);
                            return vert.m_id;
                        };

                        edge.m_startVertexId = get_vertex_id(v1);
                        edge.m_endVertexId = get_vertex_id(v2);

                        // Get curve points for visualization
                        TopLoc_Location edge_loc;
                        Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(occ_edge, edge_loc);
                        if(!polygon.IsNull()) {
                            const TColgp_Array1OfPnt& nodes = polygon->Nodes();
                            const gp_Trsf& edge_trsf = edge_loc.Transformation();
                            edge.m_curvePoints.reserve(nodes.Length());
                            for(int i = nodes.Lower(); i <= nodes.Upper(); ++i) {
                                gp_Pnt pt = nodes(i).Transformed(edge_trsf);
                                edge.m_curvePoints.emplace_back(pt.X(), pt.Y(), pt.Z());
                            }
                        }

                        model.addEdge(edge);
                    }
                    face.m_edgeIds.push_back(edge_id);
                }

                model.addFace(face);
            }
            solid.m_faceIds.push_back(face_id);
        }

        model.addSolid(solid);
    }

    // Handle case where shape has no solids but has faces (sheet body)
    if(solid_map.empty()) {
        Solid sheet_solid;
        sheet_solid.m_id = model.generateNextId();
        part.m_solidIds.push_back(sheet_solid.m_id);

        for(TopExp_Explorer face_exp(shape, TopAbs_FACE); face_exp.More(); face_exp.Next()) {
            const TopoDS_Face& occ_face = TopoDS::Face(face_exp.Current());
            size_t face_hash = std::hash<TopoDS_Face>{}(occ_face);

            if(face_map.find(face_hash) != face_map.end()) {
                sheet_solid.m_faceIds.push_back(face_map[face_hash]);
                continue;
            }

            Face face;
            face.m_id = model.generateNextId();
            face_map[face_hash] = face.m_id;

            TopLoc_Location loc;
            Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(occ_face, loc);

            if(!triangulation.IsNull()) {
                const gp_Trsf& trsf = loc.Transformation();
                int num_nodes = triangulation->NbNodes();
                int num_tris = triangulation->NbTriangles();

                face.m_meshVertices.reserve(num_nodes);
                for(int i = 1; i <= num_nodes; ++i) {
                    gp_Pnt pt = triangulation->Node(i).Transformed(trsf);
                    RenderVertex rv;
                    rv.m_position = Point3D(pt.X(), pt.Y(), pt.Z());
                    if(triangulation->HasNormals()) {
                        gp_Dir normal = triangulation->Normal(i);
                        rv.m_normal = Point3D(normal.X(), normal.Y(), normal.Z());
                    }
                    face.m_meshVertices.push_back(rv);
                }

                face.m_meshIndices.reserve(num_tris * 3);
                for(int i = 1; i <= num_tris; ++i) {
                    const Poly_Triangle& tri = triangulation->Triangle(i);
                    int n1, n2, n3;
                    tri.Get(n1, n2, n3);
                    face.m_meshIndices.push_back(static_cast<uint32_t>(n1 - 1));
                    face.m_meshIndices.push_back(static_cast<uint32_t>(n2 - 1));
                    face.m_meshIndices.push_back(static_cast<uint32_t>(n3 - 1));
                }
            }

            model.addFace(face);
            sheet_solid.m_faceIds.push_back(face.m_id);
        }

        if(!sheet_solid.m_faceIds.empty()) {
            model.addSolid(sheet_solid);
        }
    }

    model.addPart(part);
}

} // namespace Geometry
} // namespace OpenGeoLab
