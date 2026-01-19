/**
 * @file model_reader.cpp
 * @brief Implementation of ModelReader service for 3D model file processing
 */

#include "io/model_reader.hpp"
#include "geometry/geometry_document.hpp"
#include "io/reader.hpp"
#include "util/logger.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/current_thread.hpp>

#include <filesystem>

namespace OpenGeoLab::IO {

nlohmann::json ModelReader::processRequest(const std::string& /*moduleName*/,
                                           const nlohmann::json& params,
                                           App::IProgressReporterPtr progressReporter) {
    nlohmann::json result;
    progressReporter->reportProgress(0.0, "Starting model import...");

    // Validate parameters
    if(!params.contains("file_path") || !params["file_path"].is_string()) {
        throw std::invalid_argument("Missing or invalid 'file_path' parameter.");
    }

    std::string filePath = params["file_path"].get<std::string>();
    bool addToDocument = params.value("add_to_document", true);

    LOG_DEBUG("Starting model import from file: {}", filePath);

    // Determine the appropriate reader
    std::string readerType = detectReaderType(filePath);
    if(readerType.empty()) {
        throw std::invalid_argument("Unsupported file format: " + filePath);
    }

    progressReporter->reportProgress(0.1, "Creating reader [" + readerType + "]...");

    // Create the reader
    auto reader = g_ComponentFactory.createObjectWithID<ReaderFactory>(readerType);
    if(!reader) {
        throw std::runtime_error("Failed to create reader: " + readerType);
    }

    // Create progress callback that bridges to the progress reporter
    ReadProgressCallback progressCallback = [&progressReporter](double progress,
                                                                const std::string& message) {
        // Scale progress: reader uses 10%-90% of total progress
        double scaledProgress = 0.1 + progress * 0.8;
        progressReporter->reportProgress(scaledProgress, message);
        return !progressReporter->isCancelled();
    };

    progressReporter->reportProgress(0.15, "Reading file...");

    // Read the file
    ReadResult readResult = reader->readFile(filePath, progressCallback);

    if(!readResult.success) {
        throw std::runtime_error(readResult.errorMessage);
    }

    if(!readResult.part) {
        throw std::runtime_error("Reader returned success but no part was created");
    }

    progressReporter->reportProgress(0.9, "Finalizing import...");

    // Add to document if requested
    if(addToDocument) {
        auto document = Geometry::GeometryDocument::instance();
        if(!document->addPart(readResult.part)) {
            LOG_WARN("Failed to add part to document (may already exist)");
        }
    }

    // Build result JSON
    result["success"] = true;
    result["part_id"] = readResult.part->id();
    result["part_name"] = readResult.part->name();
    result["solid_count"] = readResult.part->solids().size();
    result["face_count"] = readResult.part->faces().size();
    result["file_path"] = filePath;

    auto bbox = readResult.part->boundingBox();
    result["bounding_box"] = {{"min", {{"x", bbox.min.x}, {"y", bbox.min.y}, {"z", bbox.min.z}}},
                              {"max", {{"x", bbox.max.x}, {"y", bbox.max.y}, {"z", bbox.max.z}}}};

    progressReporter->reportProgress(1.0, "Import completed successfully");

    LOG_INFO("Model import completed: {} (ID: {}, {} solids, {} faces)", readResult.part->name(),
             readResult.part->id(), readResult.part->solids().size(),
             readResult.part->faces().size());

    return result;
}

std::string ModelReader::detectReaderType(const std::string& filePath) const {
    std::filesystem::path path(filePath);
    std::string ext = path.extension().string();

    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if(ext == ".brep" || ext == ".brp") {
        return "BrepReader";
    }

    if(ext == ".step" || ext == ".stp" || ext == ".p21") {
        return "StepReader";
    }

    LOG_WARN("Unknown file extension: {}", ext);
    return "";
}

ModelReaderFactory::tObjectSharedPtr ModelReaderFactory::instance() const {
    static tObjectSharedPtr singletonInstance = std::make_shared<ModelReader>();
    return singletonInstance;
}

} // namespace OpenGeoLab::IO