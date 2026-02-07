/**
 * @file render_service.hpp
 * @brief Render module service entry for QML requests
 *
 * RenderService implements the App::IService interface and dispatches
 * render-related actions (viewport control, selection control, etc.).
 */

#pragma once

#include "app/service.hpp"

namespace OpenGeoLab::Render {

/**
 * @brief Render service that processes JSON requests from the QML layer
 */
class RenderService : public App::IService {
public:
    RenderService() = default;
    ~RenderService() override = default;

    [[nodiscard]] nlohmann::json
    processRequest(const std::string& module_name,
                   const nlohmann::json& params,
                   App::IProgressReporterPtr progress_reporter) override;
};

/**
 * @brief Singleton factory for RenderService
 */
class RenderServiceFactory : public App::IServiceSingletonFactory {
public:
    RenderServiceFactory() = default;
    ~RenderServiceFactory() override = default;

    tObjectSharedPtr instance() const override;
};

/**
 * @brief Register render-related services to the application service registry
 */
void registerServices();

} // namespace OpenGeoLab::Render
