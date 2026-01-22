/**
 * @file model_reader.hpp
 * @brief 3D model file reader service interface and factory
 */

#pragma once

#include "app/service.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <nlohmann/json.hpp>

namespace OpenGeoLab::IO {

/**
 * @brief Service for reading and importing 3D model files
 *
 * Supports various CAD formats (STEP, BREP) with progress reporting.
 */
class ModelReader : public App::IService {
public:
    ModelReader() = default;
    ~ModelReader() override = default;

    /**
     * @brief Process a model import request
     * @param module_name Service identifier
     * @param params JSON with "file_path" key
     * @param progress_reporter Progress callback interface
     * @return JSON result with imported model metadata
     */
    [[nodiscard]] nlohmann::json
    processRequest(const std::string& module_name,
                   const nlohmann::json& params,
                   App::IProgressReporterPtr progress_reporter) override;

private:
    /**
     * @brief Detect file format from extension
     * @param file_path Input file path
     * @return File format string ("brep", "step", or empty on unknown)
     */
    [[nodiscard]] std::string detectFileFormat(const std::string& file_path) const;
};

/**
 * @brief Singleton factory for ModelReader service
 */
class ModelReaderFactory : public App::IServiceSigletonFactory {
public:
    ModelReaderFactory() = default;
    ~ModelReaderFactory() override = default;

    tObjectSharedPtr instance() const override;
};

} // namespace OpenGeoLab::IO