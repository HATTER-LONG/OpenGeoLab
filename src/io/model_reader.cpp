/**
 * @file model_reader.cpp
 * @brief Implementation of ModelReader service for 3D model file processing
 */

#include "io/model_reader.hpp"
#include "io/reader.hpp"
#include "util/logger.hpp"

#include <kangaroo/util/current_thread.hpp>
namespace OpenGeoLab::IO {

nlohmann::json ModelReader::processRequest(const std::string& /*module_name*/,
                                           const nlohmann::json& params,
                                           App::IProgressReporterPtr progress_reporter) {
    nlohmann::json result;
    progress_reporter->reportProgress(0.0, "Starting model import...");
    if(!params.contains("file_path") || !params["file_path"].is_string()) {
        throw std::invalid_argument("Missing or invalid 'file_path' parameter.");
    }
    std::string file_path = params["file_path"].get<std::string>();
    LOG_DEBUG("Starting model import from file: {}", file_path);

    std::string reader_id = detectFileFormat(file_path);
    auto reader = g_ComponentFactory.createObjectWithID<ReaderFactory>(reader_id);
    progress_reporter->reportProgress(0.2, "Reading model file by [" + reader_id + "]...");
    reader->readFile(file_path);
    return result;
}

std::string ModelReader::detectFileFormat(const std::string& file_path) const {
    std::filesystem::path path(file_path);
    std::string ext = path.extension().string();

    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if(ext == ".brep" || ext == ".brp") {
        return "BrepReader";
    }

    if(ext == ".step" || ext == ".stp") {
        return "StepReader";
    }
    throw std::invalid_argument("Unsupported file extension: " + ext);
}

ModelReaderFactory::tObjectSharedPtr ModelReaderFactory::instance() const {
    static tObjectSharedPtr singleton_instance = std::make_shared<ModelReader>();
    return singleton_instance;
}

} // namespace OpenGeoLab::IO