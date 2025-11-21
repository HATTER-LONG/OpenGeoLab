/**
 * @file model_importer.cpp
 * @brief Implementation of model importer for 3D file formats
 *
 * Supports loading and converting BREP files using Open CASCADE Technology.
 * Uses mesh triangulation to generate renderable geometry data.
 */

#include "model_importer.h"
#include "geometry.h"
#include "geometry3d.h"
#include "logger.hpp"

// Open CASCADE includes
#include <BRepGProp_Face.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <Poly_Triangulation.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <QFileInfo>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab {
namespace IO {

ModelImporter::ModelImporter(QObject* parent) : QObject(parent) {
    LOG_INFO("ModelImporter initialized");
}

void ModelImporter::setTargetRenderer(QObject* renderer) {
    m_targetRenderer = qobject_cast<UI::Geometry3D*>(renderer);
    if(m_targetRenderer) {
        LOG_INFO("Target renderer set successfully");
    } else {
        LOG_DEBUG("Failed to set target renderer - invalid object type");
    }
}

void ModelImporter::importModel(const QUrl& file_url) {
    QString file_path = file_url.toLocalFile();
    LOG_INFO("Importing model from: {}", file_path.toStdString());

    if(!QFileInfo::exists(file_path)) {
        QString error = QString("File does not exist: %1").arg(file_path);
        LOG_ERROR(error.toStdString());
        emit modelLoadFailed(error);
        return;
    }

    std::shared_ptr<Geometry::GeometryData> geometry_data;

    // Select loader based on file extension
    if(file_path.endsWith(".brep", Qt::CaseInsensitive)) {
        geometry_data = loadBrepFile(file_path);
    } else if(file_path.endsWith(".stp", Qt::CaseInsensitive) ||
              file_path.endsWith(".step", Qt::CaseInsensitive)) {
        geometry_data = loadStepFile(file_path);
    } else {
        QString error = "Unsupported file format. Use .brep or .stp/.step files";
        LOG_ERROR(error.toStdString());
        emit modelLoadFailed(error);
        return;
    }

    if(!geometry_data) {
        QString error = "Failed to load geometry from file";
        LOG_ERROR(error.toStdString());
        emit modelLoadFailed(error);
        return;
    }

    // Pass geometry data to renderer
    if(m_targetRenderer) {
        m_targetRenderer->setCustomGeometry(geometry_data);
        LOG_INFO("Model loaded successfully: {} vertices, {} indices", geometry_data->vertexCount(),
                 geometry_data->indexCount());
        emit modelLoaded(QFileInfo(file_path).fileName());
    } else {
        LOG_DEBUG("No target renderer set - model loaded but not displayed");
        emit modelLoadFailed("No renderer available");
    }
}

std::shared_ptr<Geometry::GeometryData> ModelImporter::loadBrepFile(const QString& file_path) {
    try {
        // Step 1: Read BREP file
        TopoDS_Shape shape;
        BRep_Builder builder;

        if(!BRepTools::Read(shape, file_path.toStdString().c_str(), builder)) {
            LOG_ERROR("Failed to read BREP file");
            return nullptr;
        }

        if(shape.IsNull()) {
            LOG_ERROR("Loaded shape is null");
            return nullptr;
        }

        // Step 2: Perform triangulation (linear deflection 0.1, angular deflection 0.5 rad)
        BRepMesh_IncrementalMesh mesher(shape, 0.1, Standard_False, 0.5, Standard_True);
        mesher.Perform();

        if(!mesher.IsDone()) {
            LOG_ERROR("Mesh generation failed");
            return nullptr;
        }

        // Step 3: Extract triangle data
        std::vector<float> vertex_data;
        std::vector<unsigned int> index_data;

        // Vertex deduplication structure
        struct Vertex {
            float x, y, z;
            float nx, ny, nz;

            bool operator==(const Vertex& other) const {
                const float epsilon = 1e-6f;
                return std::abs(x - other.x) < epsilon && std::abs(y - other.y) < epsilon &&
                       std::abs(z - other.z) < epsilon && std::abs(nx - other.nx) < epsilon &&
                       std::abs(ny - other.ny) < epsilon && std::abs(nz - other.nz) < epsilon;
            }
        };

        struct VertexHash {
            std::size_t operator()(const Vertex& v) const {
                return std::hash<float>()(v.x) ^ (std::hash<float>()(v.y) << 1) ^
                       (std::hash<float>()(v.z) << 2);
            }
        };

        std::unordered_map<Vertex, unsigned int, VertexHash> vertex_map;
        unsigned int current_index = 0;

        // Traverse all faces
        for(TopExp_Explorer face_exp(shape, TopAbs_FACE); face_exp.More(); face_exp.Next()) {
            TopoDS_Face face = TopoDS::Face(face_exp.Current());
            TopLoc_Location location;

            Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);

            if(triangulation.IsNull()) {
                LOG_DEBUG("Face has no triangulation");
                continue;
            }

            // Get transformation matrix
            gp_Trsf transform = location.Transformation();

            // Calculate face normal direction
            bool is_reversed = (face.Orientation() == TopAbs_REVERSED);

            const Poly_Array1OfTriangle& triangles = triangulation->InternalTriangles();

            // Traverse all triangles
            for(int i = triangles.Lower(); i <= triangles.Upper(); ++i) {
                const Poly_Triangle& triangle = triangles(i);

                int n1, n2, n3;
                triangle.Get(n1, n2, n3);

                // Adjust triangle vertex order based on face orientation
                if(is_reversed) {
                    std::swap(n2, n3);
                }

                // Get three vertices
                gp_Pnt p1 = triangulation->Node(n1).Transformed(transform);
                gp_Pnt p2 = triangulation->Node(n2).Transformed(transform);
                gp_Pnt p3 = triangulation->Node(n3).Transformed(transform);

                // Calculate triangle normal
                gp_Vec v1(p1, p2);
                gp_Vec v2(p1, p3);
                gp_Vec normal = v1.Crossed(v2);

                if(normal.Magnitude() > 1e-7) {
                    normal.Normalize();
                } else {
                    normal = gp_Vec(0, 0, 1); // Degenerate triangle, use default normal
                }

                // Process three vertices
                gp_Pnt points[3] = {p1, p2, p3};
                for(int j = 0; j < 3; ++j) {
                    Vertex vertex;
                    vertex.x = static_cast<float>(points[j].X());
                    vertex.y = static_cast<float>(points[j].Y());
                    vertex.z = static_cast<float>(points[j].Z());
                    vertex.nx = static_cast<float>(normal.X());
                    vertex.ny = static_cast<float>(normal.Y());
                    vertex.nz = static_cast<float>(normal.Z());

                    // Find or insert vertex
                    auto it = vertex_map.find(vertex);
                    if(it == vertex_map.end()) {
                        // New vertex - add to buffer
                        vertex_data.push_back(vertex.x);
                        vertex_data.push_back(vertex.y);
                        vertex_data.push_back(vertex.z);
                        vertex_data.push_back(vertex.nx);
                        vertex_data.push_back(vertex.ny);
                        vertex_data.push_back(vertex.nz);
                        vertex_data.push_back(0.8f); // R
                        vertex_data.push_back(0.8f); // G
                        vertex_data.push_back(0.8f); // B (default gray color)

                        vertex_map[vertex] = current_index;
                        index_data.push_back(current_index);
                        ++current_index;
                    } else {
                        // Existing vertex - reuse index
                        index_data.push_back(it->second);
                    }
                }
            }
        }

        if(vertex_data.empty()) {
            LOG_ERROR("No geometry data extracted from shape");
            return nullptr;
        }

        // Step 4: Create MeshData object
        auto mesh_data = std::make_shared<Geometry::MeshData>();
        mesh_data->setVertexData(std::move(vertex_data));
        mesh_data->setIndexData(std::move(index_data));

        LOG_INFO("BREP loaded: {} vertices, {} triangles", mesh_data->vertexCount(),
                 mesh_data->indexCount() / 3);

        return mesh_data;

    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC exception: {}", e.GetMessageString());
        return nullptr;
    } catch(const std::exception& e) {
        LOG_ERROR("Standard exception: {}", e.what());
        return nullptr;
    }
}

std::shared_ptr<Geometry::GeometryData>
ModelImporter::loadStepFile([[maybe_unused]] const QString& file_path) {
    // STEP file loading requires STEPControl_Reader
    // To be implemented in future
    LOG_DEBUG("STEP file import not yet implemented");
    emit modelLoadFailed("STEP file format not yet supported");
    return nullptr;
}

} // namespace IO
} // namespace OpenGeoLab