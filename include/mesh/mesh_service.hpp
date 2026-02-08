/**
 * @file mesh_service.hpp
 * @brief Mesh module service entry for QML requests
 */
#pragma once

#include "app/service.hpp"

namespace OpenGeoLab::Mesh {
/**
 * @brief Service for processing mesh-related requests
 */
class MeshService : public App::IService {
public:
    MeshService() = default;
    ~MeshService() override = default;

    [[nodiscard]] nlohmann::json
    processRequest(const std::string& module_name,
                   const nlohmann::json& params,
                   App::IProgressReporterPtr progress_reporter) override;
};

/**
 * @brief Singleton factory for MeshService
 */
class MeshServiceFactory : public App::IServiceSingletonFactory {
public:
    MeshServiceFactory() = default;
    ~MeshServiceFactory() override = default;

    tObjectSharedPtr instance() const override;
};

/**
 * @brief Register mesh service and action factories
 */
void registerServices();
} // namespace OpenGeoLab::Mesh