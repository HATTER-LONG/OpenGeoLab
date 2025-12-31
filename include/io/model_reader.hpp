/**
 * @file model_reader.hpp
 * @brief 3D model file import service interface
 */
#pragma once

#include "app/service.hpp"

#include <kangaroo/util/component_factory.hpp>

#include <string>

namespace OpenGeoLab {
namespace IO {

/**
 * @brief Service for reading 3D model files
 *
 * Handles import of various CAD formats (STEP, BREP, etc.).
 * Reports progress through the provided reporter interface.
 */
class ModelReader : public App::ServiceBase {
public:
    ModelReader() = default;
    virtual ~ModelReader() = default;

    /**
     * @brief Process model reading requests
     * @param action_id Must be "read_model" for this service
     * @param params Must contain "file_path" string
     * @param reporter Optional progress reporter
     * @return JSON with "success" bool and "file_path" on success
     */
    nlohmann::json processRequest(const std::string& action_id,
                                  const nlohmann::json& params,
                                  App::ProgressReporterPtr reporter) override;

private:
    /**
     * @brief Internal model reading implementation
     * @param file_path Path to the model file
     * @param reporter Progress reporter for long operations
     * @return true on success
     */
    bool readModel(const std::string& file_path, App::ProgressReporterPtr reporter);
};

/**
 * @brief Singleton factory for ModelReader service
 */
class ModelReaderFactory : public App::ServiceBaseSingletonFactory {
public:
    ModelReaderFactory() = default;
    virtual ~ModelReaderFactory() = default;

    tObjectSharedPtr instance() const override;
};

} // namespace IO
} // namespace OpenGeoLab