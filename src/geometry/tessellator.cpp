/**
 * @file tessellator.cpp
 * @brief Implementation of geometry tessellation for OpenGL rendering
 */

#include "geometry/tessellator.hpp"
#include "geometry/face_entity.hpp"
#include "util/logger.hpp"

#include <BRepAdaptor_Curve.hxx>
#include <BRepBndLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <cmath>

namespace OpenGeoLab::Geometry {

namespace {

/**
 * @brief Calculate shape diagonal for relative tolerance
 */
double calculateShapeDiagonal(const TopoDS_Shape& shape) {
    if(shape.IsNull()) {
        return 1.0;
    }

    Bnd_Box box;
    BRepBndLib::Add(shape, box);

    if(box.IsVoid()) {
        return 1.0;
    }

    double xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    double dx = xmax - xmin;
    double dy = ymax - ymin;
    double dz = zmax - zmin;

    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace

void Tessellator::meshShape(const TopoDS_Shape& shape, const TessellationParams& params) {
    if(shape.IsNull()) {
        return;
    }

    double linear_deflection = params.m_linearDeflection;

    if(params.m_relativeTolerance) {
        double diagonal = calculateShapeDiagonal(shape);
        linear_deflection = diagonal * params.m_linearDeflection;
    }

    BRepMesh_IncrementalMesh mesher(shape, linear_deflection, Standard_False,
                                    params.m_angularDeflection, Standard_True);
    mesher.Perform();
}

TriangleMesh Tessellator::extractTriangles(const TopoDS_Shape& shape) {
    TriangleMesh mesh;

    if(shape.IsNull()) {
        return mesh;
    }

    for(TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        const TopoDS_Face& face = TopoDS::Face(exp.Current());

        TopLoc_Location location;
        Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);

        if(triangulation.IsNull()) {
            continue;
        }

        const gp_Trsf& trsf = location.Transformation();
        bool has_transform = !location.IsIdentity();
        bool face_reversed = (face.Orientation() == TopAbs_REVERSED);

        // Get base vertex index for this face
        uint32_t base_vertex = static_cast<uint32_t>(mesh.vertexCount());

        // Extract vertices and normals
        int num_nodes = triangulation->NbNodes();
        for(int i = 1; i <= num_nodes; ++i) {
            gp_Pnt pnt = triangulation->Node(i);
            if(has_transform) {
                pnt.Transform(trsf);
            }

            mesh.m_vertices.push_back(static_cast<float>(pnt.X()));
            mesh.m_vertices.push_back(static_cast<float>(pnt.Y()));
            mesh.m_vertices.push_back(static_cast<float>(pnt.Z()));

            // Use UV normals if available, otherwise compute from triangles later
            if(triangulation->HasNormals()) {
                gp_Dir normal = triangulation->Normal(i);
                if(has_transform) {
                    normal.Transform(trsf);
                }
                if(face_reversed) {
                    normal.Reverse();
                }
                mesh.m_normals.push_back(static_cast<float>(normal.X()));
                mesh.m_normals.push_back(static_cast<float>(normal.Y()));
                mesh.m_normals.push_back(static_cast<float>(normal.Z()));
            } else {
                // Placeholder, will be computed later
                mesh.m_normals.push_back(0.0f);
                mesh.m_normals.push_back(0.0f);
                mesh.m_normals.push_back(1.0f);
            }
        }

        // Extract triangles
        int num_triangles = triangulation->NbTriangles();
        for(int i = 1; i <= num_triangles; ++i) {
            const Poly_Triangle& tri = triangulation->Triangle(i);
            int n1, n2, n3;
            tri.Get(n1, n2, n3);

            // Adjust for face orientation
            if(face_reversed) {
                std::swap(n2, n3);
            }

            mesh.m_indices.push_back(base_vertex + static_cast<uint32_t>(n1 - 1));
            mesh.m_indices.push_back(base_vertex + static_cast<uint32_t>(n2 - 1));
            mesh.m_indices.push_back(base_vertex + static_cast<uint32_t>(n3 - 1));
        }
    }

    return mesh;
}

EdgeMesh Tessellator::extractEdges(const TopoDS_Shape& shape, const TessellationParams& params) {
    EdgeMesh mesh;

    if(shape.IsNull()) {
        return mesh;
    }

    for(TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        const TopoDS_Edge& edge = TopoDS::Edge(exp.Current());

        if(BRep_Tool::Degenerated(edge)) {
            continue;
        }

        // Get edge curve
        double first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        if(curve.IsNull()) {
            continue;
        }

        // Discretize edge
        BRepAdaptor_Curve adaptor(edge);
        GCPnts_TangentialDeflection discretizer(adaptor, params.m_angularDeflection,
                                                params.m_linearDeflection);

        if(discretizer.NbPoints() < 2) {
            continue;
        }

        uint32_t base_vertex = static_cast<uint32_t>(mesh.vertexCount());

        // Add points
        for(int i = 1; i <= discretizer.NbPoints(); ++i) {
            gp_Pnt pnt = discretizer.Value(i);
            mesh.m_vertices.push_back(static_cast<float>(pnt.X()));
            mesh.m_vertices.push_back(static_cast<float>(pnt.Y()));
            mesh.m_vertices.push_back(static_cast<float>(pnt.Z()));
        }

        // Add line indices
        for(int i = 0; i < discretizer.NbPoints() - 1; ++i) {
            mesh.m_indices.push_back(base_vertex + static_cast<uint32_t>(i));
            mesh.m_indices.push_back(base_vertex + static_cast<uint32_t>(i + 1));
        }
    }

    return mesh;
}

RenderData Tessellator::tessellateShape(const TopoDS_Shape& shape,
                                        const TessellationParams& params) {
    RenderData data;

    if(shape.IsNull()) {
        return data;
    }

    // Mesh the shape
    meshShape(shape, params);

    // Extract triangles and edges
    data.m_triangleMesh = extractTriangles(shape);
    data.m_edgeMesh = extractEdges(shape, params);

    return data;
}

RenderData Tessellator::tessellateEntity(const GeometryEntityPtr& entity,
                                         const TessellationParams& params) {
    RenderData data;

    if(!entity) {
        return data;
    }

    data = tessellateShape(entity->shape(), params);
    data.m_entityId = entity->entityId();

    return data;
}

PartRenderData Tessellator::tessellatePart(const PartEntityPtr& part,
                                           const TessellationParams& params) {
    PartRenderData data;

    if(!part) {
        return data;
    }

    data.m_partId = part->entityId();
    data.m_partName = part->name();
    data.m_partColor = generatePartColor(part->entityId());
    data.m_boundingBox = part->boundingBox();

    // Tessellate the entire part shape
    meshShape(part->partShape(), params);

    // Extract combined mesh
    data.m_combinedData = tessellateShape(part->partShape(), params);
    data.m_combinedData.m_entityId = part->entityId();
    data.m_combinedData.m_faceColor = data.m_partColor;

    // Optionally tessellate individual faces for per-face selection
    for(TopExp_Explorer exp(part->partShape(), TopAbs_FACE); exp.More(); exp.Next()) {
        const TopoDS_Face& face = TopoDS::Face(exp.Current());

        RenderData face_data;
        face_data.m_triangleMesh = extractTriangles(face);
        face_data.m_edgeMesh = extractEdges(face, params);
        face_data.m_faceColor = data.m_partColor;

        data.m_faceData.push_back(std::move(face_data));
    }

    return data;
}

Color4f Tessellator::generatePartColor(EntityId part_id) {
    // Use golden ratio for hue distribution
    constexpr double golden_ratio = 0.618033988749895;
    double hue = std::fmod(static_cast<double>(part_id) * golden_ratio, 1.0);

    // HSV to RGB conversion (saturation=0.7, value=0.9)
    double s = 0.7;
    double v = 0.9;

    double h = hue * 6.0;
    int i = static_cast<int>(h);
    double f = h - i;
    double p = v * (1.0 - s);
    double q = v * (1.0 - s * f);
    double t = v * (1.0 - s * (1.0 - f));

    double r, g, b;
    switch(i % 6) {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    case 5:
    default:
        r = v;
        g = p;
        b = q;
        break;
    }

    return Color4f(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), 1.0f);
}

} // namespace OpenGeoLab::Geometry
