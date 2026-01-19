/**
 * @file brep_reader.cpp
 * @brief Implementation of BREP file reader
 */

#include "brep_reader.hpp"
#include "util/logger.hpp"

#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Shape.hxx>


#include <algorithm>
#include <filesystem>

namespace OpenGeoLab::IO {

ReadResult BrepReader::readFile(const std::string& filePath,
                                ReadProgressCallback progressCallback) {
    LOG_INFO("Reading BREP file: {}", filePath);

    // Verify file exists
    if(!std::filesystem::exists(filePath)) {
        LOG_ERROR("BREP file not found: {}", filePath);
        return ReadResult::Failure("File not found: " + filePath);
    }

    if(progressCallback && !progressCallback(0.1, "Initializing BREP reader...")) {
        return ReadResult::Failure("Operation cancelled");
    }

    try {
        if(progressCallback && !progressCallback(0.3, "Reading BREP file...")) {
            return ReadResult::Failure("Operation cancelled");
        }

        // Read the shape from file
        TopoDS_Shape shape;
        BRep_Builder builder;

        if(!BRepTools::Read(shape, filePath.c_str(), builder)) {
            LOG_ERROR("Failed to read BREP file: {}", filePath);
            return ReadResult::Failure("Failed to read BREP file");
        }

        if(shape.IsNull()) {
            LOG_ERROR("Loaded shape is null: {}", filePath);
            return ReadResult::Failure("No geometry found in file");
        }

        if(progressCallback && !progressCallback(0.7, "Creating part entity...")) {
            return ReadResult::Failure("Operation cancelled");
        }

        // Create the part entity
        auto part = std::make_shared<Geometry::PartEntity>(shape);

        // Extract filename for part name
        std::filesystem::path path(filePath);
        part->setName(path.stem().string());
        part->setFilePath(filePath);

        // Build the entity hierarchy
        part->buildHierarchy();

        if(progressCallback) {
            progressCallback(1.0, "BREP file loaded successfully");
        }

        LOG_INFO("Successfully loaded BREP file: {} ({} solids)", filePath, part->solids().size());

        return ReadResult::Success(part);

    } catch(const Standard_Failure& e) {
        std::string error = e.GetMessageString() ? e.GetMessageString() : "Unknown OCC error";
        LOG_ERROR("OCC exception reading BREP file: {}", error);
        return ReadResult::Failure("OCC error: " + error);
    } catch(const std::exception& e) {
        LOG_ERROR("Exception reading BREP file: {}", e.what());
        return ReadResult::Failure(std::string("Error: ") + e.what());
    }
}

bool BrepReader::canRead(const std::string& filePath) const {
    std::filesystem::path path(filePath);
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".brep" || ext == ".brp";
}

std::vector<std::string> BrepReader::supportedExtensions() const { return {".brep", ".brp"}; }

std::string BrepReader::description() const { return "OpenCASCADE BREP file reader"; }

} // namespace OpenGeoLab::IO