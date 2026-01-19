/**
 * @file model_reader.hpp
 * @brief 3D model file reader service interface and factory
 *
 * Provides a unified service interface for importing various CAD file formats.
 * Integrates with the geometry document system for model management.
 */

#pragma once

#include "app/service.hpp"
#include "io/reader.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <nlohmann/json.hpp>

namespace OpenGeoLab::IO {

/**
 * @brief Service for reading and importing 3D model files
 *
 * Supports various CAD formats (STEP, BREP) with progress reporting.
 * Automatically adds loaded parts to the geometry document.
 */
class ModelReader : public App::IService {
public:
    ModelReader() = default;
    ~ModelReader() override = default;

    /**
     * @brief Process a model import request
     * @param moduleName Service identifier
     * @param params JSON with "file_path" key
     * @param progressReporter Progress callback interface
     * @return JSON result with imported model metadata
     *
     * Expected params:
     * {
     *   "file_path": "path/to/model.step",
     *   "add_to_document": true  // optional, default true
     * }
     *
     * Returns on success:
     * {
     *   "success": true,
     *   "part_id": 12345,
     *   "part_name": "model",
     *   "solid_count": 1,
     *   "face_count": 6
     * }
     */
    nlohmann::json processRequest(const std::string& moduleName,
                                  const nlohmann::json& params,
                                  App::IProgressReporterPtr progressReporter) override;

private:
    /**
     * @brief Detect file format from extension
     * @param filePath Input file path
     * @return Reader ID string ("BrepReader", "StepReader", or empty on unknown)
     */
    std::string detectReaderType(const std::string& filePath) const;
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