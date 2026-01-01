/**
 * @file model_reader.cpp
 * @brief 3D model file import service implementation.
 */
#include "io/model_reader.hpp"
#include "io/brep_reader.hpp"
#include "io/step_reader.hpp"
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
        result["parts"] = static_cast<int>(geometry->parts.size());
        result["solids"] = static_cast<int>(geometry->solids.size());
        result["faces"] = static_cast<int>(geometry->faces.size());
        result["edges"] = static_cast<int>(geometry->edges.size());
        result["vertices"] = static_cast<int>(geometry->vertices.size());

        // Formatted details for ResultDialog display
        result["details"] = {{"File", file_path},
                             {"Parts", geometry->parts.size()},
                             {"Solids", geometry->solids.size()},
                             {"Faces", geometry->faces.size()},
                             {"Edges", geometry->edges.size()},
                             {"Vertices", geometry->vertices.size()}};
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
    std::string format = detectFileFormat(file_path);
    if(format.empty()) {
        LOG_ERROR("Unsupported file format: {}", file_path);
        if(reporter) {
            reporter->reportError("Unsupported file format");
        }
        return nullptr;
    }

    LOG_INFO("Detected file format: {}", format);

    // Route to appropriate reader
    GeometryDataPtr geometry = nullptr;

    if(format == "brep") {
        auto reader = g_ComponentFactory.getInstanceObjectWithID<BrepReaderFactory>("BrepReader");
        if(reader) {
            geometry = reader->read(file_path);
        } else {
            LOG_ERROR("BrepReader component not available");
        }
    } else if(format == "step") {
        auto reader = g_ComponentFactory.getInstanceObjectWithID<StepReaderFactory>("StepReader");
        if(reader) {
            geometry = reader->read(file_path);
        } else {
            LOG_ERROR("StepReader component not available");
        }
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
        return "brep";
    }

    if(ext == ".step" || ext == ".stp") {
        return "step";
    }

    return ""; // Unknown format
}

ModelReaderFactory::tObjectSharedPtr ModelReaderFactory::instance() const {
    static auto obj = std::make_shared<ModelReader>();
    return obj;
}

} // namespace IO
} // namespace OpenGeoLab