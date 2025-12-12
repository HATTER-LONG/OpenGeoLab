/**
 * @file shape_triangulator.cpp
 * @brief Implementation of shape triangulation utility
 *
 * Uses Open CASCADE Technology for mesh generation from TopoDS_Shape.
 */

#include <geometry/shape_triangulator.hpp>

#include <core/logger.hpp>

// Open CASCADE includes
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>


#include <cmath>
#include <unordered_map>


namespace OpenGeoLab {
namespace Geometry {

namespace {

constexpr float EPSILON = 1e-6f;
constexpr double NORMAL_MAGNITUDE_THRESHOLD = 1e-7;

/**
 * @brief Vertex data structure for deduplication
 */
struct VertexKey {
    float posX = 0.0f;
    float posY = 0.0f;
    float posZ = 0.0f;
    float normalX = 0.0f;
    float normalY = 0.0f;
    float normalZ = 0.0f;

    bool operator==(const VertexKey& other) const {
        return std::abs(posX - other.posX) < EPSILON && std::abs(posY - other.posY) < EPSILON &&
               std::abs(posZ - other.posZ) < EPSILON &&
               std::abs(normalX - other.normalX) < EPSILON &&
               std::abs(normalY - other.normalY) < EPSILON &&
               std::abs(normalZ - other.normalZ) < EPSILON;
    }
};

struct VertexKeyHash {
    std::size_t operator()(const VertexKey& v) const {
        return std::hash<float>()(v.posX) ^ (std::hash<float>()(v.posY) << 1) ^
               (std::hash<float>()(v.posZ) << 2);
    }
};

} // anonymous namespace

bool ShapeTriangulator::performTriangulation(TopoDS_Shape& shape,
                                             const TriangulationParams& params) {
    BRepMesh_IncrementalMesh mesher(shape, params.linearDeflection,
                                    params.relative ? Standard_True : Standard_False,
                                    params.angularDeflection, Standard_True);
    mesher.Perform();

    if(!mesher.IsDone()) {
        m_lastError = "Mesh generation failed";
        LOG_ERROR(m_lastError);
        return false;
    }

    return true;
}

bool ShapeTriangulator::extractTriangleData(const TopoDS_Shape& shape,
                                            const TriangulationParams& params,
                                            std::vector<float>& vertex_data,
                                            std::vector<unsigned int>& index_data) {
    std::unordered_map<VertexKey, unsigned int, VertexKeyHash> vertex_map;
    unsigned int current_index = 0;

    for(TopExp_Explorer face_exp(shape, TopAbs_FACE); face_exp.More(); face_exp.Next()) {
        TopoDS_Face face = TopoDS::Face(face_exp.Current());
        TopLoc_Location location;

        Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);

        if(triangulation.IsNull()) {
            LOG_DEBUG("Face has no triangulation");
            continue;
        }

        gp_Trsf transform = location.Transformation();
        bool is_reversed = (face.Orientation() == TopAbs_REVERSED);

        const Poly_Array1OfTriangle& triangles = triangulation->InternalTriangles();

        for(int i = triangles.Lower(); i <= triangles.Upper(); ++i) {
            const Poly_Triangle& triangle = triangles(i);

            int n1 = 0, n2 = 0, n3 = 0;
            triangle.Get(n1, n2, n3);

            if(is_reversed) {
                std::swap(n2, n3);
            }

            gp_Pnt p1 = triangulation->Node(n1).Transformed(transform);
            gp_Pnt p2 = triangulation->Node(n2).Transformed(transform);
            gp_Pnt p3 = triangulation->Node(n3).Transformed(transform);

            // Calculate face normal
            gp_Vec v1(p1, p2);
            gp_Vec v2(p1, p3);
            gp_Vec normal = v1.Crossed(v2);

            if(normal.Magnitude() > NORMAL_MAGNITUDE_THRESHOLD) {
                normal.Normalize();
            } else {
                normal = gp_Vec(0, 0, 1);
            }

            gp_Pnt points[3] = {p1, p2, p3};
            for(int j = 0; j < 3; ++j) {
                VertexKey vertex;
                vertex.posX = static_cast<float>(points[j].X());
                vertex.posY = static_cast<float>(points[j].Y());
                vertex.posZ = static_cast<float>(points[j].Z());
                vertex.normalX = static_cast<float>(normal.X());
                vertex.normalY = static_cast<float>(normal.Y());
                vertex.normalZ = static_cast<float>(normal.Z());

                auto it = vertex_map.find(vertex);
                if(it == vertex_map.end()) {
                    // Add new vertex: position (3) + normal (3) + color (3)
                    vertex_data.push_back(vertex.posX);
                    vertex_data.push_back(vertex.posY);
                    vertex_data.push_back(vertex.posZ);
                    vertex_data.push_back(vertex.normalX);
                    vertex_data.push_back(vertex.normalY);
                    vertex_data.push_back(vertex.normalZ);
                    vertex_data.push_back(params.colorR);
                    vertex_data.push_back(params.colorG);
                    vertex_data.push_back(params.colorB);

                    vertex_map[vertex] = current_index;
                    index_data.push_back(current_index);
                    ++current_index;
                } else {
                    index_data.push_back(it->second);
                }
            }
        }
    }

    if(vertex_data.empty()) {
        m_lastError = "No geometry data extracted from shape";
        LOG_ERROR(m_lastError);
        return false;
    }

    return true;
}

std::shared_ptr<GeometryData> ShapeTriangulator::triangulate(const TopoDS_Shape& shape,
                                                             const TriangulationParams& params) {
    m_lastError.clear();

    try {
        // Create a copy for triangulation (modifies the shape)
        TopoDS_Shape working_shape = shape;

        // Step 1: Perform triangulation
        if(!performTriangulation(working_shape, params)) {
            return nullptr;
        }

        // Step 2: Extract triangle data
        std::vector<float> vertex_data;
        std::vector<unsigned int> index_data;
        if(!extractTriangleData(working_shape, params, vertex_data, index_data)) {
            return nullptr;
        }

        // Step 3: Create MeshData object
        auto mesh_data = std::make_shared<MeshData>();
        mesh_data->setVertexData(std::move(vertex_data));
        mesh_data->setIndexData(std::move(index_data));

        LOG_INFO("Shape triangulated: {} vertices, {} triangles", mesh_data->vertexCount(),
                 mesh_data->indexCount() / 3);

        return mesh_data;

    } catch(const Standard_Failure& e) {
        m_lastError = std::string("OCC exception: ") + e.GetMessageString();
        LOG_ERROR(m_lastError);
        return nullptr;
    } catch(const std::exception& e) {
        m_lastError = std::string("Standard exception: ") + e.what();
        LOG_ERROR(m_lastError);
        return nullptr;
    }
}

} // namespace Geometry
} // namespace OpenGeoLab
