/**
 * @file tessellator.cpp
 * @brief Implements OCC shape tessellation helpers for render mesh generation.
 */

#include "tessellator.hpp"

#include <BRepAdaptor_Curve.hxx>
#include <BRepBndLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <Poly_Triangulation.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <gp.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace OpenGeoLab::Geometry {
namespace {

constexpr std::size_t ComponentsPerVertex = 3;
constexpr Standard_Real EdgeSampleSpacing = 0.25;
constexpr Standard_Real MinimumEdgeParameterStep = 1.0e-6;

void appendPoint(Scene::RenderMeshData& mesh, const gp_Pnt& point) {
    mesh.positions.push_back(static_cast<float>(point.X()));
    mesh.positions.push_back(static_cast<float>(point.Y()));
    mesh.positions.push_back(static_cast<float>(point.Z()));
}

void appendNormal(Scene::RenderMeshData& mesh, const gp_Dir& normal) {
    mesh.normals.push_back(static_cast<float>(normal.X()));
    mesh.normals.push_back(static_cast<float>(normal.Y()));
    mesh.normals.push_back(static_cast<float>(normal.Z()));
}

[[nodiscard]] gp_Dir safeNormalized(const gp_Vec& vector, const gp_Dir& fallback) {
    if(vector.SquareMagnitude() <= gp::Resolution()) {
        return fallback;
    }

    gp_Vec normalized = vector;
    normalized.Normalize();
    return gp_Dir(normalized);
}

[[nodiscard]] gp_Dir faceNormalFromTriangle(const std::array<gp_Pnt, 3>& triangle_points,
                                            TopAbs_Orientation orientation) {
    const gp_Vec edge_ab(triangle_points[0], triangle_points[1]);
    const gp_Vec edge_ac(triangle_points[0], triangle_points[2]);
    gp_Vec normal = edge_ab.Crossed(edge_ac);
    if(orientation == TopAbs_REVERSED) {
        normal.Reverse();
    }

    return safeNormalized(normal, gp::DZ());
}

void appendTriangle(Scene::RenderMeshData& mesh,
                    const std::array<gp_Pnt, 3>& triangle_points,
                    const std::array<gp_Dir, 3>& triangle_normals) {
    const std::uint32_t base_index =
        static_cast<std::uint32_t>(mesh.positions.size() / ComponentsPerVertex);

    for(std::size_t point_index = 0; point_index < triangle_points.size(); ++point_index) {
        appendPoint(mesh, triangle_points[point_index]);
        appendNormal(mesh, triangle_normals[point_index]);
        mesh.indices.push_back(base_index + static_cast<std::uint32_t>(point_index));
    }
}

[[nodiscard]] std::vector<gp_Pnt> sampleEdgePoints(const BRepAdaptor_Curve& curve) {
    std::vector<gp_Pnt> sampled_points;

    GCPnts_UniformAbscissa sampler(curve, EdgeSampleSpacing);
    if(sampler.IsDone() && sampler.NbPoints() >= 2) {
        sampled_points.reserve(static_cast<std::size_t>(sampler.NbPoints()));
        for(Standard_Integer point_index = 1; point_index <= sampler.NbPoints(); ++point_index) {
            sampled_points.push_back(curve.Value(sampler.Parameter(point_index)));
        }
        return sampled_points;
    }

    const Standard_Real first_parameter = curve.FirstParameter();
    const Standard_Real last_parameter = curve.LastParameter();
    if(std::abs(last_parameter - first_parameter) <= MinimumEdgeParameterStep) {
        sampled_points.push_back(curve.Value(first_parameter));
        return sampled_points;
    }

    constexpr int fallback_segments = 8;
    sampled_points.reserve(static_cast<std::size_t>(fallback_segments) + 1U);
    for(int segment_index = 0; segment_index <= fallback_segments; ++segment_index) {
        const double alpha =
            static_cast<double>(segment_index) / static_cast<double>(fallback_segments);
        const Standard_Real parameter =
            first_parameter + (last_parameter - first_parameter) * alpha;
        sampled_points.push_back(curve.Value(parameter));
    }

    return sampled_points;
}

} // namespace

Scene::RenderMeshData Tessellator::tessellate(const TopoDS_Shape& shape,
                                              const TessellationOptions& options) {
    Scene::RenderMeshData mesh;
    mesh.topology = Scene::PrimitiveType::Triangles;

    if(shape.IsNull()) {
        return mesh;
    }

    BRepMesh_IncrementalMesh incremental_mesh(shape, options.linearDeflection, options.relative,
                                              options.angularDeflection);
    incremental_mesh.Perform();

    for(TopExp_Explorer explorer(shape, TopAbs_FACE); explorer.More(); explorer.Next()) {
        const TopoDS_Face face = TopoDS::Face(explorer.Current());
        TopLoc_Location location;
        const Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);
        if(triangulation.IsNull()) {
            continue;
        }

        const gp_Trsf transform = location.Transformation();
        const bool face_reversed = face.Orientation() == TopAbs_REVERSED;
        const bool has_normals = triangulation->HasNormals();

        for(Standard_Integer triangle_index = 1; triangle_index <= triangulation->NbTriangles();
            ++triangle_index) {
            Standard_Integer node_indices[3] = {};
            triangulation->Triangle(triangle_index)
                .Get(node_indices[0], node_indices[1], node_indices[2]);
            if(face_reversed) {
                std::swap(node_indices[1], node_indices[2]);
            }

            std::array<gp_Pnt, 3> triangle_points{};
            for(std::size_t point_index = 0; point_index < triangle_points.size(); ++point_index) {
                triangle_points[point_index] =
                    triangulation->Node(node_indices[point_index]).Transformed(transform);
            }

            std::array<gp_Dir, 3> triangle_normals{};
            if(has_normals) {
                for(std::size_t normal_index = 0; normal_index < triangle_normals.size();
                    ++normal_index) {
                    gp_Dir normal = triangulation->Normal(node_indices[normal_index]);
                    normal.Transform(transform);
                    if(face_reversed) {
                        normal.Reverse();
                    }
                    triangle_normals[normal_index] = normal;
                }
            } else {
                const gp_Dir computed_normal =
                    faceNormalFromTriangle(triangle_points, face.Orientation());
                triangle_normals.fill(computed_normal);
            }

            appendTriangle(mesh, triangle_points, triangle_normals);
        }
    }

    return mesh;
}

Scene::RenderMeshData Tessellator::extractEdges(const TopoDS_Shape& shape) {
    Scene::RenderMeshData mesh;
    mesh.topology = Scene::PrimitiveType::Lines;

    if(shape.IsNull()) {
        return mesh;
    }

    for(TopExp_Explorer explorer(shape, TopAbs_EDGE); explorer.More(); explorer.Next()) {
        const TopoDS_Edge edge = TopoDS::Edge(explorer.Current());
        BRepAdaptor_Curve curve(edge);
        if(curve.FirstParameter() > curve.LastParameter()) {
            continue;
        }

        const std::vector<gp_Pnt> sampled_points = sampleEdgePoints(curve);
        if(sampled_points.size() < 2) {
            continue;
        }

        std::uint32_t previous_index =
            static_cast<std::uint32_t>(mesh.positions.size() / ComponentsPerVertex);
        appendPoint(mesh, sampled_points.front());

        for(std::size_t point_index = 1; point_index < sampled_points.size(); ++point_index) {
            const std::uint32_t current_index =
                static_cast<std::uint32_t>(mesh.positions.size() / ComponentsPerVertex);
            appendPoint(mesh, sampled_points[point_index]);
            mesh.indices.push_back(previous_index);
            mesh.indices.push_back(current_index);
            previous_index = current_index;
        }
    }

    return mesh;
}

Scene::BoundingBox Tessellator::computeBounds(const TopoDS_Shape& shape) {
    Scene::BoundingBox bounds;
    if(shape.IsNull()) {
        return bounds;
    }

    Bnd_Box box;
    BRepBndLib::Add(shape, box);
    if(box.IsVoid()) {
        return bounds;
    }

    Standard_Real min_x = 0.0;
    Standard_Real min_y = 0.0;
    Standard_Real min_z = 0.0;
    Standard_Real max_x = 0.0;
    Standard_Real max_y = 0.0;
    Standard_Real max_z = 0.0;
    box.Get(min_x, min_y, min_z, max_x, max_y, max_z);

    bounds.min = {static_cast<float>(min_x), static_cast<float>(min_y), static_cast<float>(min_z)};
    bounds.max = {static_cast<float>(max_x), static_cast<float>(max_y), static_cast<float>(max_z)};
    return bounds;
}

} // namespace OpenGeoLab::Geometry
