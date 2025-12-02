/**
 * @file step_reader.cpp
 * @brief Implementation of STEP file format reader
 *
 * Uses Open CASCADE Technology for reading STEP files and
 * mesh triangulation to generate renderable geometry data.
 */

#include "step_reader.hpp"

#include "brep_reader.hpp" // For VertexData and VertexDataHash

#include <core/logger.hpp>
#include <geometry/geometry.hpp>

// Open CASCADE includes
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <Poly_Triangulation.hxx>
#include <STEPControl_Reader.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab {
namespace IO {

namespace {
constexpr float EPSILON = 1e-6f;
constexpr double NORMAL_MAGNITUDE_THRESHOLD = 1e-7;
constexpr double LINEAR_DEFLECTION = 0.1;
constexpr double ANGULAR_DEFLECTION = 0.5;
} // namespace

bool StepReader::canRead(const std::string& file_path) const {
    std::string lower_path = file_path;
    std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);

    for(const auto& ext : getSupportedExtensions()) {
        if(lower_path.size() >= ext.size() &&
           lower_path.compare(lower_path.size() - ext.size(), ext.size(), ext) == 0) {
            return true;
        }
    }
    return false;
}

bool StepReader::loadStepFile(const std::string& file_path, TopoDS_Shape& shape) {
    STEPControl_Reader reader;

    IFSelect_ReturnStatus status = reader.ReadFile(file_path.c_str());
    if(status != IFSelect_RetDone) {
        m_lastError = "Failed to read STEP file: " + file_path;
        LOG_ERROR(m_lastError);
        return false;
    }

    // Print statistics about what was read
    Standard_Integer num_roots = reader.NbRootsForTransfer();
    LOG_DEBUG("STEP file contains {} root entities", num_roots);

    // Transfer all roots to shapes
    reader.TransferRoots();

    // Get the resulting shape
    shape = reader.OneShape();

    if(shape.IsNull()) {
        m_lastError = "Loaded STEP shape is null";
        LOG_ERROR(m_lastError);
        return false;
    }

    return true;
}

bool StepReader::triangulateShape(TopoDS_Shape& shape) {
    BRepMesh_IncrementalMesh mesher(shape, LINEAR_DEFLECTION, Standard_False, ANGULAR_DEFLECTION,
                                    Standard_True);
    mesher.Perform();

    if(!mesher.IsDone()) {
        m_lastError = "Mesh generation failed";
        LOG_ERROR(m_lastError);
        return false;
    }

    return true;
}

bool StepReader::extractTriangleData(const TopoDS_Shape& shape,
                                     std::vector<float>& vertex_data,
                                     std::vector<unsigned int>& index_data) {
    std::unordered_map<VertexData, unsigned int, VertexDataHash> vertex_map;
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

            int n1 = 0;
            int n2 = 0;
            int n3 = 0;
            triangle.Get(n1, n2, n3);

            if(is_reversed) {
                std::swap(n2, n3);
            }

            gp_Pnt p1 = triangulation->Node(n1).Transformed(transform);
            gp_Pnt p2 = triangulation->Node(n2).Transformed(transform);
            gp_Pnt p3 = triangulation->Node(n3).Transformed(transform);

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
                VertexData vertex;
                vertex.m_posX = static_cast<float>(points[j].X());
                vertex.m_posY = static_cast<float>(points[j].Y());
                vertex.m_posZ = static_cast<float>(points[j].Z());
                vertex.m_normalX = static_cast<float>(normal.X());
                vertex.m_normalY = static_cast<float>(normal.Y());
                vertex.m_normalZ = static_cast<float>(normal.Z());

                auto it = vertex_map.find(vertex);
                if(it == vertex_map.end()) {
                    vertex_data.push_back(vertex.m_posX);
                    vertex_data.push_back(vertex.m_posY);
                    vertex_data.push_back(vertex.m_posZ);
                    vertex_data.push_back(vertex.m_normalX);
                    vertex_data.push_back(vertex.m_normalY);
                    vertex_data.push_back(vertex.m_normalZ);
                    vertex_data.push_back(0.8f); // R
                    vertex_data.push_back(0.8f); // G
                    vertex_data.push_back(0.8f); // B (default gray color)

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
        m_lastError = "No geometry data extracted from STEP shape";
        LOG_ERROR(m_lastError);
        return false;
    }

    return true;
}

std::shared_ptr<Geometry::GeometryData> StepReader::read(const std::string& file_path) {
    m_lastError.clear();

    try {
        // Step 1: Load STEP file
        TopoDS_Shape shape;
        if(!loadStepFile(file_path, shape)) {
            return nullptr;
        }

        // Step 2: Perform triangulation
        if(!triangulateShape(shape)) {
            return nullptr;
        }

        // Step 3: Extract triangle data
        std::vector<float> vertex_data;
        std::vector<unsigned int> index_data;
        if(!extractTriangleData(shape, vertex_data, index_data)) {
            return nullptr;
        }

        // Step 4: Create MeshData object
        auto mesh_data = std::make_shared<Geometry::MeshData>();
        mesh_data->setVertexData(std::move(vertex_data));
        mesh_data->setIndexData(std::move(index_data));

        LOG_INFO("STEP loaded: {} vertices, {} triangles", mesh_data->vertexCount(),
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

} // namespace IO
} // namespace OpenGeoLab
