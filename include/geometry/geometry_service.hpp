#pragma once

#include "app/service.hpp"

namespace OpenGeoLab::Geometry {
class GeometryService : public App::IService {
public:
    GeometryService() = default;
    ~GeometryService() override = default;

    [[nodiscard]] nlohmann::json
    processRequest(const std::string& module_name,
                   const nlohmann::json& params,
                   App::IProgressReporterPtr progress_reporter) override;
};

class GeometryServiceFactory : public App::IServiceSigletonFactory {
public:
    GeometryServiceFactory() = default;
    ~GeometryServiceFactory() override = default;

    tObjectSharedPtr instance() const override;
};
} // namespace OpenGeoLab::Geometry