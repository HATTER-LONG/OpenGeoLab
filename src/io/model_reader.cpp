/**
 * @file model_reader.cpp
 * @brief 3D model file import service implementation
 */
#include "io/model_reader.hpp"
#include "util/logger.hpp"

#include <chrono>
#include <thread>

namespace OpenGeoLab {
namespace IO {

nlohmann::json ModelReader::processRequest(const std::string& action_id,
                                           const nlohmann::json& params,
                                           App::ProgressReporterPtr reporter) {
    nlohmann::json result;
    result["success"] = false;
    result["action_id"] = action_id;

    if(action_id == "read_model") {
        if(!params.contains("file_path") || !params["file_path"].is_string()) {
            result["error"] = "Missing or invalid 'file_path' parameter";
            if(reporter) {
                reporter->reportError(result["error"].get<std::string>());
            }
            return result;
        }

        const std::string file_path = params["file_path"].get<std::string>();
        const bool success = readModel(file_path, reporter);

        result["success"] = success;
        result["file_path"] = file_path;

        if(!success) {
            result["error"] = "Failed to read model file";
        }
        return result;
    }

    result["error"] = "Unknown action: " + action_id;
    return result;
}

bool ModelReader::readModel(const std::string& file_path, App::ProgressReporterPtr reporter) {
    LOG_INFO("Reading model from file: {}", file_path);

    // Simulate reading progress for demonstration
    const int total_steps = 10;
    for(int i = 0; i <= total_steps; ++i) {
        if(reporter) {
            if(reporter->isCancelled()) {
                LOG_INFO("Model reading cancelled");
                return false;
            }

            const double progress = static_cast<double>(i) / static_cast<double>(total_steps);
            reporter->reportProgress(progress, "Reading model: " + std::to_string(i * 10) + "%");
        }

        // Simulate work (replace with actual model loading logic)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO("Model loaded successfully: {}", file_path);
    return true;
}

ModelReaderFactory::tObjectSharedPtr ModelReaderFactory::instance() const {
    static auto obj = std::make_shared<ModelReader>();
    return obj;
}

} // namespace IO
} // namespace OpenGeoLab