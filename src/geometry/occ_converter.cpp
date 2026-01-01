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
                                            const std::string& partName,
                                            const TessellationParams& params) {
    auto model = std::make_shared<GeometryModel>();

    if(!addShapeToModel(shape, partName, *model, params)) {
        LOG_ERROR("OccConverter: Failed to convert shape");
        return nullptr;
    }

    return model;
}

bool OccConverter::addShapeToModel(const TopoDS_Shape& shape,
                                   const std::string& partName,
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
    extractGeometry(shape, partName, model);

    LOG_INFO("OccConverter: Converted shape '{}' - {}", partName, model.getSummary());
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

void OccConverter::extractGeometry(const TopoDS_Shape& shape,
                                   const std::string& partName,
                                   GeometryModel& model) {
    // Maps to track OCC shape to our ID mapping
    std::unordered_map<size_t, uint32_t> vertexMap;
    std::unordered_map<size_t, uint32_t> edgeMap;
    std::unordered_map<size_t, uint32_t> faceMap;
    std::unordered_map<size_t, uint32_t> solidMap;

    // Create the part
    Part part;
    part.m_id = model.generateNextId();
    part.m_name = partName;

    // Extract solids
    for(TopExp_Explorer solidExp(shape, TopAbs_SOLID); solidExp.More(); solidExp.Next()) {
        const TopoDS_Solid& occSolid = TopoDS::Solid(solidExp.Current());
        size_t solidHash = std::hash<TopoDS_Solid>{}(occSolid);

        if(solidMap.find(solidHash) != solidMap.end()) {
            continue; // Already processed
        }

        Solid solid;
        solid.m_id = model.generateNextId();
        solidMap[solidHash] = solid.m_id;
        part.m_solidIds.push_back(solid.m_id);

        // Extract faces for this solid
        for(TopExp_Explorer faceExp(occSolid, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
            const TopoDS_Face& occFace = TopoDS::Face(faceExp.Current());
            size_t faceHash = std::hash<TopoDS_Face>{}(occFace);

            uint32_t faceId;
            if(faceMap.find(faceHash) != faceMap.end()) {
                faceId = faceMap[faceHash];
            } else {
                Face face;
                face.m_id = model.generateNextId();
                faceId = face.m_id;
                faceMap[faceHash] = faceId;

                // Get tessellation
                TopLoc_Location loc;
                Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(occFace, loc);

                if(!triangulation.IsNull()) {
                    const gp_Trsf& trsf = loc.Transformation();
                    int numNodes = triangulation->NbNodes();
                    int numTris = triangulation->NbTriangles();

                    // Extract vertices with normals
                    face.m_meshVertices.reserve(numNodes);
                    for(int i = 1; i <= numNodes; ++i) {
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
                    face.m_meshIndices.reserve(numTris * 3);
                    for(int i = 1; i <= numTris; ++i) {
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
                for(TopExp_Explorer edgeExp(occFace, TopAbs_EDGE); edgeExp.More(); edgeExp.Next()) {
                    const TopoDS_Edge& occEdge = TopoDS::Edge(edgeExp.Current());
                    size_t edgeHash = std::hash<TopoDS_Edge>{}(occEdge);

                    uint32_t edgeId;
                    if(edgeMap.find(edgeHash) != edgeMap.end()) {
                        edgeId = edgeMap[edgeHash];
                    } else {
                        Edge edge;
                        edge.m_id = model.generateNextId();
                        edgeId = edge.m_id;
                        edgeMap[edgeHash] = edgeId;

                        // Get edge vertices
                        TopoDS_Vertex v1, v2;
                        TopExp_Explorer vertExp(occEdge, TopAbs_VERTEX);
                        if(vertExp.More()) {
                            v1 = TopoDS::Vertex(vertExp.Current());
                            vertExp.Next();
                            if(vertExp.More()) {
                                v2 = TopoDS::Vertex(vertExp.Current());
                            }
                        }

                        // Get or create vertex IDs
                        auto getVertexId = [&](const TopoDS_Vertex& v) -> uint32_t {
                            if(v.IsNull())
                                return 0;

                            size_t vHash = std::hash<TopoDS_Vertex>{}(v);
                            auto it = vertexMap.find(vHash);
                            if(it != vertexMap.end()) {
                                return it->second;
                            }

                            gp_Pnt pt = BRep_Tool::Pnt(v);
                            Vertex vert;
                            vert.m_id = model.generateNextId();
                            vert.m_position = Point3D(pt.X(), pt.Y(), pt.Z());
                            vertexMap[vHash] = vert.m_id;
                            model.addVertex(vert);
                            return vert.m_id;
                        };

                        edge.m_startVertexId = getVertexId(v1);
                        edge.m_endVertexId = getVertexId(v2);

                        // Get curve points for visualization
                        TopLoc_Location edgeLoc;
                        Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(occEdge, edgeLoc);
                        if(!polygon.IsNull()) {
                            const TColgp_Array1OfPnt& nodes = polygon->Nodes();
                            const gp_Trsf& edgeTrsf = edgeLoc.Transformation();
                            edge.m_curvePoints.reserve(nodes.Length());
                            for(int i = nodes.Lower(); i <= nodes.Upper(); ++i) {
                                gp_Pnt pt = nodes(i).Transformed(edgeTrsf);
                                edge.m_curvePoints.emplace_back(pt.X(), pt.Y(), pt.Z());
                            }
                        }

                        model.addEdge(edge);
                    }
                    face.m_edgeIds.push_back(edgeId);
                }

                model.addFace(face);
            }
            solid.m_faceIds.push_back(faceId);
        }

        model.addSolid(solid);
    }

    // Handle case where shape has no solids but has faces (sheet body)
    if(solidMap.empty()) {
        Solid sheetSolid;
        sheetSolid.m_id = model.generateNextId();
        part.m_solidIds.push_back(sheetSolid.m_id);

        for(TopExp_Explorer faceExp(shape, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
            const TopoDS_Face& occFace = TopoDS::Face(faceExp.Current());
            size_t faceHash = std::hash<TopoDS_Face>{}(occFace);

            if(faceMap.find(faceHash) != faceMap.end()) {
                sheetSolid.m_faceIds.push_back(faceMap[faceHash]);
                continue;
            }

            Face face;
            face.m_id = model.generateNextId();
            faceMap[faceHash] = face.m_id;

            TopLoc_Location loc;
            Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(occFace, loc);

            if(!triangulation.IsNull()) {
                const gp_Trsf& trsf = loc.Transformation();
                int numNodes = triangulation->NbNodes();
                int numTris = triangulation->NbTriangles();

                face.m_meshVertices.reserve(numNodes);
                for(int i = 1; i <= numNodes; ++i) {
                    gp_Pnt pt = triangulation->Node(i).Transformed(trsf);
                    RenderVertex rv;
                    rv.m_position = Point3D(pt.X(), pt.Y(), pt.Z());
                    if(triangulation->HasNormals()) {
                        gp_Dir normal = triangulation->Normal(i);
                        rv.m_normal = Point3D(normal.X(), normal.Y(), normal.Z());
                    }
                    face.m_meshVertices.push_back(rv);
                }

                face.m_meshIndices.reserve(numTris * 3);
                for(int i = 1; i <= numTris; ++i) {
                    const Poly_Triangle& tri = triangulation->Triangle(i);
                    int n1, n2, n3;
                    tri.Get(n1, n2, n3);
                    face.m_meshIndices.push_back(static_cast<uint32_t>(n1 - 1));
                    face.m_meshIndices.push_back(static_cast<uint32_t>(n2 - 1));
                    face.m_meshIndices.push_back(static_cast<uint32_t>(n3 - 1));
                }
            }

            model.addFace(face);
            sheetSolid.m_faceIds.push_back(face.m_id);
        }

        if(!sheetSolid.m_faceIds.empty()) {
            model.addSolid(sheetSolid);
        }
    }

    model.addPart(part);
}

} // namespace Geometry
} // namespace OpenGeoLab
