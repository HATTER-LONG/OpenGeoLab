/**
 * @file render_service.hpp
 * @brief Render service for viewport/camera related requests
 *
 */

#pragma once

#include "app/service.hpp"

namespace OpenGeoLab::Render {

class RenderService : public App::IService {
public:
    RenderService() = default;
    ~RenderService() override = default;

    [[nodiscard]] nlohmann::json
    processRequest(const std::string& module_name,
                   const nlohmann::json& params,
                   App::IProgressReporterPtr progress_reporter) override;
};

class RenderServiceFactory : public App::IServiceSingletonFactory {
public:
    RenderServiceFactory() = default;
    ~RenderServiceFactory() override = default;

    tObjectSharedPtr instance() const override;
};

void registerServices();

} // namespace OpenGeoLab::Render
