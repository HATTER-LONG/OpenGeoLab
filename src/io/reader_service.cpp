/**
 * @file reader_service.cpp
 * @brief Implementation of ReaderService for 3D model file processing
 */

#include "io/reader_service.hpp"
#include "io/reader.hpp"
#include "util/logger.hpp"
#include "util/progress_bridge.hpp"

#include "brep_reader.hpp"
#include "step_reader.hpp"

namespace OpenGeoLab::IO {

nlohmann::json ReaderService::processRequest(const std::string& /*module_name*/,
                                             const nlohmann::json& params,
                                             App::IProgressReporterPtr progress_reporter) {
    nlohmann::json result;

    progress_reporter->reportProgress(0.0, "Starting model import...");

    if(!params.contains("file_path") || !params["file_path"].is_string()) {
        throw std::invalid_argument("Missing or invalid 'file_path' parameter.");
    }
    std::string file_path = params["file_path"].get<std::string>();
    LOG_DEBUG("Starting model import from file: {}", file_path);

    std::string reader_type = detectFileFormat(file_path);
    progress_reporter->reportProgress(0.01, "Creating reader [" + reader_type + "]...");
    auto reader = g_ComponentFactory.createObjectWithID<ReaderFactory>(reader_type);

    progress_reporter->reportProgress(0.05, "Reading file...");

    auto reader_result = reader->readFile(
        file_path, OpenGeoLab::Util::makeProgressCallback(progress_reporter, 0.05, 0.75));
    if(!reader_result.m_success) {
        throw std::runtime_error("Model import failed " + reader_result.m_errorMessage);
    }
    return result;
}

std::string ReaderService::detectFileFormat(const std::string& file_path) const {
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

ReaderServiceFactory::tObjectSharedPtr ReaderServiceFactory::instance() const {
    static tObjectSharedPtr singleton_instance = std::make_shared<ReaderService>();
    return singleton_instance;
}

void registerServices() {
    g_ComponentFactory.registInstanceFactoryWithID<IO::ReaderServiceFactory>("ReaderService");

    g_ComponentFactory.registFactoryWithID<IO::BrepReaderFactory>("BrepReader");
    g_ComponentFactory.registFactoryWithID<IO::StepReaderFactory>("StepReader");
}

} // namespace OpenGeoLab::IO