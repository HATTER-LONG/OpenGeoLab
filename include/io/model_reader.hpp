/**
 * @file model_reader.hpp
 * @brief 3D model file import service interface
 */
#pragma once

#include "app/service.hpp"
#include "geometry_data.hpp"

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
     * @brief Process model reading requests.
     * @param module_name Module name (ModelReader).
     * @param params Must contain "file_path" string.
     * @param reporter Optional progress reporter.
     * @return JSON with geometry summary on success.
     * @note Auto-detects file format (.brep, .step, .stp) and routes to appropriate reader.
     */
    nlohmann::json processRequest(const std::string& module_name,
                                  const nlohmann::json& params,
                                  App::ProgressReporterPtr reporter) override;

private:
    /**
     * @brief Internal model reading implementation
     * @param file_path Path to the model file
     * @param reporter Progress reporter for long operations
     * @return Geometry data on success, nullptr on failure
     */
    GeometryDataPtr readModel(const std::string& file_path, App::ProgressReporterPtr reporter);

    /**
     * @brief Detect file format from extension
     * @param file_path Input file path
     * @return File format string ("brep", "step", or empty on unknown)
     */
    std::string detectFileFormat(const std::string& file_path) const;
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