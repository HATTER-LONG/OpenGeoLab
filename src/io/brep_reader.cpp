/**
 * @file brep_reader.cpp
 * @brief BREP format reader implementation using OpenCASCADE.
 */
#include "brep_reader.hpp"
#include "geometry/occ_converter.hpp"
#include "util/logger.hpp"

#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Shape.hxx>

#include <filesystem>

namespace OpenGeoLab {
namespace IO {

GeometryDataPtr BrepReader::read(const std::string& file_path) {
    LOG_INFO("BrepReader: Reading BREP file: {}", file_path);

    auto data = std::make_shared<GeometryData>();

    if(!parseBrepFile(file_path, *data)) {
        LOG_ERROR("BrepReader: Failed to parse BREP file: {}", file_path);
        return nullptr;
    }

    LOG_INFO("BrepReader: Successfully loaded geometry - {}", data->getSummary());
    return data;
}

bool BrepReader::parseBrepFile(const std::string& file_path, GeometryData& data) {
    // Check file exists
    if(!std::filesystem::exists(file_path)) {
        LOG_ERROR("BrepReader: File does not exist: {}", file_path);
        return false;
    }

    // Read BREP file using OpenCASCADE
    TopoDS_Shape shape;
    BRep_Builder builder;

    if(!BRepTools::Read(shape, file_path.c_str(), builder)) {
        LOG_ERROR("BrepReader: BRepTools::Read failed for: {}", file_path);
        return false;
    }

    if(shape.IsNull()) {
        LOG_ERROR("BrepReader: Loaded shape is null");
        return false;
    }

    LOG_INFO("BrepReader: Successfully read BREP shape from file");

    // Convert OCC shape to our geometry format
    Geometry::OccConverter converter;
    Geometry::OccConverter::TessellationParams params;
    params.linearDeflection = 0.1;
    params.angularDeflection = 0.5;

    // Extract filename for part name
    std::filesystem::path path(file_path);
    std::string partName = path.stem().string();

    auto model = converter.convertShape(shape, partName, params);
    if(!model) {
        LOG_ERROR("BrepReader: Failed to convert OCC shape");
        return false;
    }

    // Copy data from model to GeometryData
    for(const auto& part : model->getParts()) {
        data.m_parts.push_back(part);
    }
    for(const auto& solid : model->getSolids()) {
        data.m_solids.push_back(solid);
    }
    for(const auto& face : model->getFaces()) {
        data.m_faces.push_back(face);
    }
    for(const auto& edge : model->getEdges()) {
        data.m_edges.push_back(edge);
    }
    for(const auto& vertex : model->getVertices()) {
        data.m_vertices.push_back(vertex);
    }

    LOG_INFO("BrepReader: Converted BREP geometry - {}", data.getSummary());
    return true;
}

} // namespace IO
} // namespace OpenGeoLab
