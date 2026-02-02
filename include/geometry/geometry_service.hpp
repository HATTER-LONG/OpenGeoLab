/**
 * @file geometry_service.hpp
 * @brief Geometry service for processing geometry-related requests
 *
 * GeometryService provides a unified interface for geometry operations
 * including creation, modification, and querying of geometric entities.
 * It dispatches requests to appropriate action handlers.
 */

#pragma once

#include "app/service.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Service for processing geometry-related requests
 *
 * Handles geometry operations through an action-based dispatch system.
 * Actions are registered via GeometryActionFactory and include operations
 * such as shape creation, modification, and document management.
 */
class GeometryService : public App::IService {
public:
    GeometryService() = default;
    ~GeometryService() override = default;

    /**
     * @brief Process a geometry request
     * @param module_name Service identifier (unused, for interface compliance)
     * @param params JSON parameters with "action" key and action-specific data
     * @param progress_reporter Progress callback interface
     * @return JSON response with operation status
     * @note The "action" parameter determines which GeometryAction is invoked.
     */
    [[nodiscard]] nlohmann::json
    processRequest(const std::string& module_name,
                   const nlohmann::json& params,
                   App::IProgressReporterPtr progress_reporter) override;
};

/**
 * @brief Singleton factory for GeometryService
 */
class GeometryServiceFactory : public App::IServiceSingletonFactory {
public:
    GeometryServiceFactory() = default;
    ~GeometryServiceFactory() override = default;

    tObjectSharedPtr instance() const override;
};

/**
 * @brief Register all geometry-related services and action factories
 * @note Must be called during application initialization
 */
void registerServices();

} // namespace OpenGeoLab::Geometry