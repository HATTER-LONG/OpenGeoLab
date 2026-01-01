/**
 * @file step_reader.cpp
 * @brief STEP format reader implementation using OpenCASCADE.
 */
#include "io/step_reader.hpp"
#include "geometry/occ_converter.hpp"
#include "util/logger.hpp"

#include <STEPControl_Reader.hxx>
#include <TopoDS_Shape.hxx>

#include <filesystem>

namespace OpenGeoLab {
namespace IO {

GeometryDataPtr StepReader::read(const std::string& file_path) {
    LOG_INFO("StepReader: Reading STEP file: {}", file_path);

    auto data = std::make_shared<GeometryData>();

    if(!parseStepFile(file_path, *data)) {
        LOG_ERROR("StepReader: Failed to parse STEP file: {}", file_path);
        return nullptr;
    }

    LOG_INFO("StepReader: Successfully loaded geometry - {}", data->getSummary());
    return data;
}

bool StepReader::parseStepFile(const std::string& file_path, GeometryData& data) {
    // Check file exists
    if(!std::filesystem::exists(file_path)) {
        LOG_ERROR("StepReader: File does not exist: {}", file_path);
        return false;
    }

    // Read STEP file using OpenCASCADE
    STEPControl_Reader reader;
    IFSelect_ReturnStatus status = reader.ReadFile(file_path.c_str());

    if(status != IFSelect_RetDone) {
        LOG_ERROR("StepReader: Failed to read STEP file, status: {}", static_cast<int>(status));
        return false;
    }

    // Transfer all roots
    int numRoots = reader.NbRootsForTransfer();
    LOG_INFO("StepReader: Found {} roots in STEP file", numRoots);

    reader.TransferRoots();
    int numShapes = reader.NbShapes();
    LOG_INFO("StepReader: Transferred {} shapes from STEP file", numShapes);

    if(numShapes == 0) {
        LOG_ERROR("StepReader: No shapes found in STEP file");
        return false;
    }

    // Get the shape (combine all shapes if multiple)
    TopoDS_Shape shape = reader.OneShape();

    if(shape.IsNull()) {
        LOG_ERROR("StepReader: Combined shape is null");
        return false;
    }

    LOG_INFO("StepReader: Successfully read STEP shape from file");

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
        LOG_ERROR("StepReader: Failed to convert OCC shape");
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

    LOG_INFO("StepReader: Converted STEP geometry - {}", data.getSummary());
    return true;
}

} // namespace IO
} // namespace OpenGeoLab
