/**
 * @file reader_service.hpp
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
class ReaderService : public App::IService {
public:
    ReaderService() = default;
    ~ReaderService() override = default;

    /**
     * @brief Process a model import request
     * @param module_name Service identifier
     * @param params JSON object.
     *              Required fields:
     *              - action: "load_model"
     *              - file_path: absolute or relative file path
     * @param progress_reporter Progress callback interface
     * @return JSON result with imported model metadata.
     *         On success:
     *         - success: true
     *         - action: "load_model"
     *         - file_path: input file path
     *         - reader: reader id (e.g., "StepReader", "BrepReader")
     *         - entity_count: created entity count
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
 * @brief Singleton factory for ReaderService
 */
class ReaderServiceFactory : public App::IServiceSingletonFactory {
public:
    ReaderServiceFactory() = default;
    ~ReaderServiceFactory() override = default;

    tObjectSharedPtr instance() const override;
};
void registerServices();
} // namespace OpenGeoLab::IO