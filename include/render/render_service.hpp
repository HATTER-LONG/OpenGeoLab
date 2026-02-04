/**
 * @file render_service.hpp
 * @brief Render service for viewport and camera control requests
 *
 * Provides a service layer for handling rendering-related operations such as
 * camera manipulation (fit, reset, view presets) and scene refresh. Dispatches
 * actions to the RenderSceneController based on JSON request parameters.
 */

#pragma once

#include "app/service.hpp"

namespace OpenGeoLab::Render {

/**
 * @brief Service for processing render and viewport control requests
 *
 * RenderService handles viewport control actions dispatched via the
 * BackendService. Supported actions include:
 * - ViewPortControl: Camera manipulation (fit, reset, front/back/top/bottom/left/right views)
 * - Scene refresh and update requests
 *
 * Actions are specified via the "action" field in the JSON params.
 */
class RenderService : public App::IService {
public:
    RenderService() = default;
    ~RenderService() override = default;

    /**
     * @brief Process a render service request
     * @param module_name Service module name (should be "RenderService")
     * @param params JSON parameters containing action and view_ctrl fields
     * @param progress_reporter Progress callback (unused for most render operations)
     * @return JSON response with "success" status
     */
    [[nodiscard]] nlohmann::json
    processRequest(const std::string& module_name,
                   const nlohmann::json& params,
                   App::IProgressReporterPtr progress_reporter) override;
};

/**
 * @brief Factory for creating RenderService singleton instance
 */
class RenderServiceFactory : public App::IServiceSingletonFactory {
public:
    RenderServiceFactory() = default;
    ~RenderServiceFactory() override = default;

    /**
     * @brief Get the singleton RenderService instance
     * @return Shared pointer to the RenderService
     */
    tObjectSharedPtr instance() const override;
};

/**
 * @brief Register render services with the component factory
 *
 * Registers RenderService and related factories for dependency injection.
 */
void registerServices();

} // namespace OpenGeoLab::Render
