/**
 * @file model_reader.cpp
 * @brief 3D model file import service implementation.
 */
#include "io/model_reader.hpp"
#include "io/reader.hpp"
#include "util/logger.hpp"

#include <algorithm>
#include <filesystem>

namespace OpenGeoLab {
namespace IO {

nlohmann::json ModelReader::processRequest(const std::string& /* module_name */,
                                           const nlohmann::json& params,
                                           App::ProgressReporterPtr reporter) {
    nlohmann::json result;
    result["success"] = false;
    result["show"] = true;

    if(!params.contains("file_path") || !params["file_path"].is_string()) {
        result["error"] = "Missing or invalid 'file_path' parameter";
        result["title"] = "Import Failed";
        result["message"] = "No file path provided.";
        if(reporter) {
            reporter->reportError(result["error"].get<std::string>());
        }
        return result;
    }

    const std::string file_path = params["file_path"].get<std::string>();
    GeometryDataPtr geometry = readModel(file_path, reporter);

    const bool success = (geometry != nullptr);
    result["success"] = success;
    result["file_path"] = file_path;

    if(success) {
        // Store geometry in GeometryStore for other modules to access
        geometry->storeToGeometryStore();

        result["title"] = "Import Successful";
        result["message"] = "3D model loaded successfully.";

        // Top-level geometry summary for QML ModelManager
        result["parts"] = static_cast<int>(geometry->m_parts.size());
        result["solids"] = static_cast<int>(geometry->m_solids.size());
        result["faces"] = static_cast<int>(geometry->m_faces.size());
        result["edges"] = static_cast<int>(geometry->m_edges.size());
        result["vertices"] = static_cast<int>(geometry->m_vertices.size());

        // Formatted details for ResultDialog display
        result["details"] = {{"File", file_path},
                             {"Parts", geometry->m_parts.size()},
                             {"Solids", geometry->m_solids.size()},
                             {"Faces", geometry->m_faces.size()},
                             {"Edges", geometry->m_edges.size()},
                             {"Vertices", geometry->m_vertices.size()}};
    } else {
        result["error"] = "Failed to read model file";
        result["title"] = "Import Failed";
        result["message"] = "Could not load the model file.";
    }
    return result;
}

GeometryDataPtr ModelReader::readModel(const std::string& file_path,
                                       App::ProgressReporterPtr reporter) {
    LOG_INFO("Reading model from file: {}", file_path);

    // Detect file format
    std::string reader_name = detectFileFormat(file_path);
    if(reader_name.empty()) {
        LOG_ERROR("Unsupported file format: {}", file_path);
        if(reporter) {
            reporter->reportError("Unsupported file format");
        }
        return nullptr;
    }

    LOG_INFO("Detected file format: {}", reader_name);

    // Route to appropriate reader
    GeometryDataPtr geometry = nullptr;

    auto reader = g_ComponentFactory.createObjectWithID<ReaderFactory>(reader_name);
    if(reader) {
        geometry = reader->read(file_path);
    } else {
        LOG_ERROR("{} component not available", reader_name);
    }

    // Report progress
    if(reporter) {
        if(geometry) {
            reporter->reportProgress(1.0, "Model loaded: " + geometry->getSummary());
        } else {
            reporter->reportError("Failed to read model file");
        }
    }

    return geometry;
}

std::string ModelReader::detectFileFormat(const std::string& file_path) const {
    std::filesystem::path path(file_path);
    std::string ext = path.extension().string();

    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if(ext == ".brep" || ext == ".brp") {
        return "BrepReader";
    }

    if(ext == ".step" || ext == ".stp") {
        return "StepReader";
    }

    return ""; // Unknown format
}

ModelReaderFactory::tObjectSharedPtr ModelReaderFactory::instance() const {
    static auto obj = std::make_shared<ModelReader>();
    return obj;
}

} // namespace IO
} // namespace OpenGeoLab