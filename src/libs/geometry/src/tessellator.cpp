/**
 * @file tessellator.cpp
 * @brief Tessellator — BRepMesh face triangulation, edge polylines, vertex extraction
 */

#include <opengeolab/geometry/tessellator.hpp>

#include <BRepAdaptor_Curve.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>

#include <array>
#include <cmath>

namespace OpenGeoLab::Geometry {

/** @brief Computes face normal for a triangle when triangulation lacks per-vertex normals. */
static std::array<float, 3>
computeTriangleNormal(const gp_Pnt& p0, const gp_Pnt& p1, const gp_Pnt& p2) {
    const double ax = p1.X() - p0.X(), ay = p1.Y() - p0.Y(), az = p1.Z() - p0.Z();
    const double bx = p2.X() - p0.X(), by = p2.Y() - p0.Y(), bz = p2.Z() - p0.Z();
    double cx = ay * bz - az * by;
    double cy = az * bx - ax * bz;
    double cz = ax * by - ay * bx;
    const double len = std::sqrt(cx * cx + cy * cy + cz * cz);
    if(len > 1e-12) {
        cx /= len;
        cy /= len;
        cz /= len;
    }
    return {static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(cz)};
}

/** @brief Extracts triangulated faces into SurfaceMesh and triangleTags. */
static void extractFaces(const ShapeEntry& entry, TessellationResult& result) {
    for(int fi = 1; fi <= entry.faceMap.Extent(); ++fi) {
        const auto face = TopoDS::Face(entry.faceMap.FindKey(fi));
        TopLoc_Location loc;
        const auto tri = BRep_Tool::Triangulation(face, loc);
        if(tri.IsNull()) {
            continue;
        }

        const bool reversed = (face.Orientation() == TopAbs_REVERSED);
        const auto& trsf = loc.Transformation();

        Core::SurfaceMesh surface;

        const int nb_nodes = tri->NbNodes();
        surface.positions.reserve(static_cast<std::size_t>(nb_nodes) * 3);
        surface.normals.reserve(static_cast<std::size_t>(nb_nodes) * 3);

        for(int ni = 1; ni <= nb_nodes; ++ni) {
            const gp_Pnt p = tri->Node(ni).Transformed(trsf);
            surface.positions.push_back(static_cast<float>(p.X()));
            surface.positions.push_back(static_cast<float>(p.Y()));
            surface.positions.push_back(static_cast<float>(p.Z()));
        }

        // Normals — use per-vertex normals if available, else compute per-triangle
        const bool has_normals = tri->HasNormals();
        if(has_normals) {
            for(int ni = 1; ni <= nb_nodes; ++ni) {
                gp_Dir n = tri->Normal(ni);
                // Apply rotation part of transformation
                n = n.IsEqual(gp_Dir(0, 0, 0), 1e-12) ? gp_Dir(0, 0, 1) : n;
                if(loc.IsIdentity() == Standard_False) {
                    n = n.IsEqual(gp_Dir(0, 0, 0), 1e-12) ? n : n.Transformed(trsf);
                }
                if(reversed) {
                    n.Reverse();
                }
                surface.normals.push_back(static_cast<float>(n.X()));
                surface.normals.push_back(static_cast<float>(n.Y()));
                surface.normals.push_back(static_cast<float>(n.Z()));
            }
        }

        const int nb_tri = tri->NbTriangles();
        surface.indices.reserve(static_cast<std::size_t>(nb_tri) * 3);

        for(int ti = 1; ti <= nb_tri; ++ti) {
            int n1{};
            int n2{};
            int n3{};
            tri->Triangle(ti).Get(n1, n2, n3);

            // OCC indices are 1-based; convert to 0-based
            if(reversed) {
                surface.indices.push_back(static_cast<uint32_t>(n1 - 1));
                surface.indices.push_back(static_cast<uint32_t>(n3 - 1));
                surface.indices.push_back(static_cast<uint32_t>(n2 - 1));
            } else {
                surface.indices.push_back(static_cast<uint32_t>(n1 - 1));
                surface.indices.push_back(static_cast<uint32_t>(n2 - 1));
                surface.indices.push_back(static_cast<uint32_t>(n3 - 1));
            }

            // Compute per-triangle normal if no per-vertex normals
            if(!has_normals) {
                const gp_Pnt p0 = tri->Node(n1).Transformed(trsf);
                const gp_Pnt p1 = tri->Node(n2).Transformed(trsf);
                const gp_Pnt p2 = tri->Node(n3).Transformed(trsf);
                auto [nx, ny, nz] = computeTriangleNormal(p0, p1, p2);
                if(reversed) {
                    nx = -nx;
                    ny = -ny;
                    nz = -nz;
                }
                // Fill normals for three vertices of this triangle
                // Note: this only works if each triangle has unique vertices,
                // otherwise normals will be overwritten. Per-vertex normals
                // from the triangulation are strongly preferred.
                for(const int vi : {n1, n2, n3}) {
                    const auto idx = static_cast<std::size_t>((vi - 1) * 3);
                    if(surface.normals.size() < static_cast<std::size_t>(nb_nodes) * 3) {
                        surface.normals.resize(static_cast<std::size_t>(nb_nodes) * 3, 0.0f);
                    }
                    surface.normals[idx] = nx;
                    surface.normals[idx + 1] = ny;
                    surface.normals[idx + 2] = nz;
                }
            }

            result.triangleTags.push_back({Core::EntityType::GeoFace, static_cast<uint32_t>(fi)});
        }

        result.visualData.surfaces.push_back(std::move(surface));
    }
}

/** @brief Extracts edge polylines into EdgeMesh and edgeTags. */
static void extractEdges(const ShapeEntry& entry, TessellationResult& result) {
    constexpr int k_samples = 50;

    for(int ei = 1; ei <= entry.edgeMap.Extent(); ++ei) {
        const auto edge = TopoDS::Edge(entry.edgeMap.FindKey(ei));
        if(BRep_Tool::Degenerated(edge)) {
            continue;
        }

        const BRepAdaptor_Curve curve(edge);
        const double first = curve.FirstParameter();
        const double last = curve.LastParameter();

        Core::EdgeMesh edge_mesh;
        edge_mesh.positions.reserve(static_cast<std::size_t>(k_samples + 1) * 3);

        for(int s = 0; s <= k_samples; ++s) {
            const double u = first + (last - first) * s / k_samples;
            const gp_Pnt p = curve.Value(u);
            edge_mesh.positions.push_back(static_cast<float>(p.X()));
            edge_mesh.positions.push_back(static_cast<float>(p.Y()));
            edge_mesh.positions.push_back(static_cast<float>(p.Z()));
        }

        // Line-segment indices: [0,1], [1,2], ..., [n-1,n]
        edge_mesh.indices.reserve(static_cast<std::size_t>(k_samples) * 2);
        for(int s = 0; s < k_samples; ++s) {
            edge_mesh.indices.push_back(static_cast<uint32_t>(s));
            edge_mesh.indices.push_back(static_cast<uint32_t>(s + 1));
            result.edgeTags.push_back({Core::EntityType::GeoEdge, static_cast<uint32_t>(ei)});
        }

        result.visualData.edges.push_back(std::move(edge_mesh));
    }
}

/** @brief Extracts topological vertices into PointSet and vertexTags. */
static void extractVertices(const ShapeEntry& entry, TessellationResult& result) {
    Core::PointSet points;
    points.positions.reserve(static_cast<std::size_t>(entry.vertexMap.Extent()) * 3);

    for(int vi = 1; vi <= entry.vertexMap.Extent(); ++vi) {
        const auto vertex = TopoDS::Vertex(entry.vertexMap.FindKey(vi));
        const gp_Pnt p = BRep_Tool::Pnt(vertex);
        points.positions.push_back(static_cast<float>(p.X()));
        points.positions.push_back(static_cast<float>(p.Y()));
        points.positions.push_back(static_cast<float>(p.Z()));
        result.vertexTags.push_back({Core::EntityType::GeoVertex, static_cast<uint32_t>(vi)});
    }

    if(!points.positions.empty()) {
        result.visualData.points.push_back(std::move(points));
    }
}

TessellationResult tessellate(const ShapeEntry& entry, const TessellationParams& params) {
    if(params.keepTriangulation) {
        // Preserve existing Poly_Triangulation; only mesh faces that lack one.
        for(int fi = 1; fi <= entry.faceMap.Extent(); ++fi) {
            const auto face = TopoDS::Face(entry.faceMap.FindKey(fi));
            TopLoc_Location loc;
            const auto tri = BRep_Tool::Triangulation(face, loc);
            if(tri.IsNull()) {
                const BRepMesh_IncrementalMesh mesher(face, params.linearDeflection,
                                                     Standard_False, params.angularDeflection);
            }
        }
    } else {
        // TODO: Clean existing triangulations to force complete re-tessellation.
        // Without this, BRepMesh_IncrementalMesh may reuse broken Poly_Triangulation
        // data loaded from BREP files if the stored deflection passes the consistency check.
        // BRepTools::Clean(entry.shape);
        const BRepMesh_IncrementalMesh mesher(entry.shape, params.linearDeflection, Standard_False,
                                              params.angularDeflection);
    }

    TessellationResult result;
    extractFaces(entry, result);
    extractEdges(entry, result);
    extractVertices(entry, result);
    return result;
}

} // namespace OpenGeoLab::Geometry
